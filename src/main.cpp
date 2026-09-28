#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "bplustree/BPlusTree.h"
#include "storage/Loader.h"
#include "storage/StorageConfig.h"
#include "storage/Record.h"
#include "storage/Disk.h"
#include "storage/Block.h"
 
static void printRecord(const Record& r) {
    std::printf("  %-10s  team=%d  pts=%d  fg=%.3f  ft=%.3f  fg3=%.3f  ast=%d  reb=%d  win=%d\n",
                r.game_date, r.team_id_home, r.pts_home, r.fg_pct_home, r.ft_pct_home,
                r.fg3_pct_home, r.ast_home, r.reb_home, r.home_team_wins ? 1 : 0);
}
 
int main(int argc, char* argv[]) {
    if (argc > 4 || (argc > 1 && std::string(argv[1]) == "--help")) {
        std::cout << "Usage: " << argv[0]
                  << " [data/games.txt] [storage/database.bin] [storage/index.bin]\n";
        return argc > 4 ? 1 : 0;
    }
    std::string data_path = (argc > 1) ? argv[1] : "data/games.txt";
    std::string db_path = (argc > 2) ? argv[2] : "storage/database.bin";
    std::string index_path = (argc > 3) ? argv[3] : "storage/index.bin";
 
    try {
        const auto same_file = [](const std::string& first, const std::string& second) {
            return std::filesystem::weakly_canonical(first) ==
                       std::filesystem::weakly_canonical(second) ||
                   (std::filesystem::exists(first) && std::filesystem::exists(second) &&
                    std::filesystem::equivalent(first, second));
        };
        if (same_file(data_path, db_path) || same_file(data_path, index_path) ||
            same_file(db_path, index_path)) {
            throw std::invalid_argument("Input, database and index paths must be different");
        }
        std::cout << "========== Task 1: Storage ==========\n";
        std::cout << "Loading " << data_path << " into " << db_path << " ...\n\n";
 
        Disk disk(db_path, /*create_new=*/true);
        LoadStats s = loadData(data_path, disk);
 
        std::printf("%-38s %d bytes\n", "Size of a record:",           (int)sizeof(Record));
        std::printf("%-38s %d\n",       "Number of records:",          s.records_loaded);
        std::printf("%-38s %d\n",       "Number of records per block:", RECORDS_PER_BLOCK);
        std::printf("%-38s %d\n",       "Number of blocks:",           s.blocks_used);
        std::printf("\n%-38s %d bytes\n", "Block size:",               BLOCK_SIZE);
        std::printf("%-38s %d bytes\n",   "Bytes used by Block struct:", (int)sizeof(Block));
        std::printf("%-38s %lld bytes\n", "Database file size:",
                    (long long)s.blocks_used * BLOCK_SIZE);
        std::printf("%-38s %d\n",         "Lines skipped (missing fields):", s.lines_skipped);
 
        // Sanity check: read the first block back from disk and show a few records.
        if (s.blocks_used > 0) {
            Block blk{};
            char buffer[BLOCK_SIZE];
            disk.readBlock(0, buffer);
            std::memcpy(&blk, buffer, sizeof(Block));
            std::cout << "\nFirst 3 records read back from block 0:\n";
            for (int i = 0; i < 3 && i < blk.num_records; i++) printRecord(blk.records[i]);
        }

        std::cout << "\n========== Task 2: B+ Tree Index ==========\n";
        std::cout << "Building FG_PCT_home index in " << index_path << " ...\n\n";
        Disk index_disk(index_path, /*create_new=*/true);
        BPlusTree tree(index_disk, /*create_new=*/true);
        std::size_t indexed = tree.buildFromData(disk, s.blocks_used);
        if (indexed != static_cast<std::size_t>(s.records_loaded)) {
            throw std::runtime_error("Index entry count does not match loaded records");
        }
        TreeStats tree_stats = tree.stats();
        std::printf("%-38s %d\n", "Parameter n (maximum keys):", tree_stats.parameter_n);
        std::printf("%-38s %zu\n", "Number of index entries:", indexed);
        std::printf("%-38s %d\n", "Number of tree nodes:", tree_stats.num_nodes);
        std::printf("%-38s %d\n", "Number of tree levels:", tree_stats.num_levels);
        std::printf("%-38s", "Root keys:");
        for (float key : tree_stats.root_keys) std::printf(" %.3f", key);
        std::printf("\n");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
