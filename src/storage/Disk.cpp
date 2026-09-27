#include "Disk.h"

#include <fstream>
#include <stdexcept>

Disk::Disk(const std::string& path, bool create_new) {

    if (create_new) {
        // Create or truncate the file.
        std::ofstream(
            path,
            std::ios::binary | std::ios::trunc
        ).close();
    }

    file_.open(
        path,
        std::ios::in |
        std::ios::out |
        std::ios::binary
    );

    if (!file_) {
        throw std::runtime_error(
            "Cannot open disk file: " + path
        );
    }

    file_.seekg(0, std::ios::end);

    num_blocks_ =
        static_cast<int>(file_.tellg() / BLOCK_SIZE);
}

Disk::~Disk() {

    if (file_.is_open()) {
        file_.close();
    }
}

int Disk::allocateBlock() {

    int block_id = num_blocks_;

    // Reserve one empty physical block.
    char zero_block[BLOCK_SIZE] = {0};

    writeBlock(block_id, zero_block);

    return block_id;
}

void Disk::writeBlock(
    int block_id,
    const void* data
) {

    file_.seekp(
        static_cast<std::streamoff>(block_id) * BLOCK_SIZE
    );

    file_.write(
        static_cast<const char*>(data),
        BLOCK_SIZE
    );

    file_.flush();

    if (!file_) {
        throw std::runtime_error("Block write failed");
    }

    if (block_id >= num_blocks_) {
        num_blocks_ = block_id + 1;
    }

    writes_++;
}

void Disk::readBlock(
    int block_id,
    void* data
) {

    if (block_id < 0 || block_id >= num_blocks_) {
        throw std::runtime_error(
            "Block id out of range"
        );
    }

    file_.seekg(
        static_cast<std::streamoff>(block_id) * BLOCK_SIZE
    );

    file_.read(
        static_cast<char*>(data),
        BLOCK_SIZE
    );

    if (!file_) {
        throw std::runtime_error(
            "Block read failed"
        );
    }

    reads_++;
}