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
void mkdir(char *DIRNAME) {
    if (strlen(DIRNAME) < 1 || strlen(DIRNAME) > 11) {
        printf("Error: Directory name '%s' is invalid (too long or empty).\n", DIRNAME);
        return;
    }

    // Check if the directory already exists
    DirEntry entry;
    fseek(fp, cwd.root_offset, SEEK_SET);
    while(fread(&entry, sizeof(DirEntry), 1, fp) == 1) {
        if (strncmp((char *)entry.DIR_Name, DIRNAME, strlen(DIRNAME)) == 0 &&
            (entry.DIR_Attr & 0x10)) {
            printf("Error: Directory '%s' already exists.\n", DIRNAME);
            return;
        }
    }

    // Allocate a new cluster for the directory
    unsigned int new_clus = current_clus();
    if (new_clus == 0) {
        printf("Error: No free clusters available.\n");
        return;
    }

    // Create a directory entry
    DirEntry newdir = {0};
    strncpy((char *)newdir.DIR_Name, DIRNAME, 11);
    newdir.DIR_Attr = 0x10;
    newdir.DIR_FstClusterLow = new_clus & 0xFFFF;
    newdir.DIR_FstClusterHi = (new_clus >> 16) & 0xFFFF;
    newdir.DIR_file_Size = 0;

    // Write the directory entry to the current directory
    fseek(fp, cwd.root_offset, SEEK_SET);
    while (fread(&entry, sizeof(DirEntry), 1, fp) == 1) {
        if (entry.DIR_Name[0] == 0x00 || entry.DIR_Name[0] == 0xE5) {
            fseek(fp, -sizeof(DirEntry), SEEK_CUR);
            fwrite(&newdir, sizeof(DirEntry), 1, fp);
            break;
        }
    }

    // Initialize the new directory cluster
    unsigned long cluster_offset = cluster_begin_lba + (new_clus - 2) * cluster_sectors;
    fseek(fp, cluster_offset * bpb.BPB_BytesPerSec, SEEK_SET);

    
    printf("Directory '%s' created successfully.\n", DIRNAME);
    return;
}
void creat(char * FILENAME)
{
    if(strlen(FILENAME) < 1 || strlen(FILENAME) > 11)
    {
        printf("This file is too long or empty. \n");
        return;
    }

    //This will check if the file already exist
    DirEntry ent;
    fseek(fp, cwd.root_offset, SEEK_SET);
    while(fread(&ent, sizeof(DirEntry), 1, fp)==1)
    {
        if (strncmp((char *)ent.DIR_Name, FILENAME, strlen(FILENAME)) == 0)
        {
            printf("Error: FIle already exist. \n");
            return;
        }
    }

    //Directed entry will be created for the new file.
    DirEntry nFile = {0};
    strncpy((char*)nFile.DIR_Name, FILENAME, 11);
    nFile.DIR_Attr = 0x20;
    nFile.DIR_FstClusterLow = 0; 
    nFile.DIR_FstClusterHi = 0; 
    nFile.DIR_file_Size = 0;

    //This will write the file entry to the current dir
    fseek(fp, cwd.root_offset, SEEK_SET);
    while(fread(&ent, sizeof(DirEntry), 1, fp) == 1)
    {
        if(ent.DIR_Name[0] == 0x00 || ent.DIR_Name[0] == 0xE5)
        {
            fseek(fp, -sizeof(DirEntry), SEEK_CUR);
            fwrite(&nFile, sizeof(DirEntry), 1, fp);
            printf("File has been successfully created. \n");
            return;
        }
    }
    printf("Error: No space available to create the file. \n");

}
void open(const char *FILENAME, const char *flags)
{
    //This vill validate the flag
    if(!is_flags(flags))
    {
        printf("Error: Invalid mode. \n");
        return;
    }
    //This will check if the file is already opened
    if(is_file_opened(FILENAME))
    {
        printf("Error: File is alredy opened. \n");
        return;
    }

    //This will search for the file in the current directory.
    DirEntry ent;
    if(!find_file(FILENAME, &ent))
    {
        printf("Error: FIle does not exist. \n");
        return;
    }
    //This will add the file to the files_opened array
    if(!adding_to_open_files(FILENAME, flags))
    {
        printf("Error: Too many files opened. \n");
        return;
    }

    printf("File is opened.\n");
}


///////////////////////////////////////Additonal funtions//////////////////////////////
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

int is_flags(const char *flag) 
{
    return strcmp(flag, "-r") == 0 || strcmp(flag, "-w") == 0 || strcmp(flag, "-rw") == 0 || strcmp(flag, "-wr") == 0;
}


int is_file_opened(const char * FILENAME)
{
    for(int i = 0; i < MAX_FILES_OPEN; i++)
    {
        if(files_opened[i].descriptor != 0 && strcmp(files_opened[i].filename, FILENAME)==0)
        {
            return 1; //This means the file is opened.
        }
    }
    return 0; //This means the file is not opened.
}

int find_file(const char * FILENAME,DirEntry * entry)
{
    fseek(fp, cwd.root_offset,SEEK_SET);
    while(fread(entry, sizeof(DirEntry), 1, fp)==1)
    {
        if(strncmp((char *)entry->DIR_Name, FILENAME, strlen(FILENAME)) == 0)
        {
            if((!(entry->DIR_Attr)) && 0x10 == 0)
            {
                return 1; //The file is found;
            }
        }
    }
    return 0; //The file is not found;
}
int adding_to_open_files(const char * FILENAME, const char * flags)
{
    for(int i = 0; i < MAX_FILES_OPEN; i++)
    {
        if(files_opened[i].descriptor == 0) //meaning empty slot
        {
            strcpy(files_opened[i].filename, FILENAME);
            files_opened[i].descriptor = i + 1; //This will be my unique descriptor
            strncpy(files_opened[i].flags, flags+1, 2); //This will remove it.
            files_opened[i].offset = 0;
            return 1; //This means its been added successfully.
        }
    }
    return 0; //no slots available
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

unsigned int first_sector_of_cluster(unsigned int clusterNumber){
    return ((clusterNumber-2)*bpb.BPB_SecsPerClus)+get_first_data_sector();
}

unsigned int sectors_to_bytes( unsigned int sector) {
    return sector * bpb.BPB_BytesPerSec;
}

/*void ls(){
    unsigned long cluster=cwd.cluster;
    

    //fseek(sectors_to_bytes(get_first_data_sector(cwd.cluster))

}*/