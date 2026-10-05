#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <limits>

#include "bplustree/BPlusTree.h"
#include "storage/Loader.h"
#include "storage/StorageConfig.h"
#include "storage/Record.h"
#include "storage/Disk.h"
#include "storage/Block.h"
#include "task3/Benchmark.h"
#include "task3/Query.h"

static void printRecord(const Record& r) {
    std::printf(
        "  %-10s  team=%d  pts=%d  fg=%.3f  ft=%.3f  fg3=%.3f  ast=%d  reb=%d  win=%d\n",
        r.game_date,
        r.team_id_home,
        r.pts_home,
        r.fg_pct_home,
        r.ft_pct_home,
        r.fg3_pct_home,
        r.ast_home,
        r.reb_home,
        r.home_team_wins ? 1 : 0
    );
}

static void runTask1(const std::string& data_path,
                     const std::string& db_path) {

    std::cout << "========== Task 1: Storage ==========\n";
    std::cout << "Loading " << data_path << " into " << db_path << " ...\n\n";

    Disk disk(db_path, /*create_new=*/true);
    LoadStats s = loadData(data_path, disk);

    std::printf("%-38s %d bytes\n", "Size of a record:", (int)sizeof(Record));
    std::printf("%-38s %d\n", "Number of records:", s.records_loaded);
    std::printf("%-38s %d\n", "Number of records per block:", RECORDS_PER_BLOCK);
    std::printf("%-38s %d\n", "Number of blocks:", s.blocks_used);
    std::printf("\n%-38s %d bytes\n", "Block size:", BLOCK_SIZE);
    std::printf("%-38s %d bytes\n", "Bytes used by Block struct:", (int)sizeof(Block));
    std::printf("%-38s %lld bytes\n", "Database file size:", (long long)s.blocks_used * BLOCK_SIZE);
    std::printf("%-38s %d\n", "Lines skipped (missing fields):", s.lines_skipped);

    // Check: read 3 records from the first block of the disk.
    if (s.blocks_used > 0) {
        Block blk{};
        char buffer[BLOCK_SIZE];

        disk.readBlock(0, buffer);
        std::memcpy(&blk, buffer, sizeof(Block));

        std::cout << "\nFirst 3 records read back from block 0:\n";

        for (int i = 0; i < 3 && i < blk.num_records; i++) {
            printRecord(blk.records[i]);
        }
    }
}

static void runTask2(const std::string& db_path,
                     const std::string& index_path) {

    std::cout << "========== Task 2: B+ Tree Index ==========\n";

    if (!std::filesystem::exists(db_path)) {
        throw std::runtime_error(
            "Database file does not exist: " + db_path +
            ". Run Task 1 first."
        );
    }

    if (std::filesystem::exists(index_path)) {
        throw std::runtime_error(
            "Index file already exists: " + index_path +
            ". Remove it before rebuilding the index."
        );
    }

    std::cout << "Opening database: " << db_path << "\n";
    std::cout << "Creating index: " << index_path << "\n\n";

    // Open the persistent database created by Task 1.
    Disk data_disk(db_path, /*create_new=*/false);

    // Create a new persistent index.
    Disk index_disk(index_path, /*create_new=*/true);
    BPlusTree tree(index_disk, /*create_new=*/true);

    std::size_t indexed = tree.buildFromData(data_disk, data_disk.numBlocks());
    TreeStats tree_stats = tree.stats();

    std::printf("%-38s %d\n", "Parameter n (maximum keys):", tree_stats.parameter_n);
    std::printf("%-38s %zu\n", "Number of index entries:", indexed);
    std::printf("%-38s %d\n", "Number of tree nodes:", tree_stats.num_nodes);
    std::printf("%-38s %d\n", "Number of tree levels:", tree_stats.num_levels);

    std::printf("%-38s", "Root keys:");

    for (float key : tree_stats.root_keys) {
        std::printf(" %.3f", key);
    }

    std::printf("\n");
}

