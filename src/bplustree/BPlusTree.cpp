#include "BPlusTree.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <iostream>

namespace {

constexpr int MIN_LEAF_KEYS = (BPTREE_N + 1) / 2;
constexpr int MIN_INTERNAL_KEYS = BPTREE_N / 2;

void requireFiniteKey(float key) {
    if (!std::isfinite(key)) {
        throw std::invalid_argument("Index keys must be finite");
    }
}

// First position with key greater or equal to target
int lowerBound(const Node& node, float key) {
    return static_cast<int>(std::lower_bound(node.keys, node.keys + node.num_keys, key)
                            - node.keys);
}

// First position with key strictly greater than target
int upperBound(const Node& node, float key) {
    return static_cast<int>(std::upper_bound(node.keys, node.keys + node.num_keys, key)
                            - node.keys);
}

int minimumKeys(const Node& node) {
    return node.isLeaf() ? MIN_LEAF_KEYS : MIN_INTERNAL_KEYS;
}
} 

BPlusTree::BPlusTree(Disk& index_disk, bool create_new) : disk_(index_disk) {
    if (create_new) {
        if (disk_.numBlocks() != 0) {
            throw std::invalid_argument("A new index requires an empty disk file");
        }
        Node root = allocateNode(true);
        writeNode(root);
    } else {
        readNode(ROOT_NODE_ID);
    }
}

Node BPlusTree::readNode(int32_t id) const {
    if (id < 0 || id >= disk_.numBlocks()) {
        throw std::runtime_error("Invalid index node reference");
    }

    if (measuring_query_) {
        ++query_node_accesses_;
    }

    std::array<char, BLOCK_SIZE> buffer{};
    disk_.readBlock(id, buffer.data());
    Node node{};
    std::memcpy(&node, buffer.data(), sizeof(node));
    if (node.node_id != id || node.num_keys < 0 || node.num_keys > BPTREE_N ||
        (node.is_leaf != 0 && node.is_leaf != 1)) {
        throw std::runtime_error("Invalid index node header");
    }
    return node;
}

void BPlusTree::writeNode(const Node& node) {
    std::array<char, BLOCK_SIZE> buffer{};
    std::memcpy(buffer.data(), &node, sizeof(node));
    disk_.writeBlock(node.node_id, buffer.data());
}

// Appends new empty disk blocks
Node BPlusTree::allocateNode(bool leaf) {
    int32_t id = disk_.allocateBlock();
    Node node{};
    node.init(id, leaf);
    return node;
}

Node BPlusTree::findLeaf(float key, bool insert_position,
                        std::vector<PathEntry>* path) const {
    Node node = readNode(ROOT_NODE_ID);

    // Search can start in a leaf before the actual first match to avoid skipping duplicates
    while (!node.isLeaf()) {
        int child = insert_position ? upperBound(node, key) : lowerBound(node, key);
        if (path != nullptr) path->push_back({node.node_id, child});
        node = readNode(node.child_node_ids[child]);
    }
    return node;
}

bool BPlusTree::advanceLeaf(Node& leaf, std::vector<PathEntry>& path) const {
    while (!path.empty()) {
        PathEntry& entry = path.back();
        Node parent = readNode(entry.node_id);
        if (entry.child_index < parent.num_keys) {
            ++entry.child_index;
            Node node = readNode(parent.child_node_ids[entry.child_index]);
            while (!node.isLeaf()) {
                path.push_back({node.node_id, 0});
                node = readNode(node.child_node_ids[0]);
            }
            leaf = node;
            return true;
        }
        path.pop_back();
    }
    return false;
}

