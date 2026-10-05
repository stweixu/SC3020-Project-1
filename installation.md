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
