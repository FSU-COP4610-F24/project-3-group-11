#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int size;
	char **items;
} tokenlist;

char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

//These are the functions that we need: ***ALL are VOIDS// you can add them in the .h file if you want

//void info();
//void cd(char * DIRNAME);
//void ls(void);
//mkdir(char *DIRNAME);
//creat(char * FILENAME);
//cp(char *FILENAME);
//open(char * FILENAME, int flags);
//close(char * FILENAME);
//void lsof(void);
//void size(char  * FILENAME);
//lseek(char * FILENAME, unsigned int OFFSET);
//read(char * FILENAME, char *STRING);
//void write(char * FILENAME, char * STRING);
//void rm(char * FILENAME);
//void rmdir(char *DIRNAME);

//the global variables:
CWD cdw;
FILE * fp;
BPB bpb;
DirEntry current_entry;
opened _file files_opened[10];


//variables that are needed for accessing/chnaging/working with the FAT32 files
unsigned long fat_begin_lba;
unsigned long cluster_begin_lba;
unsigned char cluster_sectors;
unsigned long root_directory_first_cluster;
unsigned long root_dir_clusters;
unsigned long first_data_sector;
unsigned long first_data_sector_offset;
unsigned long Partition_LBA_Begin;
unsigned long currentDirectory;
int number_files_open = 0;

int main(int argc, char *argv)
{
    if(argc != 2) // The number of arguments will be checked.
    (
        printf("number of arguments are invalid");
        return -1;
    )
    fp = fopen(argv[1], "r+");
    if(fp == NULL)
    {
        printf("%s this does not exist.", argv[1]);
        return -1;
    }
    //this will have the information from bpb as well as initialize any of the improtant global variables.
    fread(&bpb, sizeof(BPB), 1, fp);
    cluster_sectors = bpb.BPB_SecsPerClus;
    root_directory_first_cluster = bpb.BPB_RootClus;

    cluster_begin_lba = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATz32);

    root_dir_clusters = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytesPerSec - 1))/bpb.BPB_BytesPerSec;
    first_data_sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + root_dir_clusters;
    first_data_sector_offset = first_data_sector * bpb.BPB_BytesPerSec;
    cwd.root_offset = first_data_sector_offset;
    cwd.byte_offset = cwd.root_offset;
    cwd.cluster = bpb.BPB_RootClus;

    memset(cwd.path, 0, PATH_SIZE);
    strcat(cwd.path, argv[1]);

    //This will be the parser
    char * input;
	while (1) {
        printf("$s/> ", cwd.path);

		char input = get_input();
		printf("whole input: %s\n", input);

		tokenlist *tokens = get_tokens(input);
		for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}

		free(input);
		free_tokens(tokens);
	}

	return 0;
}

tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **) malloc(sizeof(char *));
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
	int i = tokens->size;

	tokens->items = (char **) realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *) malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

char *get_input(void)
{
	char *buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL) {
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char *) realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char *) realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;

	return buffer;
}

tokenlist *get_tokens(char *input)
{
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();

	char *tok = strtok(buf, " ");
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}

	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);
	free(tokens->items);
	free(tokens);
}