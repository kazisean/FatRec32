#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <openssl/sha.h>

#pragma pack(push, 1)
typedef struct BootEntry
{
  unsigned char BS_jmpBoot[3];    // Assembly instruction to jump to boot code
  unsigned char BS_OEMName[8];    // OEM Name in ASCII
  unsigned short BPB_BytsPerSec;  // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
  unsigned char BPB_SecPerClus;   // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
  unsigned short BPB_RsvdSecCnt;  // Size in sectors of the reserved area
  unsigned char BPB_NumFATs;      // Number of FATs
  unsigned short BPB_RootEntCnt;  // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
  unsigned short BPB_TotSec16;    // 16-bit value of number of sectors in file system
  unsigned char BPB_Media;        // Media type
  unsigned short BPB_FATSz16;     // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
  unsigned short BPB_SecPerTrk;   // Sectors per track of storage device
  unsigned short BPB_NumHeads;    // Number of heads in storage device
  unsigned int BPB_HiddSec;       // Number of sectors before the start of partition
  unsigned int BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
  unsigned int BPB_FATSz32;       // 32-bit size in sectors of one FAT
  unsigned short BPB_ExtFlags;    // A flag for FAT
  unsigned short BPB_FSVer;       // The major and minor version number
  unsigned int BPB_RootClus;      // Cluster where the root directory can be found
  unsigned short BPB_FSInfo;      // Sector where FSINFO structure can be found
  unsigned short BPB_BkBootSec;   // Sector where backup copy of boot sector is located
  unsigned char BPB_Reserved[12]; // Reserved
  unsigned char BS_DrvNum;        // BIOS INT13h drive number
  unsigned char BS_Reserved1;     // Not used
  unsigned char BS_BootSig;       // Extended boot signature to identify if the next three values are valid
  unsigned int BS_VolID;          // Volume serial number
  unsigned char BS_VolLab[11];    // Volume label in ASCII. User defines when creating the file system
  unsigned char BS_FilSysType[8]; // File system type label in ASCII
} BootEntry;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct DirEntry
{
  unsigned char DIR_Name[11];     // File name
  unsigned char DIR_Attr;         // File attributes
  unsigned char DIR_NTRes;        // Reserved
  unsigned char DIR_CrtTimeTenth; // Created time (tenths of second)
  unsigned short DIR_CrtTime;     // Created time (hours, minutes, seconds)
  unsigned short DIR_CrtDate;     // Created day
  unsigned short DIR_LstAccDate;  // Accessed day
  unsigned short DIR_FstClusHI;   // High 2 bytes of the first cluster address
  unsigned short DIR_WrtTime;     // Written time (hours, minutes, seconds
  unsigned short DIR_WrtDate;     // Written day
  unsigned short DIR_FstClusLO;   // Low 2 bytes of the first cluster address
  unsigned int DIR_FileSize;      // File size in bytes. (0 for directories)
} DirEntry;
#pragma pack(pop)

/**
 * Prints usage information to stderr.
 */
void errUse() {
    fprintf(stderr, "Usage: fatrec32 disk <options>\n");
    fprintf(stderr, "  -i                     Print the file system information.\n");
    fprintf(stderr, "  -l                     List the root directory.\n");
    fprintf(stderr, "  -r filename [-s sha1]  Recover a contiguous file.\n");
    fprintf(stderr, "  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
    fprintf(stderr, "  -ra filename           Recover all files with the given name.\n");
    fprintf(stderr, "  -all                   Recover all deleted files.\n");
}


/**
 * Formats and prints a FAT32 file name from its raw directory entry format.
 * 
 * FAT32 stores file names in a special 8.3 format where:
 * - First 8 bytes are the name
 * - Next 3 bytes are the extension
 * - Spaces are used as padding
 * 
 * This function converts this format to a standard filename by:
 * 1. Removing trailing spaces
 * 2. Adding a dot between name and extension
 * 3. Only including printable characters (ASCII >= 0x20)
 * 
 * @param name Pointer to the 11-byte raw filename from the directory entry
 */
void printName(unsigned char *name) {
    char newOut[13];  // max length: 8 chars + dot + 3 chars + null terminator
    int newIndx = 0;

    for (int i = 0; i < 11; i++) {
        if (i == 8 && name[8] != ' ') {
            newOut[newIndx++] = '.';  
            if (name[i] >= 0x20) {    
                newOut[newIndx++] = name[i];
            }
        }
        else if (name[i] == ' ') {
            continue;  
        }
        else if (name[i] >= 0x20) {   
            newOut[newIndx++] = name[i];
        }
    }
    
    newOut[newIndx] = '\0';  

    if (newIndx > 0) {
        printf("%s", newOut);
    }
}

/**
 * Displays FAT32 file system info from the boot sector.
 * 
 * The function uses memory mapping to efficiently read the disk contents
 * and handles various error conditions that might occur during file operations.
 * 
 * @param disk Path to the disk image file to analyze
 * 
 * Error handling:
 * - Exits with status 1 if disk cannot be opened
 * - Exits with status 1 if file status cannot be retrieved
 * - Exits with status 1 if memory mapping fails
 */
void printDriveInfo(char *disk) {
    struct stat sb;  // structure to hold file status
    char *addr;      // memory address of the disk image
    int fd = open(disk, O_RDONLY);  

    if (fd < 1) {
        fprintf(stderr, "Can't access the given disk fd fail\n");
        exit(1);
    }

    if (fstat(fd, &sb) == -1) {
        fprintf(stderr, "Can't access the given disk size \n");
        exit(1);
    }

    addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        exit(1);
    }

    // cast the memory mapped region to a BootEntry structure
    BootEntry *bootEntry = (BootEntry *)addr;

    // show file system info
    printf("Number of FATs = %d\n", bootEntry->BPB_NumFATs);
    printf("Number of bytes per sector = %d\n", bootEntry->BPB_BytsPerSec);
    printf("Number of sectors per cluster = %d\n", bootEntry->BPB_SecPerClus);
    printf("Number of reserved sectors = %d\n", bootEntry->BPB_RsvdSecCnt);

    // clean up 
    munmap(addr, sb.st_size);
    close(fd);
}

