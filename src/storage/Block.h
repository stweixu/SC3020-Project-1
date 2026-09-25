#pragma once

struct Block {
    int32_t block_id;                  // position of block in the file
    int32_t num_records;               // number of live (non-deleted) records
    uint8_t used[RECORDS_PER_BLOCK];   // 1 = slot holds a record, 0 = free/deleted
    Record records[RECORDS_PER_BLOCK];
    void init(int id) {
        block_id = id;
        num_records = 0;
        std::memset(used, 0, sizeof(used));
    }
