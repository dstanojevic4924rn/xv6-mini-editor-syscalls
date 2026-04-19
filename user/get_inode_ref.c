#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char*argv[])
{
    if(argc > 1 && strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0){
        printf("funkcija prikazuje broj referenci u memoriji za fajl \n");
        printf("upotreba: get_inode_ref <ime_mog_fajla.fajl> [OPCIJA]\n");
        printf("Opcije:\n");
        printf(" -h i --help prikazuju ovaj help meni\n");
        exit();
    }
    char *name = argv[1];

    int blocks = get_inode_ref(name);


    if(blocks < 0){
        printf("Greska\n");
        exit();
    }

    printf("broj referenci: %d\n", blocks);
    exit();
}