/**
 * Lists all entries in the FAT32 root directory.
 * 
 * The function handles:
 * - Regular files and directories
 * - Skips deleted files (0xE5), long file names (0x0F), and system files (0x08)
 * - Counts total number of valid entries
 * 
 * @param disk Path to the disk image file to analyze
 * 
 * Error handling:
 * - Exits with status 1 if disk cannot be opened
 * - Exits with status 1 if file status cannot be retrieved
 * - Exits with status 1 if memory mapping fails
 */
void listRootDir(char *disk) {
    struct stat sb;  
    char *addr;      
    int totalFiles = 0;  // counter for total valid directory entries
    int fd = open(disk, O_RDWR); 

    if (fd < 1) {
        fprintf(stderr, "Can't access the given disk fd fail\n");
        exit(1);
    }

    if (fstat(fd, &sb) == -1) {
        fprintf(stderr, "Can't access the given disk size \n");
        exit(1);
    }

    addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        exit(1);
    }

    // get boot sector info and calculate key offsets
    BootEntry *bootEntry = (BootEntry *)addr;
    
    // calculate FAT and data area offsets
    int fatStart = bootEntry->BPB_RsvdSecCnt * bootEntry->BPB_BytsPerSec;
    int curCluster = bootEntry->BPB_RootClus;  // root directory cluster
    int *fat = (int *)(addr + fatStart);  // FAT table pointer
    int dataSec = (bootEntry->BPB_RsvdSecCnt + (bootEntry->BPB_NumFATs * bootEntry->BPB_FATSz32)) * bootEntry->BPB_BytsPerSec;
    int size = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;  // size of one cluster

    while (curCluster != 0 && curCluster < 0x0FFFFFF8) {
        // calculate offset to current directory cluster
        int rootOffset = ((curCluster - 2) * size) + dataSec;  
        DirEntry *rootDir = (DirEntry *)(addr + rootOffset);
        int maxFile = size / sizeof(DirEntry);  // Number of entries per cluster

        // process each directory entry in the current cluster
        for (int i = 0; i < maxFile; i++) {
            if (rootDir[i].DIR_Name[0] == 0x00) {
                break;  
            }

            // skip deleted files, long file names, and system files
            if (rootDir[i].DIR_Name[0] == 0xE5 || rootDir[i].DIR_Attr == 0x0f || rootDir[i].DIR_Attr == 0x08) {
                continue;
            }

            // print the file/directory name
            printName(rootDir[i].DIR_Name);

            if (rootDir[i].DIR_Attr == 0x10) {  // Directory
                printf("/ (starting cluster = %d)\n", rootDir[i].DIR_FstClusLO);
            } else {  // File
                if (rootDir[i].DIR_FileSize == 0) {
                    printf(" (size = %d)\n", rootDir[i].DIR_FileSize);
                } else {
                    printf(" (size = %d, starting cluster = %d)\n", rootDir[i].DIR_FileSize, rootDir[i].DIR_FstClusLO);
                }
            }

            totalFiles++;
        }
        
        // move to next clstr
        curCluster = fat[curCluster];
    }

    printf("Total number of entries = %d\n", totalFiles);
    
    munmap(addr, sb.st_size);
    close(fd);
}


/**
 * Creates a standard filename string from a FAT32 directory entry name with a custom first character.
 * 
 * Similar to printName(), but:
 * 1. Returns a newly allocated string instead of printing
 * 2. Allows specifying the first character (used for recovered files)
 * 3. Doesn't filter non-printable characters (assumes valid input)
 * 
 * FAT32 name format conversion:
 * - Input:  "FILE    TXT" (11 bytes, space padded)
 * - Output: "F.TXT"       (with F replaced by 'first' parameter)
 * 
 * @param name  Pointer to the 11-byte raw filename from directory entry
 * @param first Character to use as the first character of the filename
 * 
 * @return Dynamically allocated string containing the formatted filename.
 *         Caller is responsible for freeing the memory.
 *         Format: first + up to 7 chars + optional dot + up to 3 char extension
 * TODO : 
 *  [] optimize later
 */
char* getName(unsigned char *name, char first) {
    char *newOut = (char *)malloc(13 * sizeof(char));  // Max: first char + 7 chars + dot + 3 chars + null
    int newIndx = 1;

    newOut[0] = first;  // set the custom first character
    
    for (int i = 1; i < 11; i++) {
        if (i == 8 && name[8] != ' ') {
            newOut[newIndx++] = '.';          
            newOut[newIndx++] = name[i];      
        } else if (name[i] == ' ') {
            continue;                          // skip padding spaces
        } else {
            newOut[newIndx++] = name[i];      
        }
    }
    
    newOut[newIndx] = '\0';                   
    return newOut;
}


