#include "Query.h"

#include <array>
#include <cstring>
#include <limits>
#include <set>
#include <stdexcept>

QueryResult Query::findRange(
    BPlusTree& tree,
    Disk& data_disk,
    float lower,
    float upper,
    bool lower_inclusive,
    bool upper_inclusive
) {
    QueryResult result;

    tree.resetQueryNodeAccesses();

    std::vector<RecordPointer> pointers =
        tree.rangeSearch(
            lower,
            upper,
            lower_inclusive,
            upper_inclusive
        );

    std::set<int> accessed_blocks;

    for (const RecordPointer& pointer : pointers) {
        if (pointer.block_id < 0 ||
            pointer.block_id >= data_disk.numBlocks()) {

            throw std::runtime_error(
                "Invalid data block reference returned by B+ tree"
            );
        }

        accessed_blocks.insert(pointer.block_id);
    }

    double total_fg_pct = 0.0;

    for (int block_id : accessed_blocks) {
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

        for (const RecordPointer& pointer : pointers) {
            if (pointer.block_id != block_id) {
                continue;
            }

            if (pointer.slot < 0 ||
                pointer.slot >= RECORDS_PER_BLOCK ||
                !block.used[pointer.slot]) {

                continue;
            }

            const Record& record =
                block.records[pointer.slot];

            QueryMatch match;

            match.key = record.fg_pct_home;
            match.pointer = pointer;

            result.matches.push_back(match);

            total_fg_pct += record.fg_pct_home;
        }
    }

    if (!result.matches.empty()) {
        result.average_fg_pct =
            total_fg_pct / result.matches.size();
    }

    return result;
}

QueryStats Query::deleteRange(
    BPlusTree& tree,
    Disk& data_disk,
    float lower,
    float upper,
    bool lower_inclusive,
    bool upper_inclusive
) {
    QueryStats stats;

    QueryResult result =
        findRange(
            tree,
            data_disk,
            lower,
            upper,
            lower_inclusive,
            upper_inclusive
        );

    stats.games_deleted =
        result.matches.size();

    stats.average_fg_pct =
        result.average_fg_pct;

    stats.index_nodes_accessed =
        tree.lastQueryNodeAccesses();

    std::set<int> accessed_blocks;

    for (const QueryMatch& match : result.matches) {
        accessed_blocks.insert(
            match.pointer.block_id
        );
    }

    stats.data_blocks_accessed =
        accessed_blocks.size();

    for (int block_id : accessed_blocks) {
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

        for (const QueryMatch& match : result.matches) {
            if (match.pointer.block_id != block_id) {
                continue;
            }

            block.remove(match.pointer.slot);
        }

        std::array<char, BLOCK_SIZE> output{};

        std::memcpy(
            output.data(),
            &block,
            sizeof(block)
        );

        data_disk.writeBlock(
            block_id,
            output.data()
        );
    }

    for (const QueryMatch& match : result.matches) {
        tree.remove(
            match.key,
            match.pointer
        );
    }

    return stats;
}