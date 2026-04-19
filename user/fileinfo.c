#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"

int main(int argc, char*argv[])
{
    if(argc < 2 || (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)){
        printf("fileinfo <path>\n");
        exit();
    }

    int fd =open(argv[1], O_RDONLY);
    if(fd < 0){
        printf("Fajl ne postoji ili ne moze da se otvori\n");
        exit();
    }

    struct fileblks fb;
    if(get_file_blocks(fd, &fb) < 0){
        printf("sistemski poziv nije uspeo\n");
        close(fd);
        exit();
    }

    int total_size = (fb.num_blocks - 1) * BSIZE;
    if(fb.num_blocks > 0)
        total_size += (BSIZE - fb.last_block_free);

    printf("broj blokova: %d\n", fb.num_blocks);
    printf("preostalo bajtova u poslednjem bloku: %d\n", fb.last_block_free);
    printf("Ukupna velicina fajla: %d B\n", total_size);
    printf("Adrese blokova: ");
        for(int i = 0; i < fb.num_blocks && i < 5; i++)
            printf("%d ", fb.blocks[i]);
        printf("\n");

    close(fd);
    exit();

}