/**
 * Computes the SHA-1 hash of a file's contents by following its cluster chain.
 * 
 * This function:
 * 1. Allocates a buffer for the entire file
 * 2. Reads the file's contents cluster by cluster
 * 3. Computes the SHA-1 hash of the complete file
 * 
 * The function handles:
 * - Files spanning multiple clusters
 * - Partial clusters at the end of files
 * - Memory allocation failures
 * 
 * @param addr      Memory-mapped address of the disk image
 * @param bootEntry Pointer to the boot sector structure
 * @param file      Pointer to the directory entry of the file
 * @param fat       Pointer to the File Allocation Table
 * 
 * @return Dynamically allocated buffer containing the 20-byte SHA-1 hash,
 *         or NULL if memory allocation fails.
 *         Caller is responsible for freeing the returned buffer.
 */
unsigned char* computeFileHash(char *addr, BootEntry *bootEntry, DirEntry *file, int *fat) {
    int size = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;  //size of one cluster
    int dataSec = (bootEntry->BPB_RsvdSecCnt + (bootEntry->BPB_NumFATs * bootEntry->BPB_FATSz32)) * bootEntry->BPB_BytsPerSec;  // start of data area
    
    
    unsigned char *buffer = malloc(file->DIR_FileSize);
    if (!buffer) return NULL;
    
    int curCluster = file->DIR_FstClusLO; 
    unsigned int bytesRead = 0;
    
    while (curCluster != 0 && curCluster < 0x0FFFFFF8 && bytesRead < (unsigned int)file->DIR_FileSize) {
        int clusterOffset = dataSec + ((curCluster - 2) * size);  // Calculate cluster location
        int bytesToRead = size;
        
        if (bytesRead + (unsigned int)bytesToRead > (unsigned int)file->DIR_FileSize) {
            bytesToRead = file->DIR_FileSize - bytesRead;
        }
        
        memcpy(buffer + bytesRead, addr + clusterOffset, bytesToRead);
        bytesRead += bytesToRead;
        curCluster = fat[curCluster];  
    }
    
    // Allocate buffer for SHA-1 hash
    unsigned char *hash = malloc(SHA_DIGEST_LENGTH);
    if (!hash) {
        free(buffer);
        return NULL;
    }
    
    // Compute SHA-1 hash of file contents
    SHA1(buffer, file->DIR_FileSize, hash);
    free(buffer);
    return hash;
}


/**
 * Converts a hexadecimal string to its binary representation.
 * 
 * Specifically designed for converting a 40-character SHA-1 hash string
 * into its 20-byte binary form. Each pair of hex characters is converted
 * to a single byte.
 * 
 * @param hex   Input string of 40 hexadecimal characters
 * @param bytes Output buffer to store the 20-byte binary result
 */
void hexStringToBytes(const char *hex, unsigned char *bytes) {
    for (int i = 0; i < 40; i += 2) {
        sscanf(hex + i, "%2hhx", &bytes[i/2]);  // Convert each hex pair to a byte
    }
}


/**
 * Checks if a cluster in the FAT is marked as free.
 * 
 * In FAT32, a cluster entry of 0 indicates that the cluster is free
 * and available for use.
 * 
 * @param fat     Pointer to the File Allocation Table
 * @param cluster Cluster number to check
 * @return 1 if the cluster is free, 0 otherwise
 */
int isClusterFree(int *fat, int cluster) {
    return fat[cluster] == 0;
}


/**
 * Finds the next free cluster in the FAT starting from a given cluster.
 * 
 * Used during file recovery to find available clusters for reconstructing
 * file data. Searches sequentially through the FAT until a free cluster
 * is found or the maximum cluster number is reached.
 * 
 * @param fat          Pointer to the File Allocation Table
 * @param startCluster First cluster number to check
 * @param maxCluster   Maximum cluster number to check
 * @return The first free cluster number found, or -1 if none available
 */
int getNextFreeCluster(int *fat, int startCluster, int maxCluster) {
    for (int i = startCluster; i < maxCluster; i++) {
        if (isClusterFree(fat, i)) {
            return i;
        }
    }
    return -1; 
}


/**
 * Tests a specific arrangement of clusters to see if they form the desired file.
 * 
 * This function:
 * 1. Reads data from the specified clusters in order
 * 2. Computes the SHA-1 hash of the assembled data
 * 3. Compares it with the target hash
 * 4. If matched, updates both FAT copies with the cluster chain
 * 
 * Used in non-contiguous file recovery to try different cluster combinations
 * until finding one that matches the known file hash.
 * 
 * @param addr        Memory-mapped address of the disk image
 * @param bootEntry   Pointer to the boot sector structure
 * @param file        Pointer to the directory entry of the file
 * @param fat         Pointer to the first FAT
 * @param fat2        Pointer to the second FAT
 * @param clusters    Array of cluster numbers to try in this order
 * @param numClusters Number of clusters in the array
 * @param targetHash  The expected SHA-1 hash of the correct file contents
 * 
 * @return 1 if this permutation matches the target hash, 0 otherwise
 */
