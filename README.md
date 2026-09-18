# SC3020 Project 1

Database storage and B+ tree indexing project.

## `main.cpp`

`main.cpp` is the entry point of the program. It coordinates the three tasks in order.

```text
main.cpp
   │
   ├── Task 1: Load games.txt
   │          ↓
   │      create database.bin
   │
   ├── Task 2: Build B+ Tree
   │          ↓
   │      create index.bin
   │
   └── Task 3: Query / Delete / Benchmark
```

`main.cpp` should mainly **coordinate the components**. The actual storage, B+ tree, query, and benchmarking logic will be implemented in respective modules.

Project Flow

NBA Dataset
│
▼
Task 1: Storage
│
▼
database.bin
│
│ record locations
▼
Task 2: B+ Tree
│
▼
index.bin
│
▼
Task 3: Query / Delete / Benchmark

Task 1 — Storage

Convert the NBA dataset into:

Records → Blocks → database.bin

Record = one NBA dataset row

Block = fixed-size storage unit containing records

database.bin = binary file used as the simulated disk

Storage provides an API for reading/writing records and blocks

A record can be identified by:

(blockId, slotId)

Task 2 — B+ Tree

Build a B+ tree using:

FG_PCT_home

The tree maps keys to record locations:

FG_PCT_home
↓
B+ Tree
↓
(blockId, slotId)
↓
database.bin

The B+ tree is stored in:

index.bin

Task 3 — Query / Delete / Benchmark

Find records where:

FG_PCT_home > 0.5

Then use the B+ tree and storage system to perform the required deletion and measurements.

Also compare the B+ tree approach against a brute-force linear scan.

Project Structure

SC3020-Project1/
├── data/
├── src/
│ ├── storage/
│ ├── bplustree/
│ ├── task3/
│ └── main.cpp
├── storage/
│ ├── database.bin
│ └── index.bin
├── CMakeLists.txt
├── README.md
└── .gitignore
