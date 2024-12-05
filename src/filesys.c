#include "commands.h"

int main(int argc, char *argv[]) {
    if (argc != 2) { // The number of arguments will be checked.
        printf("number of arguments are invalid\n");
        return -1;
    }

    //fopen(const char* file name. const char * mode)
    fp = fopen(argv[1], "r+");
    if (fp == NULL) {
        printf("%s this does not exist.\n", argv[1]);
        return -1;
    }
   


    // This will have the information from BPB as well as initialize any of the important global variables.
    fread(&bpb, sizeof(BPB), 1, fp);
    cluster_sectors = bpb.BPB_SecsPerClus;
    root_directory_first_cluster = bpb.BPB_RootClus;

    cluster_begin_lba = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32);

    root_dir_clusters = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytesPerSec - 1)) / bpb.BPB_BytesPerSec;
    first_data_sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + root_dir_clusters;
    first_data_sector_offset = first_data_sector * bpb.BPB_BytesPerSec;
    cwd.root_offset = first_data_sector_offset;
    cwd.byte_offset = cwd.root_offset;
    cwd.cluster = bpb.BPB_RootClus;

    memset(cwd.path, 0, PATH_SIZE);
    strcat(cwd.path, argv[1]);

    // This will be the parser
    char *input;
    while (1) {
        printf("%s/> ", cwd.path);

        input = get_input();
        printf("whole input: %s\n", input);

        tokenlist *tokens = get_tokens(input);
        for (int i = 0; i < tokens->size; i++) {
            printf("token %d: (%s)\n", i, tokens->items[i]);
        }
         
         if (strcmp(tokens->items[0], "info") == 0) {
            info();
        }
        if (strcmp(tokens->items[0], "exit") == 0) {
            exit_program();      
        }
        if(strcmp(tokens->items[0], "open")==0)
        {
            if(tokens->size != 3)
            {
                printf("Error: Invalid usage of open. \n");
            }
            else
            {
                open(tokens->items[1], tokens->items[2]);
            }
        }
        if(strcmp(tokens->items[0], "open") == 0)
        {
            if(tokens->size == 2)
            {
                close(tokens->items[1]);
            }
            else
            {
                printf("Error: Incorrect format for open command. \n");
            }
        }
        if(strcmp(tokens->items[0], "lsof") == 0)
        {
            lsof();
        }
        if(strcmp(tokens->items[0], "size") == 0)
        {
            size(tokens->items[1]);
        }
        if(strcmp(tokens->items[0], "lseek") == 0)
        {
            if(tokens->size == 3)
            {
                lseek(tokens->items[1], atoi(tokens->items[2]));
            }
            else
            {
                printf("incorrect format for lseek. \n");
            }
        }
        if(strcmp(tokens->items[0], "read") == 0)
        {
            if(tokens->size == 3)
            {
                read(tokens->items[1], atoi(tokens->items[2]));
            }
            else
            {
                printf("incorrect format for read. \n");
            }
        }
        if(strcmp(tokens->items[0], "exit") == 0)
        {
            if(tokens->size == 3)
            {
                read(tokens->items[1], atoi(tokens->items[2]));
            }
        }

        /*if (strcmp(tokens->items[0], "ls") == 0) {
            ls();      
        }*/
        
        if(strcmp(tokens->items[0], "mkdir")== 0)
        {
           if(tokens->size < 2)
           {
            printf("There is no directory name specified. \n");
           }
           else{
            mkdir(tokens->items[1]);
           }
        }
        

        free(input);
        free_tokens(tokens);
    }

    return 0;
}
