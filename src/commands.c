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
void mkdir(char *DIRNAME) 
{
    if (strlen(DIRNAME) < 1 || strlen(DIRNAME) > 11) 
    {
        printf("Error: Directory name is too long or empty.\n");
        return;
    }

    // This will check is the directory already exists.
    DirEntry ent;
    fseek(fp, cwd.root_offset, SEEK_SET);

    while(fread(&ent, sizeof(DirEntry), 1, fp) == 1) 
    {
        if(strncmp((char *)ent.DIR_Name, DIRNAME, strlen(DIRNAME)) == 0)
        {
            if(ent.DIR_Attr & 0x10)
            {
                printf("Error: Directory already exists.\n");
            return;
            }
        }
    }

    // This will allocate a new cluster for the directory
    unsigned int new_clus = current_clus();
    if (new_clus == 0)
    {
        printf("Error: No free clusters available.\n");
        return;
    }

    //This will create a new directory entry
    DirEntry ndir = {0};
    strncpy((char *)ndir.DIR_Name, DIRNAME, 11);
    ndir.DIR_Attr = 0x10;
    ndir.DIR_FstClusterLow = new_clus & 0xFFFF;
    ndir.DIR_FstClusterHi = (new_clus >> 16) & 0xFFFF;
    ndir.DIR_file_Size = 0;

    // This will write the directory entry to the current directory
    fseek(fp, cwd.root_offset, SEEK_SET);
    while (fread(&ent, sizeof(DirEntry), 1, fp) == 1) 
    {
        if (ent.DIR_Name[0] == 0x00 || ent.DIR_Name[0] == 0xE5) 
        {
            fseek(fp, -sizeof(DirEntry), SEEK_CUR);
            fwrite(&ndir, sizeof(DirEntry), 1, fp);
            break;
        }
    }

    // This will initialize the new directory cluster
    unsigned long cluster_offset = cluster_begin_lba + (new_clus - 2) * cluster_sectors;
    fseek(fp, cluster_offset * bpb.BPB_BytesPerSec, SEEK_SET);

    DirEntry d = {0}; //this is the dot
    d.DIR_Name[0] = '.';//This is the ftist byte.
    for(int i = 1; i < 11; i++)
    {
        d.DIR_Name[i]= ' ';
    }
    d.DIR_Attr = 0x10;
    d.DIR_FstClusterLow = new_clus & 0xFFFF;
    d.DIR_FstClusterHi = (new_clus >> 16) & 0xFFFF;

    DirEntry dd = {0}; //this is the dotdot
    dd.DIR_Name[0] = '.';
    dd.DIR_Name[1] = '.';
    for(int i = 2; i < 11; i++)
    {
        dd.DIR_Name[1] = ' ';
    }
    dd.DIR_Attr = 0x10;
    dd.DIR_FstClusterLow = cwd.cluster & 0xFFFF;
    dd.DIR_FstClusterHi = (cwd.cluster >> 16) & 0xFFFF;

    //This will write the new entries to the directory.
    fseek(fp,cluster_begin_lba + (new_clus -2 )*cluster_sectors *bpb.BPB_BytesPerSec,SEEK_SET);
    fwrite(&d, sizeof(DirEntry), 1, fp);
    fwrite(&dd, sizeof(DirEntry),1,fp);
    
    printf("Directory created successfully.\n");
    return;
}
void creat(char *FILENAME) {
    if (strlen(FILENAME) > 11 || strlen(FILENAME) < 1) 
    {
        printf("Error: File name is too long or empty.\n");
        return;
    }

    // this will check if the file already exist.
    DirEntry ent;
    fseek(fp, cwd.root_offset, SEEK_SET);
    while (fread(&ent, sizeof(DirEntry), 1, fp) == 1) 
    {
        if (ent.DIR_Name[0] == 0x00 || ent.DIR_Name[0] == 0xE5) 
        {
            continue;
        }

        // this creates the name comparison.
        char name_exist[12] = {0};
        strncpy(name_exist, (char *)ent.DIR_Name, 8);
        for (int i = 7; i >= 0; i--) {
            if (name_exist[i] == ' ') 
            {
                name_exist[i] = '\0'; //This will remove the trailing spaces.
            } 
            else 
            {
                break;
            }
        }

        // This will add the exention if present name.
        if (ent.DIR_Name[8] != ' ') 
        {
           size_t length = (strlen(name_exist));
           name_exist[length] = '.';
           name_exist[length + 1] = '\0';
           strncat(name_exist, (char*)&ent.DIR_Name[8], 3);
        }

        // This will compare the FILENAME with the inputed filename.
        if (strcmp(name_exist, FILENAME) == 0) 
        {
            printf("Error: File already exists. \n");
            return;
        }
    }

    // This will create a new directory entry for the file
    DirEntry new_file = {0};
    memset(new_file.DIR_Name, ' ', 11); // This will fill in the spaces.
    char *d = strchr(FILENAME, '.');

    if (d) 
    {
        size_t length_of_name = d - FILENAME;
        size_t length_of_extension = strlen(d + 1);

        if (length_of_name > 8 || length_of_extension > 3) {
            printf("Error: Filename is too long.\n");
            return;
        }

        strncpy((char *)new_file.DIR_Name, FILENAME, length_of_name);

        strncpy((char *)&new_file.DIR_Name[8], d + 1, length_of_extension);
    } 
    else 
    {
        strncpy((char *)new_file.DIR_Name, FILENAME, strlen(FILENAME));
    }

    new_file.DIR_Attr = 0x20; 
    new_file.DIR_FstClusterLow = 0; 
    new_file.DIR_FstClusterHi = 0;  
    new_file.DIR_file_Size = 0;

    // this will write the new entry to the current directory
    fseek(fp, cwd.root_offset, SEEK_SET);
    while (fread(&ent, sizeof(DirEntry), 1, fp) == 1) 
    {
        if (ent.DIR_Name[0] == 0x00 || ent.DIR_Name[0] == 0xE5) 
        {
            fseek(fp, -sizeof(DirEntry), SEEK_CUR);
            fwrite(&new_file, sizeof(DirEntry), 1, fp);
            printf("File created successfully.\n");
            return;
        }
    }

    printf("Error: No available space to create this file.\n");
}