void BPlusTree::insert(float key, RecordPointer record) {
    requireFiniteKey(key);
    if (record.block_id < 0 || record.slot < 0 || record.slot >= RECORDS_PER_BLOCK) {
        throw std::invalid_argument("Invalid data record reference");
    }
    std::vector<PathEntry> path;
    Node leaf = findLeaf(key, true, &path);
    const int position = upperBound(leaf, key);

    // If leaf has space, shift entries to insert new key
    if (!leaf.isFull()) {
        for (int i = leaf.num_keys; i > position; --i) {
            leaf.keys[i] = leaf.keys[i - 1];
            leaf.record_pointers[i] = leaf.record_pointers[i - 1];
        }
        leaf.keys[position] = key;
        leaf.record_pointers[position] = record;
        ++leaf.num_keys;
        writeNode(leaf);
        return;
    }

    // If leaf is full, hold in a temporary array
    std::array<float, BPTREE_N + 1> keys{};
    std::array<RecordPointer, BPTREE_N + 1> records{};
    for (int i = 0, old = 0; i <= BPTREE_N; ++i) {
        if (i == position) {
            keys[i] = key;
            records[i] = record;
        } else {
            keys[i] = leaf.keys[old];
            records[i] = leaf.record_pointers[old++];
        }
    }
    Node right = allocateNode(true);
    // Divide the entries into left and right leaf
    leaf.num_keys = (BPTREE_N + 1) / 2; 
    right.num_keys = BPTREE_N + 1 - leaf.num_keys;
    std::copy_n(keys.begin(), leaf.num_keys, leaf.keys);
    std::copy_n(records.begin(), leaf.num_keys, leaf.record_pointers);
    std::copy_n(keys.begin() + leaf.num_keys, right.num_keys, right.keys);
    std::copy_n(records.begin() + leaf.num_keys, right.num_keys, right.record_pointers);
    right.next_leaf = leaf.next_leaf;
    leaf.next_leaf = right.node_id;
    writeNode(leaf);
    writeNode(right);

    // Copy the new right leaf's smallest key into parent node
    float separator = right.keys[0];
    int32_t right_id = right.node_id;
    while (!path.empty()) {
        PathEntry entry = path.back();
        path.pop_back();
        Node parent = readNode(entry.node_id);
        int position_in_parent = entry.child_index;
        if (!parent.isFull()) {
            for (int i = parent.num_keys; i > position_in_parent; --i) {
                parent.keys[i] = parent.keys[i - 1];
                parent.child_node_ids[i + 1] = parent.child_node_ids[i];
            }
            parent.keys[position_in_parent] = separator;
            parent.child_node_ids[position_in_parent + 1] = right_id;
            ++parent.num_keys;
            writeNode(parent);
            return;
        }

        std::array<float, BPTREE_N + 1> parent_keys{};
        std::array<int32_t, BPTREE_N + 2> children{};
        for (int i = 0, old = 0; i <= BPTREE_N; ++i) {
            parent_keys[i] = (i == position_in_parent) ? separator : parent.keys[old++];
        }
        for (int i = 0, old = 0; i < BPTREE_N + 2; ++i) {
            children[i] = (i == position_in_parent + 1) ? right_id
                                                       : parent.child_node_ids[old++];
        }
        const int middle = (BPTREE_N + 1) / 2;
        Node sibling = allocateNode(false);
        parent.num_keys = middle;
        sibling.num_keys = BPTREE_N - middle;
        std::copy_n(parent_keys.begin(), parent.num_keys, parent.keys);
        std::copy_n(children.begin(), parent.num_keys + 1, parent.child_node_ids);
        std::copy_n(parent_keys.begin() + middle + 1, sibling.num_keys, sibling.keys);
        std::copy_n(children.begin() + middle + 1, sibling.num_keys + 1,
                    sibling.child_node_ids);
        writeNode(parent);
        writeNode(sibling);
        separator = parent_keys[middle];
        right_id = sibling.node_id;
    }

    Node left = readNode(ROOT_NODE_ID);
    left.node_id = disk_.allocateBlock();
    writeNode(left);
    Node root{};
    root.init(ROOT_NODE_ID, false);
    root.num_keys = 1;
    root.keys[0] = separator;
    root.child_node_ids[0] = left.node_id;
    root.child_node_ids[1] = right_id;
    writeNode(root);
}

std::vector<RecordPointer> BPlusTree::search(float key) const {
    requireFiniteKey(key);
    return rangeSearch(key, key);
}

std::vector<RecordPointer> BPlusTree::rangeSearch(float lower, float upper,
                                                   bool include_lower,
                                                   bool include_upper) const {
    if (std::isnan(lower) || std::isnan(upper)) {
        throw std::invalid_argument("Range bounds cannot be NaN");
    }

    query_node_accesses_ = 0;
    measuring_query_ = true;
    std::vector<RecordPointer> result;
    if (lower > upper || (lower == upper && (!include_lower || !include_upper))) {
        return result;
    }
    Node leaf = findLeaf(lower, false);
    int position = include_lower ? lowerBound(leaf, lower) : upperBound(leaf, lower);
    while (true) {
        for (int i = position; i < leaf.num_keys; ++i) {
            float key = leaf.keys[i];
            if (key > upper || (!include_upper && key == upper)) {
                measuring_query_ = false;
                return result;
            }
            if (key > lower || (include_lower && key == lower)) {
                result.push_back(leaf.record_pointers[i]);
            }
        }
        if (leaf.next_leaf == INVALID_NODE_ID) {
            measuring_query_ = false;
            return result;
        }
        leaf = readNode(leaf.next_leaf);
        position = 0;
    }
}

