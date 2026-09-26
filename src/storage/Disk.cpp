#include "storage.h"
#include <cstring>
#include <stdexcept>
 
Disk::Disk(const std::string& path, bool create_new) {
    if (create_new) {
        // Create (or truncate) the file first, then reopen for read + write.
        std::ofstream(path, std::ios::binary | std::ios::trunc).close();
    }
    file_.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_) throw std::runtime_error("Cannot open disk file: " + path);
 
    file_.seekg(0, std::ios::end);
    num_blocks_ = static_cast<int>(file_.tellg() / BLOCK_SIZE);
}
 
Disk::~Disk() {
    if (file_.is_open()) file_.close();
}
 
int Disk::allocateBlock() {
    Block blk;
    blk.init(num_blocks_);
    writeBlock(blk);            // reserve the space on disk
    return blk.block_id;
}
 
void Disk::writeBlock(const Block& blk) {
    // Always write exactly BLOCK_SIZE bytes; unused tail bytes are zero.
    char buf[BLOCK_SIZE] = {0};
    std::memcpy(buf, &blk, sizeof(Block));
 
    file_.seekp(static_cast<std::streamoff>(blk.block_id) * BLOCK_SIZE);
    file_.write(buf, BLOCK_SIZE);
    file_.flush();
    if (!file_) throw std::runtime_error("Block write failed");
 
    if (blk.block_id >= num_blocks_) num_blocks_ = blk.block_id + 1;
    writes_++;
}
 
void Disk::readBlock(int block_id, Block& blk) {
    if (block_id < 0 || block_id >= num_blocks_)
        throw std::runtime_error("Block id out of range");
 
    char buf[BLOCK_SIZE];
    file_.seekg(static_cast<std::streamoff>(block_id) * BLOCK_SIZE);
    file_.read(buf, BLOCK_SIZE);
    if (!file_) throw std::runtime_error("Block read failed");
 
    std::memcpy(&blk, buf, sizeof(Block));
    reads_++;
}
