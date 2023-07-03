#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_FILENAME_LENGTH 100
#define MAX_FILES 100
#define MAX_DATA_BLOCKS 1000
#define DATA_BLOCK_SIZE 1024

typedef struct {
    char name[MAX_FILENAME_LENGTH];
    int size;
    int permissions;
    int firstDataBlock;
    pthread_mutex_t lock; 
} FileMetadata;

typedef struct {
    FileMetadata files[MAX_FILES];
    int fileCount;
    pthread_mutex_t lock;
} Directory;

typedef struct {
    int nextDataBlock;
    char data[DATA_BLOCK_SIZE];
    pthread_mutex_t lock;
} DataBlock;

typedef struct {
    int allocationTable[MAX_DATA_BLOCKS];
    pthread_mutex_t lock;
} FAT;

Directory rootDirectory;
DataBlock dataBlocks[MAX_DATA_BLOCKS];
FAT fileAllocationTable;

void initializeFileSystem() {
    rootDirectory.fileCount = 0;
    pthread_mutex_init(&rootDirectory.lock, NULL);
    pthread_mutex_init(&fileAllocationTable.lock, NULL);
    memset(fileAllocationTable.allocationTable, -1, sizeof(fileAllocationTable.allocationTable));
}

void createFile(char *name, int size, int permissions) {
    pthread_mutex_lock(&rootDirectory.lock);

    if (rootDirectory.fileCount >= MAX_FILES) {
        printf("Error: Maximum number of files reached.\n");
        pthread_mutex_unlock(&rootDirectory.lock);
        return;
    }

    if (strlen(name) > MAX_FILENAME_LENGTH) {
        printf("Error: File name is too long.\n");
        pthread_mutex_unlock(&rootDirectory.lock);
        return;
    }

    if (size <= 0 || size > MAX_DATA_BLOCKS * DATA_BLOCK_SIZE) {
        printf("Error: Invalid file size.\n");
        pthread_mutex_unlock(&rootDirectory.lock);
        return;
    }

    int fileIndex = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (rootDirectory.files[i].size == 0) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("Error: Failed to find an available file slot.\n");
        pthread_mutex_unlock(&rootDirectory.lock);
        return;
    }

    FileMetadata *file = &rootDirectory.files[fileIndex];
    strcpy(file->name, name);
    file->size = size;
    file->permissions = permissions;

    pthread_mutex_lock(&fileAllocationTable.lock);

    int allocatedBlocks = 0;
    int currentBlock = 0;
    while (allocatedBlocks < size / DATA_BLOCK_SIZE && currentBlock < MAX_DATA_BLOCKS) {
        if (fileAllocationTable.allocationTable[currentBlock] == -1) {
            fileAllocationTable.allocationTable[currentBlock] = file->firstDataBlock + allocatedBlocks;
            allocatedBlocks++;
        }
        currentBlock++;
    }

    pthread_mutex_unlock(&fileAllocationTable.lock);

    if (allocatedBlocks < size / DATA_BLOCK_SIZE) {
        printf("Error: Not enough free data blocks available.\n");
        pthread_mutex_unlock(&rootDirectory.lock);
        return;
    }

    FILE *filePtr = fopen(name, "wb");
    if (filePtr == NULL) {
        printf("Error: Failed to create file '%s'.\n", name);
        pthread_mutex_unlock(&rootDirectory.lock);
        return;
    }

    int blockIndex = file->firstDataBlock;
    while (size > 0 && blockIndex != -1) {
        DataBlock *dataBlock = &dataBlocks[blockIndex];

        pthread_mutex_lock(&dataBlock->lock);
        fwrite(dataBlock->data, sizeof(char), DATA_BLOCK_SIZE, filePtr);
        pthread_mutex_unlock(&dataBlock->lock);

        size -= DATA_BLOCK_SIZE;
        blockIndex = fileAllocationTable.allocationTable[blockIndex];
    }

    fclose(filePtr);

    pthread_mutex_unlock(&rootDirectory.lock);

    printf("File '%s' created successfully.\n", name);
}

void listFiles() {
    pthread_mutex_lock(&rootDirectory.lock);

    printf("Files in the root directory:\n");
    for (int i = 0; i < rootDirectory.fileCount; i++) {
        FileMetadata *file = &rootDirectory.files[i];
        printf("- %s (Size: %d bytes, Permissions: %d)\n", file->name, file->size, file->permissions);
    }

    pthread_mutex_unlock(&rootDirectory.lock);
}

int main() {
    initializeFileSystem();

    createFile("file1.txt", 2048, 644);
    createFile("file2.txt", 1024, 600);
    createFile("file3.txt", 4096, 777);

    listFiles();

    return 0;
}
