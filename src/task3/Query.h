#pragma once

#include <cstddef>
#include <vector>

#include "bplustree/BPlusTree.h"
#include "storage/Disk.h"

struct QueryMatch {
    float key;
    RecordPointer pointer;
};

struct QueryResult {
    std::vector<QueryMatch> matches;
    double average_fg_pct = 0.0;
};

struct QueryStats {
    std::size_t games_deleted = 0;
    double average_fg_pct = 0.0;
    std::size_t index_nodes_accessed = 0;
    std::size_t data_blocks_accessed = 0;
};

class Query {
public:

    static QueryResult findRange(
        BPlusTree& tree,
        Disk& disk,
        float lower,
        float upper,
        bool lower_inclusive,
        bool upper_inclusive
    );
    
    static QueryStats deleteRange(
        BPlusTree& tree,
        Disk& disk,
        float lower,
        float upper,
        bool lower_inclusive,
        bool upper_inclusive
    );
};