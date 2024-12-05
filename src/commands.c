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


//Part 3 Functions
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

//Part 4 Functions
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

void close(char *FILENAME)
{
    for(int i = 0; i < MAX_FILES_OPEN; i++)
    {
        if(files_opened->descriptor != 0 && strcmp(files_opened[i].filename, FILENAME) == 0)
        {
            files_opened[i].descriptor = 0;
            files_opened[i].filename[0] = '\0';
            files_opened[i].flags[0] = '\0';
            files_opened[i].offset = 0;
            printf("File has been closed successfully. \n");
            return;
        }
    }
    printf("File is not open or does not exist. \n");
}
void size(char * FILENAME)
{
    DirEntry ent;
    if(!find_file(FILENAME, &ent))
    {
        printf("Error: The file does not exist. \n");
        return;
    }
    if(ent.DIR_Attr & 0x10)
    {
        printf("This is a Directory, not a file. \n");
        return;
    }
    printf("size of '%s': %d bytes. \n", FILENAME, ent.DIR_file_Size);
}

void lseek(char * FILENAME, unsigned int OFFSET)
{
    for(int i = 0; i < MAX_FILES_OPEN; i++)
    {
        if(files_opened[i].descriptor != 0 && strcmp(files_opened[i].filename, FILENAME) == 0)
        {
            DirEntry ent;
            if(!find_file(FILENAME, &ent))
            {
                printf("Error: File does not exist. \n");
                return;
            }
            if(OFFSET > ent.DIR_file_Size)
            {
                printf("Error: Offset exceeds file size. \n");
                return;
            }
            else
            {
                files_opened[i].offset = OFFSET;
                printf("The Offset if the file '%s' is set to %d. \n", FILENAME, OFFSET);
                return;
            }
        }
    }
    printf("Error: File is not opened. \n");
}

void lsof(void)
{
    bool Is_found = false;
    for(int i = 0; i < MAX_FILES_OPEN; i++)
    {
        if(files_opened[i].descriptor != 0)
        {
            Is_found = true;
            printf("%d\t%s\t%s\t%d\t%s\n", i, files_opened[i].filename,files_opened[i].flags, files_opened[i].offset, cwd.path);
        }
    }
    if(Is_found == false)
    {
        printf("No files opened. \n");
    }
}

void read(char * FILENAME, unsigned int size)
{
    int the_file_index = get_index(FILENAME);

    //This will validate the file
    DirEntry ent;
    if(!validating_file_for_reading(FILENAME, the_file_index, &ent))
    {
        return;
    }

    //This will calculate the read size 
    unsigned int the_offset = files_opened[the_file_index].offset;
    unsigned int the_read_size = clac_read_size(the_offset, size, ent.DIR_file_Size);

    if(the_read_size == 0)
    {
        printf("The end of the file has been reached. \n");
            return;
    }

    //This will correct the offset
    seeking_file_offset(ent.DIR_FstClusterLow, the_offset);

    //This will read and print the data
    read_print_data(the_read_size);

    //Need to update the file offset
    files_opened[the_file_index].offset += the_read_size;
}

//Part 5 Functions

