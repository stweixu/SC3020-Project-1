#pragma once
 
#include <cstdint>
#include <cstring>
#include "record.h"
 
// Size of one disk block
constexpr int block_size = 4096;
 
// block_id (4 bytes) + num_records (4 bytes)
constexpr int block_header_size = 2 * sizeof(int);
 
// Each slot costs one Record plus one byte in the used[] array.
constexpr int records_per_block =
    (block_size - block_header_size) / (sizeof(Record) + 1);
 
struct Block {
    int block_id; // position of block
    int num_records; // number of live records

    bool used[records_per_block]; // 1 = slot holds a record, 0 = free/deleted
    Record records[records_per_block]; 
 
    void init(int id) {
        block_id = id;
        num_records = 0;
        std::memset(used, 0, sizeof(used));
    }
 
    // True if block is full
    bool isFull() const { return num_records >= records_per_block; }
 
    // Returns the slot the record was placed in, or -1 if the block is full
    int insert(const Record& r) {
        for (int i = 0; i < records_per_block; i++) {
            if (!used[i]) {
                records[i] = r;
                used[i] = 1;
                num_records++;
                return i;
            }
        }
        return -1;
    }
 
    // Marks slot as free
    bool remove(int slot) {
        if (slot < 0  slot >= records_per_block  !used[slot]) return false;
        used[slot] = 0;
        num_records--;
        return true;
    }
};
 
static_assert(sizeof(Block) <= block_size, "Block does not fit in block size");
 
// Address of a record on disk (B+ tree leaves store RecordPointers)
struct RecordPointer {
    int32_t block_id;
    int32_t slot;
};