int tryClusterPermutation(char *addr, BootEntry *bootEntry, DirEntry *file, int *fat, int *fat2, 
                         int *clusters, int numClusters, unsigned char *targetHash) {
    int size = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus; 
    int dataSec = (bootEntry->BPB_RsvdSecCnt + (bootEntry->BPB_NumFATs * bootEntry->BPB_FATSz32)) * bootEntry->BPB_BytsPerSec;  
    
    unsigned char *buffer = malloc(file->DIR_FileSize);
    if (!buffer) return 0;
    
    unsigned int bytesRead = 0;
    
    // read data from each cluster
    for (int i = 0; i < numClusters && bytesRead < (unsigned int)file->DIR_FileSize; i++) {
        int clusterOffset = dataSec + ((clusters[i] - 2) * size); 
        int bytesToRead = size;
        
        // handle partial cluster at end of file
        if (bytesRead + (unsigned int)bytesToRead > (unsigned int)file->DIR_FileSize) {
            bytesToRead = file->DIR_FileSize - bytesRead;
        }
        memcpy(buffer + bytesRead, addr + clusterOffset, bytesToRead);
        bytesRead += bytesToRead;
    }
    
    // compute and compare hash
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(buffer, file->DIR_FileSize, hash);
    free(buffer);
    
    // if hash matches, update FAT entries to link the clusters
    if (memcmp(hash, targetHash, SHA_DIGEST_LENGTH) == 0) {
        for (int i = 0; i < numClusters - 1; i++) {
            fat[clusters[i]] = clusters[i + 1];     
            fat2[clusters[i]] = clusters[i + 1];
        }
        fat[clusters[numClusters - 1]] = 0x0FFFFFF8;   
        fat2[clusters[numClusters - 1]] = 0x0FFFFFF8; 
        return 1;
    }
    
    return 0;
}

/**
 * Generates the next lexicographically greater permutation of an integer array.
 * 
 * Algorithm steps:
 * 1. Find the largest index i such that arr[i-1] < arr[i]
 * 2. Find the largest index j such that arr[j] > arr[i-1]
 * 3. Swap arr[i-1] and arr[j]
 * 4. Reverse the sequence from arr[i] to arr[n-1]
 * 
 * Used in non-contiguous file recovery to systematically try all possible
 * cluster arrangements until finding one that matches the target hash.
 * 
 * @param arr Array to permute in place
 * @param n   Length of the array
 * @return    1 if a next permutation exists, 0 if this is the last permutation
 */
int next_permutation(int *arr, int n) {
    // Find longest non-increasing suffix
    int i = n - 1;
    while (i > 0 && arr[i - 1] >= arr[i]) i--;
    if (i <= 0) return 0;  
    
    // find successor to pivot
    int j = n - 1;
    while (arr[j] <= arr[i - 1]) j--;
    
    // swap pivot with successor
    int temp = arr[i - 1];
    arr[i - 1] = arr[j];
    arr[j] = temp;
    
    // reverse suffix
    j = n - 1;
    while (i < j) {
        temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
        i++;
        j--;
    }
    
    return 1;  
}

/**
 * attempts all possible permutations of free clusters to find a match for a non-contiguous file.
 * 
 * this function implements a brute-force approach to recover non-contiguous files by:
 * 1. calculating how many clusters are needed based on file size
 * 2. finding that many free clusters in the fat
 * 3. trying all possible orderings of those clusters until finding one that produces
 *    the correct file content (verified by sha-1 hash)
 * 
 * limitations:
 * - only attempts recovery for files requiring 5 or fewer clusters
 * - assumes clusters are relatively close together (starts search from cluster 2)
 * 
 * @param addr        memory-mapped address of the disk image
 * @param bootEntry   pointer to the boot sector structure
 * @param file        pointer to the directory entry of the file to recover
 * @param fat         pointer to the first fat
 * @param fat2        pointer to the second fat (backup)
 * @param targetHash  the expected sha-1 hash of the correct file contents
 * 
 * @return 1 if a valid cluster permutation was found and fats were updated,
 *         0 if no valid permutation was found or if an error occurred
 */
int tryAllPermutations(char *addr, BootEntry *bootEntry, DirEntry *file, int *fat, int *fat2, 
                      unsigned char *targetHash) {
    // calculate number of clusters needed for the file
    int size = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;
    int numClusters = (file->DIR_FileSize - 1) / size + 1;
    if (numClusters > 5) return 0;  
    
    int *clusters = malloc(numClusters * sizeof(int));
    if (!clusters) return 0;
    
    int curCluster = 2;  // start from cluster 2 (first data cluster)
    for (int i = 0; i < numClusters; i++) {
        curCluster = getNextFreeCluster(fat, curCluster, 20);  // look free until cluster 20
        if (curCluster == -1) {
            free(clusters);
            return 0;  
        }
        clusters[i] = curCluster;
        curCluster++;
    }
    
    int found = 0;
    do {
        if (tryClusterPermutation(addr, bootEntry, file, fat, fat2, clusters, numClusters, targetHash)) {
            found = 1;
            break;
        }
    } while (next_permutation(clusters, numClusters));
    
    free(clusters);
    return found;
}


/**
 * Recovers a deleted file by restoring its directory entry and FAT chain.
 * 
 * This function handles the core file recovery process by:
 * 1. Restoring the first character of the filename (which was marked as deleted)
 * 2. Updating the FAT entries to properly chain the file's clusters
 * 
 * The function handles two cases:
 * - Small files (≤ 1 cluster): Simply marks the cluster as end-of-chain
 * - Larger files: Additional recovery logic (implementation incomplete in snippet)
 * 
 * @param recFile    Pointer to the directory entry of the file to recover
 * @param name       The original filename to restore (first character used)
 * @param addr       Memory-mapped address of the disk image
 * @param bootEntry  Pointer to the boot sector structure
 * @param hash       SHA-1 hash of the file's contents (for verification)
 */
