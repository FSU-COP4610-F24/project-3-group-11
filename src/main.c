#include "commands.h"
#include "commands.c"

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
        

        free(input);
        free_tokens(tokens);
    }

    return 0;
}
