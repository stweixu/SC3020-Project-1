#pragma once

#include <cstddef>

#include "Query.h"
#include "storage/Disk.h"
#include "bplustree/BPlusTree.h"

struct BenchmarkResult {
    double bplus_tree_time_ms = 0.0;
    std::size_t bplus_tree_index_nodes = 0;
    std::size_t bplus_tree_data_blocks = 0;

    double brute_force_time_ms = 0.0;
    std::size_t brute_force_data_blocks = 0;
};

class Benchmark {
public:
    static BenchmarkResult run(
        BPlusTree& tree,
        Disk& data_disk,
        float threshold = 0.5f
    );
};