void recover(DirEntry *recFile, char *name, char *addr, BootEntry *bootEntry, char *hash) {
    // Restore first character of filename from deleted state (0xE5)
    recFile->DIR_Name[0] = name[0];
    
    // Calculate cluster size and get file size
    int size = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;
    int fileSize = recFile->DIR_FileSize;
    
    // Get pointers to both FAT tables
    int fatStart = bootEntry->BPB_RsvdSecCnt * bootEntry->BPB_BytsPerSec;
    int fat2Start = fatStart + (bootEntry->BPB_FATSz32 * bootEntry->BPB_BytsPerSec);
    int *fat = (int *)(addr + fatStart);
    int *fat2 = (int *)(addr + fat2Start);

    // Handle single-cluster files
    if (fileSize <= size) {
        fat[recFile->DIR_FstClusLO] = 0x0FFFFFF8;    // Mark as end of chain
        fat2[recFile->DIR_FstClusLO] = 0x0FFFFFF8;   // Update backup FAT
        return;
    }

    // Handle multi-cluster files
    int oldCount = (fileSize - 1) / size + 1;        // Calculate number of clusters needed
    int curCluster = recFile->DIR_FstClusLO;         // Start with first cluster
    int recovered = 0;

    // Reconstruct FAT chain for multi-cluster files
    while (recovered < oldCount) {
        if (recovered + 1 == oldCount) {
            fat[curCluster] = 0x0FFFFFF8;            // Mark last cluster as end of chain
            fat2[curCluster] = 0x0FFFFFF8;           // Update backup FAT
            break;
        }
        
        fat[curCluster] = curCluster + 1;            // Link to next cluster
        fat2[curCluster] = curCluster + 1;           // Update backup FAT
        curCluster++;
        recovered++;
    }
}

/**
 * Validates that a string represents a valid SHA-1 hash.
 * 
 * A valid SHA-1 hash must:
 * 1. Be exactly 40 characters long (20 bytes in hex representation)
 * 2. Contain only hexadecimal characters (0-9, a-f, A-F)
 * 
 * @param hash String to validate as a SHA-1 hash
 * @return 1 if the string is a valid SHA-1 hash, 0 otherwise
 */
int isValidHash(const char *hash) {
    if (strlen(hash) != 40) return 0;
    for (int i = 0; i < 40; i++) {
        if (!isxdigit(hash[i])) return 0;
    }
    return 1;
}


/**
 * Recovers a deleted file from the FAT32 file system.
 * 
 * This function implements the core file recovery logic by:
 * 1. Validating input parameters and file system access
 * 2. Searching the root directory for deleted files matching the target name
 * 3. If a SHA-1 hash is provided, verifying file contents match
 * 4. Recovering the file by restoring its directory entry and FAT chain
 * 
 * The function handles both contiguous and non-contiguous file recovery:
 * - Contiguous files: Direct recovery of sequential clusters
 * - Non-contiguous files: Uses permutation testing to find correct cluster order
 * 
 * @param name           Name of the file to recover
 * @param disk          Path to the disk image file
 * @param hash          Optional SHA-1 hash to verify file contents
 * @param isNonContiguous Flag indicating if file may be non-contiguous
 * 
 * Error handling:
 * - Exits with status 1 if disk cannot be opened
 * - Exits with status 1 if file status cannot be retrieved
 * - Exits with status 1 if memory mapping fails
 * - Exits with status 1 if filename is invalid
 * - Exits with status 1 if SHA-1 hash format is invalid
 */
