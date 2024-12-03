#include "commands.h"

// Global variables
CWD cwd;
FILE *fp;
BPB bpb;
DirEntry current_entry;
opened_file files_opened[10];

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

// Tokenlist-related functions
tokenlist *new_tokenlist(void) {
    tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
    tokens->size = 0;
    tokens->items = (char **) malloc(sizeof(char *));
    tokens->items[0] = NULL; // NULL-terminated
    return tokens;
}

void add_token(tokenlist *tokens, char *item) {
    int i = tokens->size;

    tokens->items = (char **) realloc(tokens->items, (i + 2) * sizeof(char *));
    tokens->items[i] = (char *) malloc(strlen(item) + 1);
    tokens->items[i + 1] = NULL;
    strcpy(tokens->items[i], item);

    tokens->size += 1;
}

char *get_input(void) {
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

tokenlist *get_tokens(char *input) {
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

void free_tokens(tokenlist *tokens) {
    for (int i = 0; i < tokens->size; i++)
        free(tokens->items[i]);
    free(tokens->items);
    free(tokens);
}

void info(){

    unsigned int ds = bpb.BPB_TotSec32 - (bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32));
    unsigned int total = ds / bpb.BPB_SecsPerClus;
    unsigned int image_size = bpb.BPB_TotSec32 * bpb.BPB_BytesPerSec;

    printf("Position of root cluster: %u\n", bpb.BPB_RootClus);
    printf("Bytes per Sector: %u\n", bpb.BPB_BytesPerSec);
    printf("Sectors per Cluster: %u\n", bpb.BPB_SecsPerClus);
    printf("Number of FATs: %u\n", bpb.BPB_NumFATs);
    printf("Total # of clusters in the data region: %u\n", total);
    printf("# of entries in one FAT: %u\n", bpb.BPB_RootClus);
    printf("Size of one image: %u sectors\n", image_size);
}

void exit_program(){
    if(fp != NULL)
    {
        fclose(fp);
        printf("File had been closed. ");
    }
    printf("Exiting the program.");
}


//Part 3 Funtions
void mkdir(char *DIRNAME){
    if(strlen(DIRNAME) <= 10) //checks it length is in bounds.
    {
        for(int i = 0; i < 10;i++) //This checks if the directoty already exist.
        {
            if(files_opened[i].descriptor != 0 && strcmp(files_opened[i].filename, DIRNAME)== 0)
            {
                printf("Error: Directory '%s' exists.\n", DIRNAME);
                return;
            }
        }

        //This will allocate new cluster to a new directory.
        unsigned int new_clus = current_clus();

        if(new_clus == 0)
        {
            printf("No clusters available. \n");
            return;
        }
        //This will create a directorty entry
        DirEntry newdir= {0};
        strncpy((char*) newdir.DIR_Name, DIRNAME,11);
        newdir.DIR_Attr = 0x10;
        newdir.DIR_FstClusterLow = new_clus & 0xFFFF;
        newdir.DIR_FstClusterHi = 0;
        newdir.DIR_file_Size = 0;

        //We have to write the directory entry to the current dirrectory
        fwrite(&newdir,sizeof(DirEntry), 1, fp);

    }
    else
    {
        printf("Error: Directory name '%s' is long. \n", DIRNAME);
        return;
    }
}


//Additonal funtions
unsigned int current_clus(){
    unsigned int fat_entry;
    unsigned int clus = 2;

    //I need to seek the beginning of the the FAT file
    fseek(fp, fat_begin_lba * bpb.BPB_BytesPerSec, SEEK_SET);
    
    //This will loop through the FAT table to find a free cluster

    while(1)
    {
        fread(&fat_entry, sizeof(unsigned int),1, fp);

        //this is a checker to see if the cluster if free
        if(fat_entry == 0x00000000)
        {
            //I need to mark this cluster off since it will be allocated 
            fseek(fp, -sizeof(unsigned int), SEEK_CUR);

            unsigned int end_of_chain_marker = 0x0FFFFFFF; //This is the end of chain marker for FAT32
            fwrite(&end_of_chain_marker, sizeof(unsigned int), 1,fp );
            return clus;
        }
        clus++;

        //Creating a check so we dont go over the boundary
        if(clus >= (bpb.BPB_TotSec32 - first_data_sector)/cluster_sectors)
        {
            printf("There are no free clusters available.\n");
            return 0;
        }
    }
}

// int dir_location(char DIRNAME){
//       DirEntry dir;
//       unsigned long cluster=cwd.cluster;
//       unsigned long 
// } 


// void cd(char *DIRNAME){


// }

unsigned int get_first_data_sector(){
    return bpb.BPB_RsvdSecCnt+(bpb.BPB_NumFATs*bpb.BPB_FATSz32);
}

unsigned int sectors_to_bytes( unsigned int sector) {
    return sector * bpb.BPB_BytesPerSec;
}

void ls(){
    // fseek(sectors_to_bytes(first_data_sector(cwd.cluster)))

}