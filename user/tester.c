#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void run_program(char *name) {
    printf("\n>>>>>>>> POKRECEM: %s <<<<<<<<\n", name);
    int pid = fork();

    if(pid < 0) {
        printf("Greska: Fork nije uspeo za %s\n", name);
        return;
    }

    if(pid == 0) {
        char *argv[] = {name, 0};
        exec(name, argv);
        printf("Greska: Nije moguce pokrenuti program %s!\n", name);
        exit();
    }

    wait();
}

int main() {
    printf("\n========================================================\n");
    printf("   GLAVNI TESTER (Automatsko pokretanje svih testova) \n");
    printf("========================================================\n");

    run_program("/bin/testerkernel");

    run_program("/bin/testeruser");

    printf("\n========================================================\n");
    printf("   SVI TESTOVI SU ZAVRSENI! \n");
    printf("========================================================\n\n");

    exit();
}
