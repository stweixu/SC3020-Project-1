#pragma once

#include "Record.h"

// Size of a disk block in bytes
constexpr int BLOCK_SIZE = 4096;

// block_id (4 bytes) + num_records (4 bytes)
constexpr int BLOCK_HEADER_SIZE = 2 * sizeof(int);
 
// Each slot costs one Record plus one byte in the used[] array.
constexpr int RECORDS_PER_BLOCK =
    (BLOCK_SIZE - BLOCK_HEADER_SIZE) / (sizeof(Record) + 1);