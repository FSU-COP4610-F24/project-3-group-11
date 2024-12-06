# FAT32 File System Utility

[We have created a shell system where it runs through a FAT file.
Our job is to maninuplate the file and be able to access different things
within this file.]

## Group Members
- **Donald Walton**: djw21c@fsu.edu
- **Isabela Terra**: irt21@fsu.edu
- **James Tanner**: jwt20@fsu.edu
## Division of Labor

### Part 1: Mounting the Image
- **Responsibilities**: [Mounted the image to be able to have access to it the FAT32 file.]
- **Assigned to**: Isabela Terra

### Part 2: Navigation
- **Responsibilities**: [Created ls and cd to navigate through the FAT32 file.]
- **Assigned to**: Isabela Terra

### Part 3: Create
- **Responsibilities**: [Created mkdir and creat functions to creat things within the fat file.]
- **Assigned to**: Doanld Walton

### Part 4: Read
- **Responsibilities**: [Created open close lsof read size lseek to be able to have access to different files and to understand their status.]
- **Assigned to**:Donald Walton

### Part 5: Update
- **Responsibilities**: [Created functions where the user is able to write to a file and rename it.]
- **Assigned to**: James Tanner

### Part 6: Delete
- **Responsibilities**: [Created functions where the user is able to rm a directory and file from the FAT32 image.]
- **Assigned to**: James Tanner

### Extra Credit
- **Responsibilities**:
- **Assigned to**: 

## File Listing
filesys/
│
├── src/
│ ├── filesys.c
│ └── commands.h
│ └── commands.c
│
├── README.md
└── Makefile
one make is ran an obj folder will be created and a bin folder with the filesys executable in it so it can be ran.
## How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C/C++, `rustc` for Rust.
- **Dependencies**: List any libraries or frameworks necessary (rust only).

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```bash
make run
```
This will run the program ...

## Bugs
- **Bug 1**: This is bug 1.
- **Bug 2**: This is bug 2.
- **Bug 3**: This is bug 3.

## Extra Credit
- **Extra Credit 1**: [Extra Credit Option]
- **Extra Credit 2**: [Extra Credit Option]
- **Extra Credit 3**: [Extra Credit Option]

## Considerations
[Description]