void recFile(char *name, char *disk, char *hash, int isNonContiguous) {
    // Validate input parameters
    if (name == NULL || name[0] == '\0' || name[0] == ' ') {
        fprintf(stderr, "Read the doc! Cant have empty file name\n");
        exit(1);
    }

    if (hash != NULL && !isValidHash(hash)) {
        fprintf(stderr, "Invalid SHA-1 hash format. Must be 40 hexadecimal characters.\n");
        exit(1);
    }

    // Open and map the disk image
    struct stat sb;
    char *addr;
    int fd = open(disk, O_RDWR);

    if (fd < 1) {
        fprintf(stderr, "Can't access the given disk fd fail\n");
        exit(1);
    }

    if (fstat(fd, &sb) == -1) {
        fprintf(stderr, "Can't access the given disk size \n");
        exit(1);
    }

    addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        exit(1);
    }

    // Get pointers to FAT tables and calculate key offsets
    BootEntry *bootEntry = (BootEntry *)addr;
    int fatStart = bootEntry->BPB_RsvdSecCnt * bootEntry->BPB_BytsPerSec;
    int curCluster = bootEntry->BPB_RootClus;
    int *fat = (int *)(addr + fatStart);
    int fat2Start = fatStart + (bootEntry->BPB_FATSz32 * bootEntry->BPB_BytsPerSec);
    int *fat2 = (int *)(addr + fat2Start);
    int dataSec = (bootEntry->BPB_RsvdSecCnt + (bootEntry->BPB_NumFATs * bootEntry->BPB_FATSz32)) * bootEntry->BPB_BytsPerSec;
    int size = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;

    // Variables to track file search results
    int found = 0;              // Found without hash verification
    int foundAmount = 0;        // Number of files found without hash
    int shaFound = 0;           // Found with hash verification
    int indxFoundAt = 0;        // Index of last found file
    int hashMatchCount = 0;     // Number of files matching hash
    DirEntry *firstMatch = NULL; // First matching file entry
    int firstMatchIndex = -1;   // Index of first matching file

    // Search through root directory clusters
    while (curCluster != 0 && curCluster < 0x0FFFFFF8) {
        int rootOffset = dataSec + ((curCluster - 2) * size);
        DirEntry *rootDir = (DirEntry *)(addr + rootOffset);
        int maxFile = size / sizeof(DirEntry);

        // Process each directory entry
        for (int i = 0; i < maxFile + 1; i++) {
            if (rootDir[i].DIR_Name[0] == 0x00) {
                break;  // End of directory entries
            }

            // Skip special entries (directories, long names, system files)
            if (rootDir[i].DIR_Name[0] == 0x10 || rootDir[i].DIR_Attr == 0x0f || rootDir[i].DIR_Attr == 0x08) {
                continue;
            }

            // Check for deleted files (0xE5)
            if (rootDir[i].DIR_Name[0] == 0xE5) {
                char *newName = getName(rootDir[i].DIR_Name, name[0]);

                if (strcmp(newName, name) == 0) {
                    if (hash != NULL) {
                        // Convert hash string to bytes for comparison
                        unsigned char targetHash[SHA_DIGEST_LENGTH];
                        hexStringToBytes(hash, targetHash);
                        
                        if (isNonContiguous) {
                            // Try non-contiguous recovery with permutations
                            if (tryAllPermutations(addr, bootEntry, &rootDir[i], fat, fat2, targetHash)) {
                                shaFound = 1;
                                indxFoundAt = i;
                                hashMatchCount++;
                                if (firstMatch == NULL) {
                                    firstMatch = &rootDir[i];
                                    firstMatchIndex = i;
                                }
                            }
                        } else {
                            // Verify hash for contiguous files
                            unsigned char *fileHash = computeFileHash(addr, bootEntry, &rootDir[i], fat);
                            if (fileHash) {
                                if (memcmp(fileHash, targetHash, SHA_DIGEST_LENGTH) == 0) {
                                    shaFound = 1;
                                    indxFoundAt = i;
                                    hashMatchCount++;
                                    if (firstMatch == NULL) {
                                        firstMatch = &rootDir[i];
                                        firstMatchIndex = i;
                                    }
                                }
                                free(fileHash);
                            }
                        }
                    } else {
                        // No hash provided, just match name
                        found = 1;  
                        indxFoundAt = i;
                        foundAmount++;
                        if (firstMatch == NULL) {
                            firstMatch = &rootDir[i];
                            firstMatchIndex = i;
                        }
                    }
                }

                free(newName);
            }
        }

        // Recover file if we found exactly one match
        if ((found == 1 && foundAmount == 1) || (shaFound == 1 && hashMatchCount == 1)) {
            recover(&rootDir[indxFoundAt], name, addr, bootEntry, hash);
            if (shaFound) {
                printf("%s: successfully recovered with SHA-1\n", name);
            } else {
                printf("%s: successfully recovered\n", name);
            }
            break;
        }

        curCluster = fat[curCluster];
    }

    // Handle cases where we found multiple matches or no matches
    if (firstMatch != NULL && !shaFound && !found) {
        recover(firstMatch, name, addr, bootEntry, hash);
        printf("%s: successfully recovered\n", name);
    } else if (firstMatch == NULL) {
        printf("%s: file not found\n", name);
    }
    
    if (foundAmount > 1 || hashMatchCount > 1) {
        printf("%s: multiple candidates found\n", name);
    }

    // Clean up
    munmap(addr, sb.st_size);
    close(fd);
}


/**
 * Recovers all deleted files with a given name from the FAT32 file system.
 * 
 * This function implements a two-pass recovery strategy:
 * 1. First pass: Scans the root directory to find all deleted files matching
 *    the target name, storing their locations
 * 2. Second pass: Recovers each found file by restoring its directory entry
 *    and FAT chain
 * 
 * The function handles:
 * - Multiple files with the same name
 * - Memory allocation for tracking found files
 * - Proper cleanup of allocated resources
 * 
 * @param name Name of the files to recover
 * @param disk Path to the disk image file
 * 
 * Error handling:
 * - Exits with status 1 if disk cannot be opened
 * - Exits with status 1 if file status cannot be retrieved
 * - Exits with status 1 if memory mapping fails
 * - Exits with status 1 if filename is invalid
 */
