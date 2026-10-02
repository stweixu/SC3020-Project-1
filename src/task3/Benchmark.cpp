#include "Benchmark.h"

#include <array>
#include <chrono>
#include <cstring>
#include <limits>
#include <set>

BenchmarkResult Benchmark::run(
    BPlusTree& tree,
    Disk& data_disk,
    float threshold
) {
    BenchmarkResult result;

    auto bplus_start = std::chrono::high_resolution_clock::now();

    QueryResult query_result =
        Query::findRange(
            tree,
            data_disk,
            threshold,
            std::numeric_limits<float>::max(),
            false,
            true
        );

    auto bplus_end = std::chrono::high_resolution_clock::now();

    result.bplus_tree_time_ms =
        std::chrono::duration<double, std::milli>(
            bplus_end - bplus_start
        ).count();

    result.bplus_tree_index_nodes =
        tree.lastQueryNodeAccesses();

    std::set<int> bplus_blocks;

    for (const QueryMatch& match : query_result.matches) {
        bplus_blocks.insert(
            match.pointer.block_id
        );
    }

    result.bplus_tree_data_blocks =
        bplus_blocks.size();

    auto brute_force_start =
        std::chrono::high_resolution_clock::now();

    std::size_t matching_records = 0;

    for (int block_id = 0;
         block_id < data_disk.numBlocks();
         ++block_id) {

        std::array<char, BLOCK_SIZE> buffer{};

        data_disk.readBlock(
            block_id,
            buffer.data()
        );

        Block block{};

        std::memcpy(
            &block,
            buffer.data(),
            sizeof(block)
        );

        for (int slot = 0;
             slot < block.num_records;
             ++slot) {

            if (!block.used[slot]) {
                continue;
            }

            if (block.records[slot].fg_pct_home > threshold) {
                ++matching_records;
            }
        }
    }

    auto brute_force_end =
        std::chrono::high_resolution_clock::now();

    result.brute_force_time_ms =
        std::chrono::duration<double, std::milli>(
            brute_force_end - brute_force_start
        ).count();

    result.brute_force_data_blocks =
        data_disk.numBlocks();

    return result;
}