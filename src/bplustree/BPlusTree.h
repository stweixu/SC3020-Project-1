#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Node.h"
#include "../storage/Disk.h"

// Task 2 statistics
struct TreeStats {
    int parameter_n = BPTREE_N;
    int num_nodes = 0;
    int num_levels = 0;
    std::vector<float> root_keys;
};

class BPlusTree {
public:
    // Disk object stores index, create new tree or open existing tree
    BPlusTree(Disk& index_disk, bool create_new);
    BPlusTree(const BPlusTree&) = delete;
    BPlusTree& operator=(const BPlusTree&) = delete;

    // Insert index entry
    void insert(float key, RecordPointer record);

    // Search
    std::vector<RecordPointer> search(float key) const;

    std::vector<RecordPointer> rangeSearch(
        float lower,
        float upper,
        bool include_lower = true,
        bool include_upper = true
    ) const;

    // Task 3: query access statistics
    std::size_t lastQueryNodeAccesses() const;
    void resetQueryNodeAccesses() const;

    // Remove index entry
    bool remove(float key, RecordPointer record);

    // Build / statistics
    std::size_t buildFromData(Disk& data_disk, int data_blocks);
    TreeStats stats() const;

    int32_t rootNodeId() const {
        return ROOT_NODE_ID;
    }

private:
    struct PathEntry {
        int32_t node_id;
        int child_index;
    };

    static constexpr int32_t ROOT_NODE_ID = 0;

    Disk& disk_;

    // Task 3 query access counter.
    // mutable because rangeSearch() is const.
    mutable std::size_t query_node_accesses_ = 0;
    mutable bool measuring_query_ = false;

    Node readNode(int32_t id) const;
    void writeNode(const Node& node);
    Node allocateNode(bool leaf);

    Node findLeaf(
        float key,
        bool insert_position,
        std::vector<PathEntry>* path = nullptr
    ) const;

    bool advanceLeaf(
        Node& leaf,
        std::vector<PathEntry>& path
    ) const;

    float minimumKey(const Node& node) const;

    void rebalanceChild(Node& parent, int child_index);

    void mergeNodes(
        Node& left,
        const Node& right,
        float separator
    );

    static void removeChild(
        Node& parent,
        int separator_index
    );
};