float BPlusTree::minimumKey(const Node& node) const {
    Node current = node;
    while (!current.isLeaf()) current = readNode(current.child_node_ids[0]);
    if (current.num_keys == 0) {
        throw std::runtime_error("Empty non-root subtree");
    }
    return current.keys[0];
}

std::size_t BPlusTree::lastQueryNodeAccesses() const {
    return query_node_accesses_;
}

void BPlusTree::resetQueryNodeAccesses() const {
    query_node_accesses_ = 0;
    measuring_query_ = false;
}

void BPlusTree::removeChild(Node& parent, int separator_index) {
    for (int i = separator_index; i < parent.num_keys - 1; ++i) {
        parent.keys[i] = parent.keys[i + 1];
    }
    for (int i = separator_index + 1; i < parent.num_keys; ++i) {
        parent.child_node_ids[i] = parent.child_node_ids[i + 1];
    }
    --parent.num_keys;
}

// Appends right leaf's entries to left leaf, then bypasses right leaf
void BPlusTree::mergeNodes(Node& left, const Node& right, float separator) {
    int offset = left.num_keys;
    if (left.isLeaf()) {
        std::copy_n(right.keys, right.num_keys, left.keys + offset);
        std::copy_n(right.record_pointers, right.num_keys, left.record_pointers + offset);
        left.num_keys += right.num_keys;
        left.next_leaf = right.next_leaf;
    } else {
        left.keys[offset] = separator;
        std::copy_n(right.keys, right.num_keys, left.keys + offset + 1);
        std::copy_n(right.child_node_ids, right.num_keys + 1,
                    left.child_node_ids + offset + 1);
        left.num_keys += right.num_keys + 1;
    }
    writeNode(left);
}

void BPlusTree::rebalanceChild(Node& parent, int child_index) {
    Node child = readNode(parent.child_node_ids[child_index]);
    if (child_index > 0) parent.keys[child_index - 1] = minimumKey(child);
    if (child.num_keys >= minimumKeys(child)) return; // Check for occupancy

    if (child_index > 0) {
        Node left = readNode(parent.child_node_ids[child_index - 1]);
        if (left.num_keys > minimumKeys(left)) { // Try borrowing from left sibling
            for (int i = child.num_keys; i > 0; --i) child.keys[i] = child.keys[i - 1];
            if (child.isLeaf()) {
                for (int i = child.num_keys; i > 0; --i) {
                    child.record_pointers[i] = child.record_pointers[i - 1];
                }
                child.keys[0] = left.keys[left.num_keys - 1];
                child.record_pointers[0] = left.record_pointers[left.num_keys - 1];
                parent.keys[child_index - 1] = child.keys[0];
            } else {
                for (int i = child.num_keys + 1; i > 0; --i) {
                    child.child_node_ids[i] = child.child_node_ids[i - 1];
                }
                child.keys[0] = parent.keys[child_index - 1];
                child.child_node_ids[0] = left.child_node_ids[left.num_keys];
                parent.keys[child_index - 1] = left.keys[left.num_keys - 1];
            }
            --left.num_keys;
            ++child.num_keys;
            writeNode(left);
            writeNode(child);
            return;
        }
    }

    if (child_index < parent.num_keys) {
        Node right = readNode(parent.child_node_ids[child_index + 1]);
        if (right.num_keys > minimumKeys(right)) { // Try borrowing from right sibling
            if (child.isLeaf()) {
                child.keys[child.num_keys] = right.keys[0];
                child.record_pointers[child.num_keys] = right.record_pointers[0];
                for (int i = 0; i < right.num_keys - 1; ++i) {
                    right.record_pointers[i] = right.record_pointers[i + 1];
                }
                parent.keys[child_index] = right.keys[1];
            } else {
                child.keys[child.num_keys] = parent.keys[child_index];
                child.child_node_ids[child.num_keys + 1] = right.child_node_ids[0];
                parent.keys[child_index] = right.keys[0];
                for (int i = 0; i < right.num_keys; ++i) {
                    right.child_node_ids[i] = right.child_node_ids[i + 1];
                }
            }
            for (int i = 0; i < right.num_keys - 1; ++i) right.keys[i] = right.keys[i + 1];
            --right.num_keys;
            ++child.num_keys;
            writeNode(right);
            writeNode(child);
            return;
        }
    }

    if (child_index > 0) {
        Node left = readNode(parent.child_node_ids[child_index - 1]);
        mergeNodes(left, child, parent.keys[child_index - 1]);
        removeChild(parent, child_index - 1);
    } else {
        Node right = readNode(parent.child_node_ids[1]);
        mergeNodes(child, right, parent.keys[0]);
        removeChild(parent, 0);
    }
}

