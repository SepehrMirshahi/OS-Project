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

int createFile(char *name, int size, int permissions) {
    if (rootDirectory.fileCount >= MAX_FILES) {
        printf("Error: Maximum number of files reached.\n");
        return -1;
    }

    if (strlen(name) > MAX_FILENAME_LENGTH) {
        printf("Error: File name is too long.\n");
        return -2;
    }

    if (size <= 0 || size > MAX_DATA_BLOCKS * DATA_BLOCK_SIZE) {
        printf("Error: Invalid file size.\n");
        return -3;
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

    if (allocatedBlocks < size / DATA_BLOCK_SIZE) {
        printf("Error: Not enough free data blocks available.\n");
        return -4;
    }

    FILE *filePtr = fopen(name, "wb");
    if (filePtr == NULL) {
        printf("Error: Failed to create file '%s'.\n", name);
        return -5;
    }

    int blockIndex = file.firstDataBlock;
    while (size > 0 && blockIndex != -1) {
        DataBlock *dataBlock = &dataBlocks[blockIndex];
        fwrite(dataBlock->data, sizeof(char), DATA_BLOCK_SIZE, filePtr);
        size -= DATA_BLOCK_SIZE;
        blockIndex = fileAllocationTable.allocationTable[blockIndex];
    }

    fclose(filePtr);

    rootDirectory.files[rootDirectory.fileCount] = file;
    rootDirectory.fileCount++;

    printf("File '%s' created successfully.\n", name);
    return 0;
}

void listFiles() {
    printf("Files in the root directory:\n");
    for (int i = 0; i < rootDirectory.fileCount; i++) {
        printf("- %s (Size: %d bytes, Permissions: %d)\n", rootDirectory.files[i].name,
               rootDirectory.files[i].size, rootDirectory.files[i].permissions);
    }
}

int readFile(char *name) {
    int errorCode = 0;

    int fileIndex = -1;
    for (int i = 0; i < rootDirectory.fileCount; i++) {
        if (strcmp(rootDirectory.files[i].name, name) == 0) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("Error: File '%s' not found.\n", name);
        errorCode = -1;
    } else {
        FileMetadata *file = &rootDirectory.files[fileIndex];

        FILE *filePtr = fopen(name, "rb");
        if (filePtr == NULL) {
            printf("Error: Failed to open file '%s' for reading.\n", name);
            errorCode = -2;
        } else {
            int blockIndex = file->firstDataBlock;
            while (file->size > 0 && blockIndex != -1) {
                DataBlock *dataBlock = &dataBlocks[blockIndex];
                printf("%s", dataBlock->data);
                file->size -= DATA_BLOCK_SIZE;
                blockIndex = fileAllocationTable.allocationTable[blockIndex];
            }

            fclose(filePtr);
        }
    }

    return errorCode;
}

int writeFile(char *name, char *content) {
    int errorCode = 0;

    int fileIndex = -1;
    for (int i = 0; i < rootDirectory.fileCount; i++) {
        if (strcmp(rootDirectory.files[i].name, name) == 0) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("Error: File '%s' not found.\n", name);
        errorCode = -1;
    } else {
        FileMetadata *file = &rootDirectory.files[fileIndex];

        FILE *filePtr = fopen(name, "wb");
        if (filePtr == NULL) {
            printf("Error: Failed to open file '%s' for writing.\n", name);
            errorCode = -2;
        } else {
            int contentLength = strlen(content);
            int blockSize = DATA_BLOCK_SIZE;
            int blockCount = 0;
            int blockIndex = file->firstDataBlock;
            while (blockCount < file->size / blockSize && blockIndex != -1 && contentLength > 0) {
                DataBlock *dataBlock = &dataBlocks[blockIndex];
                strncpy(dataBlock->data, content, blockSize);
                fwrite(dataBlock->data, sizeof(char), blockSize, filePtr);
                content += blockSize;
                contentLength -= blockSize;
                blockCount++;
                blockIndex = fileAllocationTable.allocationTable[blockIndex];
            }

            fclose(filePtr);
        }
    }

    return errorCode;
}

int main() {
    initializeFileSystem();

    int createFileResult = createFile("file1.txt", 2048, 644);
    if (createFileResult != 0) {
        printf("Error: Failed to create file. Error code: %d\n", createFileResult);
        return createFileResult;
    }

    createFileResult = createFile("file2.txt", 1024, 600);
    if (createFileResult != 0) {
        printf("Error: Failed to create file. Error code: %d\n", createFileResult);
        return createFileResult;
    }

    createFileResult = createFile("file3.txt", 4096, 777);
    if (createFileResult != 0) {
        printf("Error: Failed to create file. Error code: %d\n", createFileResult);
        return createFileResult;
    }

    listFiles();

    int readFileResult = readFile("file1.txt");
    if (readFileResult != 0) {
        printf("Error: Failed to read file. Error code: %d\n", readFileResult);
        return readFileResult;
    }

    int writeFileResult = writeFile("file2.txt", "Updated content.");
    if (writeFileResult != 0) {
        printf("Error: Failed to write file. Error code: %d\n", writeFileResult);
        return writeFileResult;
    }

    readFileResult = readFile("file2.txt");
    if (readFileResult != 0) {
        printf("Error: Failed to read file. Error code: %d\n", readFileResult);
        return readFileResult;
    }

    return 0;
}