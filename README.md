# SC3020 Project 1

Database storage and B+ tree indexing project.

## `main.cpp`

`main.cpp` is the entry point of the program. It coordinates the three tasks in order.

```text
main.cpp
   │
   ├── Task 1: Read games.txt
   │          ↓
   │      create database.bin
   │
   ├── Task 2: Build B+ Tree
   │          ↓
   │      create index.bin
   │
   └── Task 3: Query / Delete / Benchmark
```

## Task 1 — Storage

Create a database.bin binary file (simulating a local disk) that stores NBA datasets in fixed-size blocks

- **Record** = one NBA dataset row
- **Record size** = number of bytes used to store one record
- **Block** = fixed-size storage unit containing multiple records
- **Block size** = number of bytes in one block
- **Records per block** = how many records fit inside one block
- **database.bin** = binary file containing all blocks

A record will be identified based on (blockId, slotId)

```text
(blockId, slotId)
```

## Task 2 — B+ Tree

Create a B+ tree using FG_PCT_home as the key.

Main operations:

- insert
- search
- delete

B+ tree will map a key → (blockId, slotId)

The B+ tree will be stored in **index.bin** for indexing.

## Task 3 — Query / Delete / Benchmark

Perform search and deletion of records where _FG_PCT_home > 0.5_

Compare the B+ tree approach against a brute-force linear scan via

1. Index nodes accessed
2. Data blocks accessed
3. Number of games deleted
4. Average FG_PCT_home of returned records
5. Retrieval running time

## Project Structure

```text
SC3020-Project1/
├── data/
├── src/
│   ├── storage/
│   ├── bplustree/
│   ├── task3/
│   └── main.cpp
├── storage/
│   ├── database.bin
│   └── index.bin
├── CMakeLists.txt
├── README.md
└── .gitignore
```

`database.bin` and `index.bin` are generated at runtime by the program.
