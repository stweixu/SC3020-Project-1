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
    std::cout << "Loading " << data_path
              << " into " << db_path << " ...\n\n";

    Disk disk(db_path, /*create_new=*/true);
    LoadStats s = loadData(data_path, disk);

    std::printf("%-38s %d bytes\n",
                "Size of a record:", (int)sizeof(Record));
    std::printf("%-38s %d\n",
                "Number of records:", s.records_loaded);
    std::printf("%-38s %d\n",
                "Number of records per block:", RECORDS_PER_BLOCK);
    std::printf("%-38s %d\n",
                "Number of blocks:", s.blocks_used);

    std::printf("\n%-38s %d bytes\n",
                "Block size:", BLOCK_SIZE);
    std::printf("%-38s %d bytes\n",
                "Bytes used by Block struct:", (int)sizeof(Block));
    std::printf("%-38s %lld bytes\n",
                "Database file size:",
                (long long)s.blocks_used * BLOCK_SIZE);
    std::printf("%-38s %d\n",
                "Lines skipped (missing fields):", s.lines_skipped);

    // check: read 3 recors from first block of disk.
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

    std::size_t indexed =
        tree.buildFromData(data_disk, data_disk.numBlocks());

    TreeStats tree_stats = tree.stats();

    std::printf("%-38s %d\n",
                "Parameter n (maximum keys):",
                tree_stats.parameter_n);

    std::printf("%-38s %zu\n",
                "Number of index entries:",
                indexed);

    std::printf("%-38s %d\n",
                "Number of tree nodes:",
                tree_stats.num_nodes);

    std::printf("%-38s %d\n",
                "Number of tree levels:",
                tree_stats.num_levels);

    std::printf("%-38s", "Root keys:");

    for (float key : tree_stats.root_keys) {
        std::printf(" %.3f", key);
    }

    std::printf("\n");
}

static void runTask3(const std::string& db_path,
                     const std::string& index_path) {
    std::cout << "========== Task 3: Delete FG_PCT_home > 0.5 ==========\n";

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

    // For now, just verify that both persistent files can be reopened.
    Disk data_disk(db_path, /*create_new=*/false);
    Disk index_disk(index_path, /*create_new=*/false);

    BPlusTree tree(index_disk, /*create_new=*/false);

    TreeStats tree_stats = tree.stats();

    std::printf("Database blocks: %d\n", data_disk.numBlocks());
    std::printf("Index blocks:    %d\n", index_disk.numBlocks());
    std::printf("Tree nodes:      %d\n", tree_stats.num_nodes);
    std::printf("Tree levels:     %d\n", tree_stats.num_levels);

    std::cout << "\nTask 3 query/deletion not implemented yet.\n";
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2 || std::string(argv[1]) == "--help") {
            std::cout
                << "Usage:\n"
                << "  " << argv[0]
                << " task1 [data/games.txt] [storage/database.bin]\n"
                << "  " << argv[0]
                << " task2 [storage/database.bin] [storage/index.bin]\n"
                << "  " << argv[0]
                << " task3 [storage/database.bin] [storage/index.bin]\n";

            return argc < 2 ? 1 : 0;
        }

        std::string task = argv[1];

        if (task == "task1") {
            std::string data_path =
                (argc > 2) ? argv[2] : "data/games.txt";

            std::string db_path =
                (argc > 3) ? argv[3] : "storage/database.bin";

            runTask1(data_path, db_path);
        }
        else if (task == "task2") {
            std::string db_path =
                (argc > 2) ? argv[2] : "storage/database.bin";

            std::string index_path =
                (argc > 3) ? argv[3] : "storage/index.bin";

            runTask2(db_path, index_path);
        }
        else if (task == "task3") {
            std::string db_path =
                (argc > 2) ? argv[2] : "storage/database.bin";

            std::string index_path =
                (argc > 3) ? argv[3] : "storage/index.bin";

            runTask3(db_path, index_path);
        }
        else {
            throw std::invalid_argument(
                "Unknown task: " + task
            );
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}