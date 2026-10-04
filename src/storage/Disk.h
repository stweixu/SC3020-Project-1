#pragma once

#include <fstream>
#include <string>
#include "StorageConfig.h"

// CHANGE: Store both data blocks and B+ tree index blocks
// Simulates a disk using one binary file.
// Block i is stored at byte offset i * BLOCK_SIZE.
// All access goes through readBlock / writeBlock, one whole block at a time.
class Disk {
public:
    // create_new = true wipes any existing file; false opens an existing one.
    Disk(const std::string& path, bool create_new);
    ~Disk();

    int allocateBlock();                             // reserves a new, empty block and returns its id
    void writeBlock(int block_id, const void* data); // writes BLOCK_SIZE bytes from data to block block_id
    void readBlock(int block_id, void* data);        // reads block block_id into data (BLOCK_SIZE bytes)

    int numBlocks() const { return num_blocks_; }    // total blocks in the file (data + index)

    long blockReads() const { return reads_; }       // number of block reads since last reset
    long blockWrites() const { return writes_; }     // number of block writes since last reset

    void resetCounters() { reads_ = writes_ = 0; }   // resets both counters to zero

private:
    std::fstream file_;  // the binary file simulating the disk

    int num_blocks_ = 0; // number of blocks currently in the file
    long reads_ = 0;     // block read counter
    long writes_ = 0;    // block write counter
};