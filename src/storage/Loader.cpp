
#include "Loader.h"
#include "Record.h"
#include "Block.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

static void writeDataBlock(Disk& disk, const Block& blk) {
    char buffer[BLOCK_SIZE] = {0};

    std::memcpy(
        buffer,
        &blk,
        sizeof(Block)
    );

    disk.writeBlock(
        blk.block_id,
        buffer
    );
}
 
static std::vector<std::string> split(const std::string& line, char delim) {
    std::vector<std::string> fields;
    std::string field;
    std::stringstream ss(line);
    while (std::getline(ss, field, delim)) fields.push_back(field);
    if (!line.empty() && line.back() == delim) fields.push_back("");  // trailing empty field
    return fields;
}
 
static bool toInt(const std::string& s, int& out) {
    if (s.empty()) return false;
    char* end;
    long v = std::strtol(s.c_str(), &end, 10);
    if (*end != '\0') return false;
    out = static_cast<int>(v);
    return true;
}
 
static bool toFloat(const std::string& s, float& out) {
    if (s.empty()) return false;
    char* end;
    out = std::strtof(s.c_str(), &end);
    return *end == '\0';
}
 
bool parseLine(const std::string& raw, Record& r) {
    std::string line = raw;
    if (!line.empty() && line.back() == '\r') line.pop_back();   // Windows line endings
 
    std::vector<std::string> f = split(line, '\t');
    if (f.size() < 9) return false;
 
    std::memset(&r, 0, sizeof(Record));
    if (f[0].empty() || f[0].size() > 10) return false;
    std::strncpy(r.game_date, f[0].c_str(), 10);
    r.game_date[10] = '\0';
 
    int wins;
    bool ok = toInt(f[1], r.team_id_home)  &&
              toInt(f[2], r.pts_home)      &&
              toFloat(f[3], r.fg_pct_home) &&
              toFloat(f[4], r.ft_pct_home) &&
              toFloat(f[5], r.fg3_pct_home)&&
              toInt(f[6], r.ast_home)      &&
              toInt(f[7], r.reb_home)      &&
              toInt(f[8], wins);
    if (!ok) return false;
 
    r.home_team_wins = (wins != 0);
    return true;
}
 
LoadStats loadData(const std::string& data_path, Disk& disk) {
    std::ifstream in(data_path);
    if (!in) throw std::runtime_error("Cannot open data file: " + data_path);
 
    LoadStats stats;
    std::string line;
    std::getline(in, line);                 // skip the header line
 
    Block blk;
    blk.init(0);
 
    while (std::getline(in, line)) {
        if (line.empty() || line == "\r") continue;
 
        Record r;
        if (!parseLine(line, r)) { 
            stats.lines_skipped++; 
            std::cout << "SKIPPED: " << line << '\n';
            continue; }
 
        if (blk.isFull()) {                 // current block full: flush it, start the next
            writeDataBlock(disk, blk);
            blk.init(blk.block_id + 1);
        }
        blk.insert(r);
        stats.records_loaded++;
    }
    if (blk.num_records > 0) writeDataBlock(disk, blk);   // last, partially filled block
 
    stats.blocks_used = disk.numBlocks();
    return stats;
}
 
