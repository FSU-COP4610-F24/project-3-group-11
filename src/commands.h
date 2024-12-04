#ifndef COMMANDS_H
#define COMMANDS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_SIZE 256
#define MAX_FILES_OPEN 10
// Tokenlist structure
typedef struct {
    int size;
    char **items;
} tokenlist;

// Tokenlist-related function declarations
char *get_input(void);
tokenlist *get_tokens(char *input);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

void info();
void cd(char *DIRNAME);
void ls();

//Part 3 functions
void mkdir(char *DIRNAME);
void creat(char *FILENAME);
//Part 4 functions
void open(char *FILENAME, int FLAGS);
void close(char *FILENAME);
void lsof(void);
void size(char *FILENAME);
void lseek(char * FILENAME, unsigned int OFFSET);
void read(char * FILENAME, unsigned int size); 
void exit_program();


//Additonal funtions
int dir_location(char DIRNAME); //This function will check if the dir name is exist or not. 
unsigned int current_clus(); //This function will get the current cluster.
unsigned int get_first_data_sector();
unsigned int sectors_to_bytes(unsigned int);
unsigned int first_sector_of_cluster(unsigned int clusterNumber);


typedef struct {
    char path[PATH_SIZE];     // this will be the part of CWD
    unsigned long root_offset; // this is the offset for the root directory.
    unsigned long byte_offset; // this is the byte offset
    unsigned long cluster;
 } CWD;

typedef struct {
    char filename[256];
    int descriptor;
    unsigned int cluster;
} opened_file;

typedef struct __attribute__((packed))
{
    unsigned char BS_jmpBoot[3];
    unsigned char BS_OEMName[8];
    unsigned short BPB_BytesPerSec;
    unsigned char BPB_SecsPerClus;
    unsigned short BPB_RsvdSecCnt;  
    unsigned char BPB_NumFATs;
    unsigned short BPB_RootEntCnt;
    unsigned short BPB_TotSec16;
    unsigned char BPB_Media;
    unsigned short BPB_FATSz16;
    unsigned short BPB_SecPerTrk;
    unsigned short BPB_NumHeads;
    unsigned int BPB_HiddSec;
    unsigned int BPB_TotSec32;
    unsigned int BPB_FATSz32;
    unsigned short BPB_ExtFlags;
    unsigned short BPB_FSVer;
    unsigned int BPB_RootClus;
    unsigned short BPB_FSInfo;
    unsigned short BPB_BkBootSe;
    unsigned char BPB_Reserved[12];
    unsigned char BS_DrvNum;
    unsigned char BS_Reserved1;
    unsigned char BS_BootSig;
    unsigned int BS_VollD;
    unsigned char BS_VolLab[11];
    unsigned char BS_FilSysType[8];
    unsigned char empty[420];
    unsigned short Signature_word;
} BPB;

typedef struct __attribute__((packed))
{
    unsigned char DIR_Name[11];
    unsigned char DIR_Attr;
    unsigned char DIR_NTRes;
    unsigned char DIR_CrtTimeTenth;
    unsigned short DIR_CrtTime;
    unsigned short DIR_CrtDate;
    unsigned short DIR_LastAccDate;
    unsigned short DIR_FstClusterHi;
    unsigned short DIR_FstClusterLow;
    unsigned short DIR_WriteTime;
    unsigned short WriteDate;
    unsigned int DIR_file_Size;
    
} DirEntry;



//Global Variables
extern CWD cwd;
extern FILE *fp;
extern BPB bpb;
extern DirEntry current_entry;
extern opened_file files_opened[10];

extern unsigned long fat_begin_lba;
extern unsigned long cluster_begin_lba;
extern unsigned char cluster_sectors;
extern unsigned long root_directory_first_cluster;
extern unsigned long root_dir_clusters;
extern unsigned long first_data_sector;
extern unsigned long first_data_sector_offset;
extern unsigned long Partition_LBA_Begin;
extern unsigned long currentDirectory;
extern int number_files_open;
#endif // COMMANDS_H
