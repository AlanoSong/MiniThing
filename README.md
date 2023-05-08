# MiniThing

！[Chinese Version](./README.ch.md)

## 1. Introduction
- The Everything software on Windows is very fast in file location, with the advantage of utilizing NTFS's USN logs and the file monitoring mechanism on Windows
- This project follows a similar principle by querying USN logs, monitoring Windows platform file changes, using SQLite databases to store file nodes, and providing file information query functions
![](./Docs/Pictures/Architecture.png)

## 2. How to use

### 2.1 Compile
- [x] Visual Studio
- [ ] CMake: To be added

### 2.2 Usage
- ***When debugging or using, run it with administrator privileges***
- Now only support command line, after the program starts, enter the volume name to be queried (eg. "F:"）
- Constructe the database for the first time, wait for the database to be built, and then enter the file name to search
![](./Docs/Pictures/Use0.png)
- File changes under volume are also captured by the software
![](./Docs/Pictures/Use1.png)
- QT interface in preparing...
- Regular lookup and other complex functions in preparing...

## 3. Performance parameters
- Performance bottlenecks are mainly concentrated in 2 aspects:
- 1. File node sorting: read file nodes from USN logs and store them in the unordered map. Sorting those file nodes requires recursive functions, so multithreaded acceleration is used here
- 2. SQLite stores data: SQLite uses some insert acceleration methods
- Test conditions: CPU: Intel i5-7200U, memory: 8G, disk: SanDisk SSD
- Measured data: `472,276` file nodes, it takes `23.9014 S` to build a sqlite database (it only takes `0.505526 S` for sorting, and the rest for sqlite inserting), and `0.100215 S` for a single query
- Summary: There is still room for optimization in the SQLite insert process, and the CPU of i5-7200U is also too old

## 4. Participate in contributing
- Fork this repository
- Create a new Feature_xxx branch
- Submit the code
- Create a new pull request