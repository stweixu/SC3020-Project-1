#pragma once

#include <cstdint>
#include "../storage/Block.h"

// Maximum number of keys in a node, N
constexpr int BPTREE_N = 340;
constexpr int32_t INVALID_NODE_ID = -1;

// B+ tree node, which can be internal or leaf
struct Node {

    // Node header
    int32_t node_id;
    int32_t num_keys;
    int32_t is_leaf;    // Identify leaf node or internal node
    int32_t next_leaf;

    // N-Sized Array storing keys FG_PCT_home
    float keys[BPTREE_N];

    // Array storing 2 pointer types depending on is_leaf
    union {

        // Internal node : point to a child node 
        int32_t child_node_ids[BPTREE_N + 1];

        // Leaf node     : point to an actual record in database
        RecordPointer record_pointers[BPTREE_N];
    };

    void init(int32_t id, bool leaf);

    bool isLeaf() const;

    bool isFull() const;
}; 