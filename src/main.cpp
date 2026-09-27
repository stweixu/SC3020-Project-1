#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

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
    std::string data_path = (argc > 1) ? argv[1] : "games.txt";
    std::string db_path   = (argc > 2) ? argv[2] : "data.db";
 
    try {
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
        Block blk;
        char buffer[BLOCK_SIZE];
        disk.readBlock(0, buffer);
        std::memcpy(&blk, buffer, sizeof(Block));
        std::cout << "\nFirst 3 records read back from block 0:\n";
        for (int i = 0; i < 3 && i < blk.num_records; i++) printRecord(blk.records[i]);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