void recoverAllFiles(char *name, char *disk) {
    // Validate input filename
    if (name == NULL || name[0] == '\0' || name[0] == ' ') {
        fprintf(stderr, "Read the doc! Cant have empty file name\n");
        exit(1);
    }

    // Open and map the disk image
    struct stat sb;
    char *addr;
    int fd = open(disk, O_RDWR);

    if (fd < 1) {
        fprintf(stderr, "Can't access the given disk fd fail\n");
        exit(1);
    }

    if (fstat(fd, &sb) == -1) {
        fprintf(stderr, "Can't access the given disk size \n");
        exit(1);
    }

    addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        exit(1);
    }

    // Get pointers to FAT tables and calculate key offsets
    BootEntry *bootEntry = (BootEntry *)addr;
    int fatStart = bootEntry->BPB_RsvdSecCnt * bootEntry->BPB_BytsPerSec;
    int curCluster = bootEntry->BPB_RootClus;
    int *fat = (int *)(addr + fatStart);
    int fat2Start = fatStart + (bootEntry->BPB_FATSz32 * bootEntry->BPB_BytsPerSec);
    int *fat2 = (int *)(addr + fat2Start);
    int dataSec = (bootEntry->BPB_RsvdSecCnt + (bootEntry->BPB_NumFATs * bootEntry->BPB_FATSz32)) * bootEntry->BPB_BytsPerSec;
    int size = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;

    // Arrays to track found files and their locations
    int foundCount = 0;
    DirEntry **foundFiles = NULL;    // Array of pointers to found file entries
    int *foundIndices = NULL;        // Array of indices where files were found
    DirEntry **foundDirs = NULL;     // Array of pointers to directory entries

    // First pass - find all matching files
    while (curCluster != 0 && curCluster < 0x0FFFFFF8) {
        int rootOffset = dataSec + ((curCluster - 2) * size);
        DirEntry *rootDir = (DirEntry *)(addr + rootOffset);
        int maxFile = size / sizeof(DirEntry);

        // Process each directory entry
        for (int i = 0; i < maxFile; i++) {
            if (rootDir[i].DIR_Name[0] == 0x00) {
                break;  // End of directory entries
            }

            // Skip special entries (directories, long names, system files)
            if (rootDir[i].DIR_Name[0] == 0x10 || rootDir[i].DIR_Attr == 0x0f || rootDir[i].DIR_Attr == 0x08) {
                continue;
            }

            // Check for deleted files (0xE5)
            if (rootDir[i].DIR_Name[0] == 0xE5) {
                char *newName = getName(rootDir[i].DIR_Name, name[0]);

                if (strcmp(newName, name) == 0) {
                    // Found a match - add to tracking arrays
                    foundCount++;
                    foundFiles = realloc(foundFiles, foundCount * sizeof(DirEntry *));
                    foundIndices = realloc(foundIndices, foundCount * sizeof(int));
                    foundDirs = realloc(foundDirs, foundCount * sizeof(DirEntry *));
                    
                    foundFiles[foundCount-1] = &rootDir[i];
                    foundIndices[foundCount-1] = i;
                    foundDirs[foundCount-1] = rootDir;
                }

                free(newName);
            }
        }

        curCluster = fat[curCluster];
    }

    // Second pass - recover all found files
    if (foundCount == 0) {
        printf("%s: file not found\n", name);
    } else {
        printf("%s: %d file(s) recovered\n", name, foundCount);
        
        // Recover each found file
        for (int i = 0; i < foundCount; i++) {
            recover(foundFiles[i], name, addr, bootEntry, NULL);
        }
    }

    // Clean up allocated memory
    free(foundFiles);
    free(foundIndices);
    free(foundDirs);
    
    // Clean up disk mapping
    munmap(addr, sb.st_size);
    close(fd);
}


/**
 * Recovers all deleted files from the FAT32 file system.
 * 
 * This function implements a comprehensive recovery strategy by:
 * 1. Scanning the entire root directory for deleted files
 * 2. Filtering out special entries (long filenames, system files, directories)
 * 3. Recovering each valid deleted file by:
 *    - Restoring the first character of the filename (using '_' as default)
 *    - Reconstructing the FAT chain based on file size
 *    - Updating both FAT copies for redundancy
 * 
 * The function handles:
 * - Files of any size (single or multiple clusters)
 * - Proper FAT chain reconstruction
 * - Maintaining file system consistency
 * 
 * @param disk Path to the disk image file
 * 
 * Error handling:
 * - Exits with status 1 if disk cannot be opened
 * - Exits with status 1 if file status cannot be retrieved
 * - Exits with status 1 if memory mapping fails
 */
