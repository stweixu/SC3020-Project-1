#pragma once
 
#include <cstdint>
#include <cstring>
#include "record.h"
 
// Size of one disk block (the unit of I/O to the simulated disk file).
constexpr int BLOCK_SIZE = 4096;
 
// Header: block_id (4 bytes) + num_records (4 bytes)
constexpr int BLOCK_HEADER_SIZE = 2 * sizeof(int32_t);
 
// Each slot costs one Record plus one byte in the used[] array.
constexpr int RECORDS_PER_BLOCK =
    (BLOCK_SIZE - BLOCK_HEADER_SIZE) / (sizeof(Record) + 1);
 
struct Block {
    // ---- header ----
    int32_t block_id;                  // position of this block in the file
    int32_t num_records;               // number of live (non-deleted) records
 
    // ---- slot directory ----
    uint8_t used[RECORDS_PER_BLOCK];   // 1 = slot holds a record, 0 = free/deleted
 
    // ---- data ----
    Record records[RECORDS_PER_BLOCK];
 
    void init(int id) {
        block_id = id;
        num_records = 0;
        std::memset(used, 0, sizeof(used));
    }
 
    bool isFull() const { return num_records >= RECORDS_PER_BLOCK; }
 
    // Returns the slot the record was placed in, or -1 if the block is full.
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
 
    // Marks a slot as free (used in Task 3 deletions).
    bool remove(int slot) {
        if (slot < 0 || slot >= RECORDS_PER_BLOCK || !used[slot]) return false;
        used[slot] = 0;
        num_records--;
        return true;
    }
};
 
static_assert(sizeof(Block) <= BLOCK_SIZE, "Block does not fit in BLOCK_SIZE");
 
// Address of a record on disk; this is what the B+ tree leaves will store.
struct RecordPointer {
    int32_t block_id;
    int32_t slot;
};
 
