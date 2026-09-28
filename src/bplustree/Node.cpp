#include "Node.h"

#include <algorithm>
#include <cstring>

void Node::init(int32_t id, bool leaf) {
    std::memset(this, 0, sizeof(*this));
    node_id = id;
    is_leaf = leaf ? 1 : 0;
    next_leaf = INVALID_NODE_ID;
    if (leaf) {
        std::fill_n(record_pointers, BPTREE_N, RecordPointer{-1, -1});
    } else {
        std::fill_n(child_node_ids, BPTREE_N + 1, INVALID_NODE_ID);
    }
}

bool Node::isLeaf() const {
    return is_leaf != 0;
}

bool Node::isFull() const {
    return num_keys >= BPTREE_N;
}
