#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME_LENGTH 100
#define MAX_FILES 100
#define MAX_DATA_BLOCKS 1000
#define DATA_BLOCK_SIZE 1024

typedef struct {
    char name[MAX_FILENAME_LENGTH];
    int size;
    int permissions;
    int firstDataBlock;
} FileMetadata;

typedef struct {
    FileMetadata files[MAX_FILES];
    int fileCount;
} Directory;

typedef struct {
    int nextDataBlock;
    char data[DATA_BLOCK_SIZE];
} DataBlock;

typedef struct {
    int allocationTable[MAX_DATA_BLOCKS];
} FAT;

Directory rootDirectory;
DataBlock dataBlocks[MAX_DATA_BLOCKS];
FAT fileAllocationTable;

void initializeFileSystem() {
    rootDirectory.fileCount = 0;
    memset(fileAllocationTable.allocationTable, -1, sizeof(fileAllocationTable.allocationTable));
}

void createFile(char *name, int size, int permissions) {
    if (rootDirectory.fileCount >= MAX_FILES) {
        printf("Error: Maximum number of files reached.\n");
        return;
    }

    if (strlen(name) > MAX_FILENAME_LENGTH) {
        printf("Error: File name is too long.\n");
        return;
    }

    if (size <= 0 || size > MAX_DATA_BLOCKS * DATA_BLOCK_SIZE) {
        printf("Error: Invalid file size.\n");
        return;
    }

    FileMetadata file;
    strcpy(file.name, name);
    file.size = size;
    file.permissions = permissions;

    int allocatedBlocks = 0;
    int currentBlock = 0;
    while (allocatedBlocks < size / DATA_BLOCK_SIZE && currentBlock < MAX_DATA_BLOCKS) {
        if (fileAllocationTable.allocationTable[currentBlock] == -1) {
            fileAllocationTable.allocationTable[currentBlock] = file.firstDataBlock + allocatedBlocks;
            allocatedBlocks++;
        }
        currentBlock++;
    }

    // Check if all data blocks are allocated
    if (allocatedBlocks < size / DATA_BLOCK_SIZE) {
        printf("Error: Not enough free data blocks available.\n");
        return;
    }

    // Open file for writing
    FILE *filePtr = fopen(name, "wb");
    if (filePtr == NULL) {
        printf("Error: Failed to create file '%s'.\n", name);
        return;
    }

    // Write file data block by block
    int blockIndex = file.firstDataBlock;
    while (size > 0 && blockIndex != -1) {
        DataBlock *dataBlock = &dataBlocks[blockIndex];
        fwrite(dataBlock->data, sizeof(char), DATA_BLOCK_SIZE, filePtr);
        size -= DATA_BLOCK_SIZE;
        blockIndex = fileAllocationTable.allocationTable[blockIndex];
    }

    // Close the file
    fclose(filePtr);

    rootDirectory.files[rootDirectory.fileCount] = file;
    rootDirectory.fileCount++;

    printf("File '%s' created successfully.\n", name);
}

void listFiles() {
    printf("Files in the root directory:\n");
    for (int i = 0; i < rootDirectory.fileCount; i++) {
        printf("- %s (Size: %d bytes, Permissions: %d)\n", rootDirectory.files[i].name,
               rootDirectory.files[i].size, rootDirectory.files[i].permissions);
    }
}

int main() {
    initializeFileSystem();

    createFile("file1.txt", 2048, 644);
    createFile("file2.txt", 1024, 600);
    createFile("file3.txt", 4096, 777);

    listFiles();

    return 0;
}
