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

    int allocateBlock();                                // returns the id of a new, empty block
    void writeBlock(int block_id, const void* data); // writes blk to position block_id
    void readBlock(int block_id, void* data);        // reads block block_id into blk

    int numBlocks() const { return num_blocks_; }

    long blockReads() const { return reads_; }
    long blockWrites() const { return writes_; }

    void resetCounters() { reads_ = writes_ = 0; }

private:
    std::fstream file_;

    int num_blocks_ = 0;
    long reads_ = 0;
    long writes_ = 0;
};