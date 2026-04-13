#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char*argv[])
{
    if(argc > 1 && strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "--h") == 0){
        printf("freeblocks prikazuje broj slobonih blokova na disku\n");
        exit();
    }

    int blocks = get_free_blocks();
    if(blocks < 0){
        printf("Greska pri dohvatanju slobodnih blokova\n");
        exit();
    }

    int kb  = blocks * 512/ 1024; // pretvara u KB
    printf("slobodnih blokova: %d (%d KB)\n", blocks, kb);
    exit();
}