void recoverAllDeleted(char *disk) {
    // Open and map the disk image
    struct stat sb;
    char *addr;
    int fd = open(disk, O_RDWR);

    if (fd < 1) {
        fprintf(stderr, "Can't access the given disk fd fail\n");
        exit(1);
    }

    if (fstat(fd, &sb) == -1) {
        fprintf(stderr, "Can't access the given disk size \n");
        exit(1);
    }

    addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        exit(1);
    }

    // Get pointers to FAT tables and calculate key offsets
    BootEntry *bootEntry = (BootEntry *)addr;
    int fatStart = bootEntry->BPB_RsvdSecCnt * bootEntry->BPB_BytsPerSec;
    int curCluster = bootEntry->BPB_RootClus;
    int *fat = (int *)(addr + fatStart);
    int fat2Start = fatStart + (bootEntry->BPB_FATSz32 * bootEntry->BPB_BytsPerSec);
    int *fat2 = (int *)(addr + fat2Start);
    int dataSec = (bootEntry->BPB_RsvdSecCnt + (bootEntry->BPB_NumFATs * bootEntry->BPB_FATSz32)) * bootEntry->BPB_BytsPerSec;
    int size = bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;

    int totalRecovered = 0;  // Counter for successfully recovered files

    // Search through root directory clusters
    while (curCluster != 0 && curCluster < 0x0FFFFFF8) {
        int rootOffset = dataSec + ((curCluster - 2) * size);
        DirEntry *rootDir = (DirEntry *)(addr + rootOffset);
        int maxFile = size / sizeof(DirEntry);

        // Process each directory entry
        for (int i = 0; i < maxFile; i++) {
            if (rootDir[i].DIR_Name[0] == 0x00) {
                break;  // End of directory entries
            }

            // Check for deleted files (0xE5) that aren't special entries
            if (rootDir[i].DIR_Name[0] == 0xE5 && 
                rootDir[i].DIR_Attr != 0x0f &&  // Not a long filename entry
                rootDir[i].DIR_Attr != 0x08 &&  // Not a system file
                rootDir[i].DIR_Attr != 0x10) {  // Not a directory
                
                // Create a filename from the deleted entry
                char filename[13] = {0};
                int filenameIdx = 0;
                
                // First character is special - we use a default
                filename[filenameIdx++] = '_';
                
                // Rest of name (up to 8 chars)
                for (int j = 1; j < 8; j++) {
                    if (rootDir[i].DIR_Name[j] == ' ') continue;
                    filename[filenameIdx++] = rootDir[i].DIR_Name[j];
                }
                
                // Extension if it exists
                if (rootDir[i].DIR_Name[8] != ' ') {
                    filename[filenameIdx++] = '.';
                    for (int j = 8; j < 11; j++) {
                        if (rootDir[i].DIR_Name[j] == ' ') continue;
                        filename[filenameIdx++] = rootDir[i].DIR_Name[j];
                    }
                }
                
                filename[filenameIdx] = '\0';
                
                // Recover this file
                rootDir[i].DIR_Name[0] = '_';  // Use '_' as the first character for recovered files
                
                // Update FAT entries based on file size
                int fileSize = rootDir[i].DIR_FileSize;
                int startCluster = rootDir[i].DIR_FstClusLO;
                
                if (fileSize > 0 && startCluster >= 2) {
                    if (fileSize <= size) {
                        // Single cluster file - mark as end of chain
                        fat[startCluster] = 0x0FFFFFF8;
                        fat2[startCluster] = 0x0FFFFFF8;
                    } else {
                        // Multi-cluster file - reconstruct chain
                        int clusterCount = (fileSize - 1) / size + 1;
                        int curCluster = startCluster;
                        
                        // Link clusters sequentially
                        for (int j = 0; j < clusterCount - 1; j++) {
                            fat[curCluster] = curCluster + 1;
                            fat2[curCluster] = curCluster + 1;
                            curCluster++;
                        }
                        
                        // Mark last cluster as end of chain
                        fat[curCluster] = 0x0FFFFFF8;
                        fat2[curCluster] = 0x0FFFFFF8;
                    }
                }
                
                printf("%s: recovered\n", filename);
                totalRecovered++;
            }
        }

        curCluster = fat[curCluster];
    }

    // Print summary of recovery operation
    if (totalRecovered == 0) {
        printf("No deleted files were found.\n");
    } else {
        printf("Successfully recovered %d file(s)\n", totalRecovered);
    }

    // Clean up
    munmap(addr, sb.st_size);
    close(fd);
}
/**
 * main entry point for the fat32 file system utility.
 * 
 * this function implements the command-line interface for the utility, handling:
 * 1. command-line argument parsing
 * 2. input validation
 * 3. routing to appropriate recovery functions
 * 
 * supported commands:
 * - -i: display file system information
 * - -l: list root directory contents
 * - -r filename [-s sha1]: recover a contiguous file
 * - -R filename -s sha1: recover a possibly non-contiguous file
 * - -ra filename: recover all files with given name
 * - -all: recover all deleted files
 * 
 * @param argc number of command-line arguments
 * @param argv array of command-line argument strings
 * 
 * @return 0 on successful execution, 1 on error
 * 
 * error handling:
 * - exits with status 1 if insufficient arguments provided
 * - exits with status 1 if invalid command-line options provided
 * - exits with status 1 if required parameters are missing
 * - exits with status 1 if multiple commands are specified
 */
int main(int argc, char *argv[]) {
    char *fileName = NULL;
    char *hash = NULL;
    char *diskName = NULL;
    int opt = 0;
    int infoCount = 0;
    int listCount = 0;
    int recCount = 0;
    int fileLen = 0;
    int shaCount = 0;
    int recoverAllCount = 0;
    int recoverAllDeletedCount = 0;

    if (argc < 3) {
        errUse();
        exit(EXIT_FAILURE);
    }

    diskName = argv[1];

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            infoCount = 1;
        } else if (strcmp(argv[i], "-l") == 0) {
            listCount = 1;
        } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            recCount = 1;
            fileName = argv[++i];
        } else if (strcmp(argv[i], "-R") == 0 && i + 1 < argc) {
            fileLen = 1;
            fileName = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            shaCount = 1;
            hash = argv[++i];
        } else if (strcmp(argv[i], "-ra") == 0 && i + 1 < argc) {
            recoverAllCount = 1;
            fileName = argv[++i];
        } else if (strcmp(argv[i], "-all") == 0) {
            recoverAllDeletedCount = 1;
        } else {
            errUse();
            exit(EXIT_FAILURE);
        }
    }

    if (infoCount + listCount + recCount + fileLen + recoverAllCount + recoverAllDeletedCount > 1 || 
        (fileLen && !fileName) || 
        (fileLen && !hash)) {
        errUse();
        exit(EXIT_FAILURE);
    }

    if (infoCount) {
       printDriveInfo(diskName);
    }

    if (listCount) {
       listRootDir(diskName);
    }

    if (recCount || fileLen) {
        if (fileName == NULL || fileName[0] == '\0' || fileName[0] == '\n') {
            errUse();
            exit(EXIT_FAILURE);
        }
        recFile(fileName, diskName, hash, fileLen);
    }
    
    if (recoverAllCount) {
        if (fileName == NULL || fileName[0] == '\0' || fileName[0] == '\n') {
            errUse();
            exit(EXIT_FAILURE);
        }
        recoverAllFiles(fileName, diskName);
    }
    
    if (recoverAllDeletedCount) {
        recoverAllDeleted(diskName);
    }

    return 0;
}
