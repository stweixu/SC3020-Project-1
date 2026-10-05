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

Create a `database.bin` binary file that simulates a local disk and stores the NBA dataset in fixed-size blocks.

- **Record** = one NBA dataset row
- **Record size** = number of bytes used to store one record
- **Block** = fixed-size storage unit containing multiple records
- **Block size** = number of bytes in one block
- **Records per block** = how many records fit inside one block
- **database.bin** = binary file containing all blocks

A record is identified by `(blockId, slotId)`.

## Task 2 — B+ Tree

Create a B+ tree using `FG_PCT_home` as the key.

Main operations:

- Insert
- Search
- Delete

The B+ tree maps:

```text
FG_PCT_home → (blockId, slotId)
```

The B+ tree is stored in `index.bin`.

## Task 3 — Query / Delete / Benchmark

Perform search and deletion of records where:

```text
FG_PCT_home > 0.5
```

The B+ tree approach is compared against a brute-force linear scan using:

1. Number of index nodes accessed
2. Number of data blocks accessed
3. Number of games deleted
4. Average `FG_PCT_home` of returned records
5. Retrieval running time
6. Brute-force data blocks accessed
7. Brute-force running time

The B+ tree is then updated after the deletion.

## Installation guide

### Requirements

- C++17 compatible compiler
- CMake 3.10 or later

### Build

From the project root:

```bash
mkdir -p build
cd build
cmake ..
make
```

The executable will be created as:

```text
build/project1
```

### Dataset

Place the NBA dataset file in the location expected by the program:

```text
games.txt
```

The dataset is used by Task 1 to create the binary database file.

## Running the Program

From the project root:

### Run all tasks

```bash
./build/project1
```

This runs:

```text
Task 1 → Task 2 → Task 3
```

and generates:

```text
storage/database.bin
storage/index.bin
```

### Run Task 3

```bash
./build/project1 task3
```

Optional database and index paths can also be provided:

```bash
./build/project1 task3 [database_path] [index_path]
```

Example:

```bash
./build/project1 task3 storage/database.bin storage/index.bin
```

### Verify Task 3

```bash
./build/project1 verify
```

or:

```bash
./build/project1 verify storage/database.bin storage/index.bin
```

The verification checks that no records with `FG_PCT_home > 0.5` remain in either the database or B+ tree.

## Configuration

Important configuration values are defined in the source code.

### B+ Tree Order

The maximum number of keys per B+ tree node is controlled by:

```cpp
BPTREE_N
```

in:

```text
src/bplustree/Node.h
```

Current value:

```cpp
BPTREE_N = 340
```

### Task 3 Threshold

The Task 3 query currently uses:

```text
FG_PCT_home > 0.5
```

The threshold can be changed in the Task 3 query configuration if testing with a different condition.

### Storage Configuration

Storage-related parameters such as block size are defined in:

```text
src/storage/StorageConfig.h
```

## Project Structure

```text
SC3020-Project1/
│
├── CMakeLists.txt
├── README.md
├── games.txt
│
├── src/
│   ├── main.cpp
│   │
│   ├── storage/
│   │   ├── Record.h
│   │   ├── Block.h
│   │   ├── Block.cpp
│   │   ├── Disk.h
│   │   ├── Disk.cpp
│   │   ├── Loader.h
│   │   ├── Loader.cpp
│   │   └── StorageConfig.h
│   │
│   ├── bplustree/
│   │   ├── Node.h
│   │   ├── Node.cpp
│   │   ├── BPlusTree.h
│   │   └── BPlusTree.cpp
│   │
│   └── task3/
│       ├── Query.h
│       ├── Query.cpp
│       ├── Benchmark.h
│       └── Benchmark.cpp
│
├── storage/
│   ├── database.bin
│   └── index.bin
│
└── build/
```

`database.bin` and `index.bin` are generated at runtime and do not need to be included as source files.