bool BPlusTree::remove(float key, RecordPointer record) {
    requireFiniteKey(key);
    std::vector<PathEntry> path;
    Node leaf = findLeaf(key, false, &path);
    while (true) {
        for (int i = lowerBound(leaf, key); i < leaf.num_keys; ++i) {
            if (leaf.keys[i] != key) return false;
            if (leaf.record_pointers[i].block_id != record.block_id ||
                leaf.record_pointers[i].slot != record.slot) continue;

            for (int j = i; j < leaf.num_keys - 1; ++j) {
                leaf.keys[j] = leaf.keys[j + 1];
                leaf.record_pointers[j] = leaf.record_pointers[j + 1];
            }
            --leaf.num_keys;
            writeNode(leaf);
            while (!path.empty()) {
                PathEntry entry = path.back();
                path.pop_back();
                Node parent = readNode(entry.node_id);
                rebalanceChild(parent, entry.child_index);
                writeNode(parent);
            }
            Node root = readNode(ROOT_NODE_ID);
            if (!root.isLeaf() && root.num_keys == 0) {
                // Promote the remaining child into the fixed root block.
                root = readNode(root.child_node_ids[0]);
                root.node_id = ROOT_NODE_ID;
                writeNode(root);
            }
            return true;
        }
        if (!advanceLeaf(leaf, path)) return false;
    }
}

std::size_t BPlusTree::buildFromData(Disk& data_disk, int data_blocks) {
    if (&data_disk == &disk_) {
        throw std::invalid_argument("Data and index must use separate disk files");
    }
    if (data_blocks < 0 || data_blocks > data_disk.numBlocks()) {
        throw std::invalid_argument("Invalid number of data blocks");
    }
    Node root = readNode(ROOT_NODE_ID);
    if (!root.isLeaf() || root.num_keys != 0) {
        throw std::logic_error("Build requires an empty B+ tree");
    }
    std::size_t entries = 0;
    for (int block_id = 0; block_id < data_blocks; ++block_id) {
        std::array<char, BLOCK_SIZE> buffer{};
        data_disk.readBlock(block_id, buffer.data());
        Block block{};
        std::memcpy(&block, buffer.data(), sizeof(block));
        for (int slot = 0; slot < RECORDS_PER_BLOCK; ++slot) {
            if (block.used[slot]) {
                insert(block.records[slot].fg_pct_home, {block_id, slot});
                ++entries;
            }
        }
    }
    return entries;
}

TreeStats BPlusTree::stats() const {
    TreeStats result;
    // Count all reachable nodes 
    std::vector<std::pair<int32_t, int>> pending{{ROOT_NODE_ID, 1}};
    while (!pending.empty()) {
        auto [id, level] = pending.back();
        pending.pop_back();
        Node node = readNode(id);
        ++result.num_nodes;
        result.num_levels = std::max(result.num_levels, level);
        if (id == ROOT_NODE_ID) {
            result.root_keys.assign(node.keys, node.keys + node.num_keys);
        }
        if (!node.isLeaf()) {
            for (int i = 0; i <= node.num_keys; ++i) {
                pending.emplace_back(node.child_node_ids[i], level + 1);
            }
        }
    }
    return result;
}

bool BPlusTree::validate() const {
    Node root = readNode(ROOT_NODE_ID);

    std::vector<int32_t> current_level;
    current_level.push_back(ROOT_NODE_ID);

    int expected_level = 0;

    while (!current_level.empty()) {
        std::vector<int32_t> next_level;

        for (int32_t node_id : current_level) {
            Node node = readNode(node_id);

            if (node.num_keys < 0 || node.num_keys > BPTREE_N) {
                std::cout << "Invalid key count in node "
                          << node_id << "\n";
                return false;
            }

            for (int i = 1; i < node.num_keys; ++i) {
                if (node.keys[i - 1] > node.keys[i]) {
                    std::cout << "Unsorted keys in node "
                              << node_id << "\n";
                    return false;
                }
            }

            if (!node.isLeaf()) {
                for (int i = 0; i <= node.num_keys; ++i) {
                    if (node.child_node_ids[i] == INVALID_NODE_ID) {
                        std::cout << "Invalid child pointer in node "
                                  << node_id << "\n";
                        return false;
                    }

                    next_level.push_back(node.child_node_ids[i]);
                }
            }
        }

        current_level = next_level;
        ++expected_level;
    }

    std::cout << "B+ tree structural validation: PASSED\n";
    return true;
}