void fs_rename(char *FILENAME, char *NEW_FILENAME) {
    // Validate the filenames
    if (strcmp(FILENAME, ".") == 0 || strcmp(FILENAME, "..") == 0) {
        printf("Error: Cannot rename special directories '.' or '..'.\n");
        return;
    }

    // Check if NEW_FILENAME already exists
    DirEntry entry;
    fseek(fp, cwd.byte_offset, SEEK_SET);
    while (fread(&entry, sizeof(DirEntry), 1, fp) == 1) {
        if (strncmp((char *)entry.DIR_Name, NEW_FILENAME, strlen(NEW_FILENAME)) == 0) {
            printf("Error: A file or directory with the name '%s' already exists.\n", NEW_FILENAME);
            return;
        }
    }

    // Locate the entry for FILENAME
    fseek(fp, cwd.byte_offset, SEEK_SET);
    while (fread(&entry, sizeof(DirEntry), 1, fp) == 1) {
        if (strncmp((char *)entry.DIR_Name, FILENAME, strlen(FILENAME)) == 0) {
            // Update the name
            memset(entry.DIR_Name, ' ', sizeof(entry.DIR_Name)); // Clear old name
            strncpy((char *)entry.DIR_Name, NEW_FILENAME, strlen(NEW_FILENAME));
            fseek(fp, -sizeof(DirEntry), SEEK_CUR); // Go back to the entry
            fwrite(&entry, sizeof(DirEntry), 1, fp);
            printf("Successfully renamed '%s' to '%s'.\n", FILENAME, NEW_FILENAME);
            return;
        }
    }

    // If we didn't find FILENAME
    printf("Error: File or directory '%s' not found.\n", FILENAME);
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

int get_index(const char *FILENAME)
{
    for(int i = 0; i < MAX_FILES_OPEN; i++)
    {
        if (files_opened[i].descriptor != 0)
        {
            if(strcmp(files_opened[i].filename, FILENAME) == 0)
            {
                return i; //This means the file is not opened.
            }
        }
    }
    return 0;
}

int validating_file_for_reading(const char * FILENAME, int index, DirEntry * ent)
{
    if(index == -1)
    {
        printf("The file is not opened. \n");
        return 0;
    }
    else if (strchr(files_opened[index].flags, 'r') == NULL)
    {
        printf("Error: File is not opened for reading. \n");
        return 0;
    }
    else if(!find_file(FILENAME, ent))
    {
        printf("Error: File does not exist. \n");
        return 0;
    }
    else if(ent->DIR_Attr & 0x10)
    {
        printf("Error: this is a directory, not a file. \n");
    }

    return 1;
    

}
unsigned int clac_read_size(unsigned int offset, unsigned int size, unsigned int file_size)
{
    if(offset >= file_size)
    {
        return 0; //There is nothing to read.
    }
    else if(offset + size > file_size)
    {
        return file_size - offset; //This if for the remaining size to be adjusted.
    }
    else
    {
        return size;
    }
}
void seeking_file_offset(unsigned int clus, unsigned int offset)
{
    unsigned int theBytesPerClus = bpb.BPB_BytesPerSec * cluster_sectors;
    unsigned int ClusOffset = offset % theBytesPerClus;
    unsigned int long sectOffset = first_sector_of_cluster(clus);
    fseek(fp, sectOffset + ClusOffset, SEEK_SET);
}
void read_print_data(unsigned int size)
{
    char * the_buf = (char*) malloc(size + 1);
    if(!the_buf)
    {
        printf("Error: Memory allocation had failed. \n");
        return;
    }
    fread(the_buf, 1, size, fp);
    the_buf[size] = '\0'; //Null-terminate the buffer
    printf("Read data: \n%s\n", the_buf);
    free(the_buf);
}
// int dir_location(char DIRNAME){
//       DirEntry dir;
//       unsigned long cluster=cwd.cluster;
//       unsigned long 
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


unsigned int first_cluster_of_entry(unsigned int lo, unsigned int hi) {
    return (hi * 65536) + lo;
}

void ls(){ 
    unsigned long cluster = cwd.cluster; //start cluster
    unsigned long sector = first_sector_of_cluster(cluster);  //first sector
     
    DirEntry entry;
    fseek(fp, sectors_to_bytes(sector), SEEK_SET); 
    // printf("%ld", ftell(fp));
    for(int i = 0; i < bpb.BPB_BytesPerSec * bpb.BPB_SecsPerClus / 32; i++) {
        fread(&entry, sizeof(DirEntry), 1, fp); 

        // skip empty
        if (entry.DIR_Name[0] == 0x00) {  //if empty
            continue;
        }

        if(entry.DIR_Attr == (0x01 | 0x02 | 0x04 | 0x08)) {
            continue;
        }

        printf("%s\n", entry.DIR_Name); 
    }
}

void cd(char * name) {
    unsigned long cluster = cwd.cluster; //start cluster
    unsigned long sector = first_sector_of_cluster(cluster);  //first sector
     
    DirEntry entry;
    fseek(fp, sectors_to_bytes(sector), SEEK_SET); 
    // printf("%ld", ftell(fp));
    for(int i = 0; i < bpb.BPB_BytesPerSec * bpb.BPB_SecsPerClus / 32; i++) {
        fread(&entry, sizeof(DirEntry), 1, fp); 

        // skip empty
        if (entry.DIR_Name[0] == 0x00) {  //if empty
            continue;
        }

        if(entry.DIR_Attr == (0x01 | 0x02 | 0x04 | 0x08)) {
            continue;
        }

        // trim name
        for(int i = 0; i < 11; i++) {
            if(entry.DIR_Name[i] == ' ') {
                entry.DIR_Name[i] = '\0';
            }
        }

        // str compare
        if(strcmp(entry.DIR_Name, name) == 0) {
            cwd.cluster = first_cluster_of_entry(entry.DIR_FstClusterLow, entry.DIR_FstClusterHi);
            
        }
    }
}

    // while (complete==false) { 
    //     fseek(fp, sectors_to_bytes(sector), SEEK_SET); 

    //     fread(&entry, sizeof(DirEntry), 1, fp); 

    //     if (entry.DIR_Name[0] == 0x00) {  //if empty
    //             complete=true; 
    //             break;
    //         } 
    //     else if (entry.DIR_Name[0] == 0xE5) { // if deleted. 
    //             continue;
    //         } 
    //     else { 
    //         printf("%s\n", entry.DIR_Name);
    //     } 



