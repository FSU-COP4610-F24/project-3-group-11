#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Global variables
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