static void runTask3(const std::string& db_path,
                     const std::string& index_path) {

    std::cout << "========== Task 3: Delete FG_PCT_home > 0.5 ==========\n";

    // Task 3 uses the database created in Task 1 and the B+ tree index created in Task 2.
    if (!std::filesystem::exists(db_path)) {
        throw std::runtime_error(
            "Database file does not exist: " + db_path +
            ". Run Task 1 first."
        );
    }

    if (!std::filesystem::exists(index_path)) {
        throw std::runtime_error(
            "Index file does not exist: " + index_path +
            ". Run Task 2 first."
        );
    }

    std::cout << "Opening database: " << db_path << "\n";
    std::cout << "Opening index: " << index_path << "\n\n";

    // Reopen the persistent database created by Tasks 1 and 2.
    // create_new=false means we work with the existing files.
    Disk data_disk(db_path, /*create_new=*/false);
    Disk index_disk(index_path, /*create_new=*/false);
    BPlusTree tree(index_disk, /*create_new=*/false);

    // Benchmark the retrieval phase before performing any deletion.
    std::cout << "Running Task 3 retrieval benchmark...\n\n";

    BenchmarkResult benchmark = Benchmark::run(tree, data_disk, 0.5f);

    std::printf("========== Retrieval Statistics ==========\n");

    // Number of B+ tree node read access
    std::printf("%-38s %zu\n", "Number of index nodes accessed:", benchmark.bplus_tree_index_nodes);

    // Number of distinct database blocks containing records
    std::printf("%-38s %zu\n", "Number of data blocks accessed:", benchmark.bplus_tree_data_blocks);

    // Time taken to retrieve records using the B+ tree
    std::printf("%-38s %.3f ms\n", "B+ tree retrieval time:", benchmark.bplus_tree_time_ms);

    // Brute-force method
    std::printf("%-38s %zu\n", "Brute-force data blocks accessed:", benchmark.brute_force_data_blocks);

    // Time taken for brute-force method to scan every database block
    std::printf("%-38s %.3f ms\n", "Brute-force running time:", benchmark.brute_force_time_ms);

    // Actual Task 3 deletion.
    //
    // Query::deleteRange():
    // 1. Finds all records where FG_PCT_home > 0.5
    // 2. Removes those records from the data blocks
    // 3. Removes the entries from the B+ tree
    std::cout << "\nDeleting records with FG_PCT_home > 0.5...\n\n";

    QueryStats query_stats = Query::deleteRange(tree, data_disk, 0.5f, std::numeric_limits<float>::max(), false, true);

    std::printf("========== Deletion Statistics ==========\n");

    // Number of matching games that were removed.
    std::printf("%-38s %zu\n", "Number of games deleted:", query_stats.games_deleted);

    // Average FG_PCT_home among the records returned by the query.
    std::printf("%-38s %.6f\n", "Average FG_PCT_home:", query_stats.average_fg_pct);

    // Recalculate the B+ tree statistics after deletion.
    // Deletion may cause nodes to merge and the root to change.
    TreeStats updated_stats = tree.stats();

    std::printf("\n========== Updated B+ Tree ==========\n");

    // Number of B+ tree nodes remaining after deletion.
    std::printf("%-38s %d\n", "Number of tree nodes:", updated_stats.num_nodes);

    // Number of levels remaining in the B+ tree.
    std::printf("%-38s %d\n", "Number of tree levels:", updated_stats.num_levels);

    // Print the keys currently stored in the root node.
    std::printf("%-38s", "Root keys:");

    for (float key : updated_stats.root_keys) {
        std::printf(" %.3f", key);
    }

    std::printf("\n");
}

static void verifyTask3(const std::string& db_path,
                        const std::string& index_path) {

    std::cout << "========== Task 3 Verification ==========\n";

    Disk data_disk(db_path, /*create_new=*/false);
    Disk index_disk(index_path, /*create_new=*/false);
    BPlusTree tree(index_disk, /*create_new=*/false);

    // Check the database directly for any remaining records above 0.5.
    int remaining_records = 0;

    for (int block_id = 0; block_id < data_disk.numBlocks(); block_id++) {
        Block block{};
        char buffer[BLOCK_SIZE];

        data_disk.readBlock(block_id, buffer);
        std::memcpy(&block, buffer, sizeof(Block));

        for (int slot = 0; slot < block.num_records; slot++) {
            if (!block.used[slot]) {
                continue;
            }

            if (block.records[slot].fg_pct_home > 0.5f) {
                remaining_records++;
            }
        }
    }

    // Check the B+ tree for any remaining keys above 0.5.
    std::vector<RecordPointer> remaining_index_entries =
        tree.rangeSearch(0.5f, std::numeric_limits<float>::max(), false, true);

    std::printf("%-38s %d\n", "Records > 0.5 remaining in database:", remaining_records);
    std::printf("%-38s %zu\n", "Index entries > 0.5 remaining:", remaining_index_entries.size());

    if (remaining_records == 0 && remaining_index_entries.empty()) {
        std::cout << "\nTask 3 deletion verification: PASSED\n";
    }
    else {
        std::cout << "\nTask 3 deletion verification: FAILED\n";
    }
    std::cout << "\n";
    tree.validate();
}

int main(int argc, char* argv[]) {

    try {
        if (argc < 2) {
            runTask1(
                "data/games.txt",
                "storage/database.bin"
            );

            runTask2(
                "storage/database.bin",
                "storage/index.bin"
            );

            runTask3(
                "storage/database.bin",
                "storage/index.bin"
            );

            return 0;
        }

        if (std::string(argv[1]) == "--help") {
            std::cout
                << "Usage:\n"
                << "  " << argv[0] << "                 Run all tasks\n"
                << "  " << argv[0] << " task1 [data/games.txt] [storage/database.bin]\n"
                << "  " << argv[0] << " task2 [storage/database.bin] [storage/index.bin]\n"
                << "  " << argv[0] << " task3 [storage/database.bin] [storage/index.bin]\n"
                << "  " << argv[0] << " verify [storage/database.bin] [storage/index.bin]\n";
            return 0;
        }

        std::string task = argv[1];

        if (task == "task1") {
            std::string data_path = (argc > 2) ? argv[2] : "data/games.txt";
            std::string db_path = (argc > 3) ? argv[3] : "storage/database.bin";

            runTask1(data_path, db_path);
        }
        else if (task == "task2") {
            std::string db_path = (argc > 2) ? argv[2] : "storage/database.bin";
            std::string index_path = (argc > 3) ? argv[3] : "storage/index.bin";

            runTask2(db_path, index_path);
        }
        else if (task == "task3") {
            std::string db_path = (argc > 2) ? argv[2] : "storage/database.bin";
            std::string index_path = (argc > 3) ? argv[3] : "storage/index.bin";

            runTask3(db_path, index_path);
        }
        else if (task == "verify") {
            std::string db_path = (argc > 2) ? argv[2] : "storage/database.bin";
            std::string index_path = (argc > 3) ? argv[3] : "storage/index.bin";
            verifyTask3(db_path, index_path);
        }
        else {
            throw std::invalid_argument("Unknown task: " + task);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}