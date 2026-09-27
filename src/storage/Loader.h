#pragma once
 
#include <string>
#include "Record.h"
#include "Disk.h"
 
struct LoadStats {
    int records_loaded = 0;   // records written to disk
    int lines_skipped  = 0;   // lines with missing / invalid fields
    int blocks_used    = 0;
};
 
// Parses one tab-separated line of games.txt into r. Returns false if the line is incomplete.
bool parseLine(const std::string& line, Record& r);
 
// Reads games.txt and packs the records into blocks on the disk.
LoadStats loadData(const std::string& data_path, Disk& disk);
 
