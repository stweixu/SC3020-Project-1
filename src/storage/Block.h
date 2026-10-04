#pragma once
 
#include <cstdint>
#include <cstring>
#include "Record.h"
#include "StorageConfig.h"
 
// block_id (4 bytes) + num_records (4 bytes)
constexpr int BLOCK_HEADER_SIZE = 2 * sizeof(int);
 
// Each slot costs one Record plus one byte in the used[] array.
constexpr int RECORDS_PER_BLOCK =
    (BLOCK_SIZE - BLOCK_HEADER_SIZE) / (sizeof(Record) + 1);
 
struct Block {
    int block_id; // position of block
    int num_records; // number of live records

    bool used[RECORDS_PER_BLOCK]; // 1 = slot holds a record, 0 = free/deleted
    Record records[RECORDS_PER_BLOCK]; 
 
    void init(int id) {
        block_id = id;
        num_records = 0;
        std::memset(used, 0, sizeof(used));
    }
 
    // True if block is full
    bool isFull() const { return num_records >= RECORDS_PER_BLOCK; }
 
    // Returns the slot the record was placed in, or -1 if the block is full
    int insert(const Record& r) {
        for (int i = 0; i < RECORDS_PER_BLOCK; i++) {
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
        if (slot < 0 || slot >= RECORDS_PER_BLOCK || !used[slot]) return false;
        used[slot] = 0;
        num_records--;
        return true;
    }
};
 
static_assert(sizeof(Block) <= BLOCK_SIZE, "Block does not fit in block size");
 
// Address of a record on disk (B+ tree leaves store RecordPointers)
struct RecordPointer {
    int block_id;
    int slot;
};