void open(const char *FILENAME, const char *flags)
{
    //This vill validate the flag
    if(!is_flags(flags))
    {
        printf("Error: Invalid mode.\n");
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
        printf("Error: File does not exist. \n");
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

void write(char *FILENAME, char *STRING) 
{
    // Check if the file is opened
    int file_index = -1;
    for (int i = 0; i < MAX_FILES_OPEN; i++) {
        if (files_opened[i].descriptor != 0 && strcmp(files_opened[i].filename, FILENAME) == 0) {
            file_index = i;
            break;
        }
    }

    if (file_index == -1) {
        printf("Error: File '%s' is not opened.\n", FILENAME);
        return;
    }

    // Ensure the file is opened for writing
    if (strchr(files_opened[file_index].flags, 'w') == NULL) {
        printf("Error: File '%s' is not opened for writing.\n", FILENAME);
        return;
    }

    // Strip quotes from STRING if necessary
    if (STRING[0] == '"' && STRING[strlen(STRING) - 1] == '"') {
        STRING[strlen(STRING) - 1] = '\0';
        STRING++;
    }

    // Locate the file in the directory
    DirEntry entry;
    if (!find_file(FILENAME, &entry)) {
        printf("Error: File '%s' not found.\n", FILENAME);
        return;
    }

    unsigned int offset = files_opened[file_index].offset;
    unsigned int bytes_per_cluster = bpb.BPB_BytesPerSec * bpb.BPB_SecsPerClus;
    unsigned int cluster = first_cluster_of_entry(entry.DIR_FstClusterLow, entry.DIR_FstClusterHi);
    unsigned int string_length = strlen(STRING);
    unsigned int written = 0;

    // Traverse to the correct cluster based on the offset
    while (offset >= bytes_per_cluster) {
        cluster = get_next_cluster(cluster);
        if (cluster == 0xFFFFFFFF) {
            printf("Error: Reached end of file cluster chain.\n");
            return;
        }
        offset -= bytes_per_cluster;
    }

    // Write data to the file
    while (written < string_length) {
        unsigned int sector = first_sector_of_cluster(cluster) + (offset / bpb.BPB_BytesPerSec);
        unsigned int byte_offset = offset % bpb.BPB_BytesPerSec;

        // Seek to the correct position in the file
        fseek(fp, sectors_to_bytes(sector) + byte_offset, SEEK_SET);

        // Write as much as possible in the current cluster
        unsigned int write_size = bytes_per_cluster - offset;
        if (write_size > (string_length - written)) {
            write_size = string_length - written;
        }
        fwrite(&STRING[written], 1, write_size, fp);

        // Update metadata
        written += write_size;
        offset += write_size;

        // Allocate a new cluster if needed
        if (offset >= bytes_per_cluster) {
            unsigned int new_cluster = current_clus();
            if (new_cluster == 0) {
                printf("Error: No more clusters available.\n");
                return;
            }
            set_next_cluster(cluster, new_cluster);
            cluster = new_cluster;
            offset = 0;
        }
    }

    // Update the file's size and offset
    files_opened[file_index].offset += written;
    if (files_opened[file_index].offset > entry.DIR_file_Size) {
        entry.DIR_file_Size = files_opened[file_index].offset;
        fseek(fp, -sizeof(DirEntry), SEEK_CUR);
        fwrite(&entry, sizeof(DirEntry), 1, fp);
    }

    printf("Successfully wrote '%s' to '%s'.\n", STRING, FILENAME);
}

unsigned int get_next_cluster(unsigned int cluster) 
{
    unsigned int fat_offset = fat_begin_lba * bpb.BPB_BytesPerSec + cluster * 4;
    unsigned int next_cluster;
    fseek(fp, fat_offset, SEEK_SET);
    fread(&next_cluster, sizeof(unsigned int), 1, fp);
    return next_cluster;
}

void set_next_cluster(unsigned int cluster, unsigned int next_cluster) 
{
    unsigned int fat_offset = fat_begin_lba * bpb.BPB_BytesPerSec + cluster * 4;
    fseek(fp, fat_offset, SEEK_SET);
    fwrite(&next_cluster, sizeof(unsigned int), 1, fp);
}

//Part 6 Functions
void rm(char *FILENAME) {
    // Check if the file is opened
    if (is_file_opened(FILENAME)) {
        printf("Error: File '%s' is currently opened.\n", FILENAME);
        return;
    }

    // Locate the file in the current directory
    DirEntry entry;
    if (!find_file(FILENAME, &entry)) {
        printf("Error: File '%s' does not exist.\n", FILENAME);
        return;
    }

    // Ensure the entry is not a directory
    if (entry.DIR_Attr & 0x10) {
        printf("Error: '%s' is a directory, not a file.\n", FILENAME);
        return;
    }

    // Traverse the FAT table to free all clusters allocated to the file
    unsigned int cluster = first_cluster_of_entry(entry.DIR_FstClusterLow, entry.DIR_FstClusterHi);
    unsigned int fat_offset;
    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        fat_offset = fat_begin_lba * bpb.BPB_BytesPerSec + cluster * 4;
        fseek(fp, fat_offset, SEEK_SET);

        unsigned int next_cluster;
        fread(&next_cluster, sizeof(unsigned int), 1, fp);

        // Mark the current cluster as free
        fseek(fp, fat_offset, SEEK_SET);
        unsigned int free_marker = 0x00000000;
        fwrite(&free_marker, sizeof(unsigned int), 1, fp);

        // Move to the next cluster
        cluster = next_cluster;
    }

    // Mark the directory entry as deleted
    fseek(fp, -sizeof(DirEntry), SEEK_CUR);
    entry.DIR_Name[0] = 0xE5; // Mark entry as deleted
    fwrite(&entry, sizeof(DirEntry), 1, fp);

    printf("File '%s' deleted successfully.\n", FILENAME);
}

void rmdir(char *DIRNAME) {
    // Locate the directory in the current directory
    DirEntry entry;
    if (!find_file(DIRNAME, &entry)) {
        printf("Error: Directory '%s' does not exist.\n", DIRNAME);
        return;
    }

    // Ensure the entry is a directory
    if (!(entry.DIR_Attr & 0x10)) {
        printf("Error: '%s' is not a directory.\n", DIRNAME);
        return;
    }

    // Check if the directory is empty
    unsigned int cluster = first_cluster_of_entry(entry.DIR_FstClusterLow, entry.DIR_FstClusterHi);
    unsigned int sector = first_sector_of_cluster(cluster);
    fseek(fp, sectors_to_bytes(sector), SEEK_SET);

    DirEntry subentry;
    for (int i = 0; i < bpb.BPB_BytesPerSec * bpb.BPB_SecsPerClus / sizeof(DirEntry); i++) {
        fread(&subentry, sizeof(DirEntry), 1, fp);

        // Skip empty entries or special directories (".", "..")
        if (subentry.DIR_Name[0] == 0x00 || subentry.DIR_Name[0] == 0xE5) {
            continue;
        }
        if (strncmp((char *)subentry.DIR_Name, ".", 1) == 0 || strncmp((char *)subentry.DIR_Name, "..", 2) == 0) {
            continue;
        }

        // If we find any other entry, the directory is not empty
        printf("Error: Directory '%s' is not empty.\n", DIRNAME);
        return;
    }

    // Traverse the FAT table to free the cluster
    unsigned int fat_offset;
    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        fat_offset = fat_begin_lba * bpb.BPB_BytesPerSec + cluster * 4;
        fseek(fp, fat_offset, SEEK_SET);

        unsigned int next_cluster;
        fread(&next_cluster, sizeof(unsigned int), 1, fp);

        // Mark the current cluster as free
        fseek(fp, fat_offset, SEEK_SET);
        unsigned int free_marker = 0x00000000;
        fwrite(&free_marker, sizeof(unsigned int), 1, fp);

        // Move to the next cluster
        cluster = next_cluster;
    }

    // Mark the directory entry as deleted
    fseek(fp, -sizeof(DirEntry), SEEK_CUR);
    entry.DIR_Name[0] = 0xE5; // Mark entry as deleted
    fwrite(&entry, sizeof(DirEntry), 1, fp);

    printf("Directory '%s' deleted successfully.\n", DIRNAME);
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

int find_file(const char * FILENAME,DirEntry * ent)
{
   fseek(fp, cwd.root_offset, SEEK_SET);
    char the_name[12] = {0}; //formatting the name
    memset(the_name, ' ', 11); // needed to do some space padding for the the filename and extension name.
    const char *d = strchr(FILENAME, '.');
    if (d) {
        strncpy(the_name, FILENAME, d - FILENAME); // the will copy the name
        strncpy(the_name+ 8, d + 1, 3); // this will copy the extension.
    } else
    {
        strncpy(the_name, FILENAME, strlen(FILENAME)); // this had no extension.
    }

    while (fread(ent, sizeof(DirEntry), 1, fp) == 1) 
    {
        if (ent->DIR_Name[0] == 0x00 || ent->DIR_Name[0] == 0xE5)
        {
            continue; //This will skip of delete related entries
        }
        if (strncmp((char *)ent->DIR_Name, the_name, 11) == 0) 
        {
            return 1; // This means the file has been found.
        }
    }
    return 0; //This means the file has not been found.
}
int adding_to_open_files(const char * FILENAME, const char * flags)
{
    for(int i = 0; i < MAX_FILES_OPEN; i++)
    {
        if(files_opened[i].descriptor == 0) //meaning empty slot
        {
            strcpy(files_opened[i].filename, FILENAME);

            files_opened[i].descriptor = i + 1;

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



