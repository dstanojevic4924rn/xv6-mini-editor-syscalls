#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int total_pts = 0;
char buf[4096];


char to_lower(char c) {
    if(c >= 'A' && c <= 'Z') return c + 32;
    return c;
}


int contains(const char *text, const char *pattern) {
    if(!text || !pattern) return 0;
    int t_len = strlen(text);
    int p_len = strlen(pattern);
    if(p_len == 0) return 1;
    for(int i = 0; i <= t_len - p_len; i++) {
        int j = 0;
        while(j < p_len && to_lower(text[i+j]) == to_lower(pattern[j])) j++;
        if(j == p_len) return 1;
    }
    return 0;
}


#define ASSERT(cond, msg, points_x10) \
if(cond) { \
    printf("[OK] %s (+%d.%d pts)\n", msg, (points_x10)/10, (points_x10)%10); \
    total_pts += points_x10; \
} else { \
    printf("[FAIL] %s\n", msg); \
}


void read_file(char *filename) {
    memset(buf, 0, sizeof(buf));
    int fd = open(filename, O_RDONLY);
    if(fd >= 0) {
        int n = read(fd, buf, sizeof(buf)-1);
        if(n >= 0) buf[n] = '\0';
        close(fd);
    }
}

void create_file(char *filename, char *content) {
    int fd = open(filename, O_CREATE | O_WRONLY);
    if(fd >= 0) {
        write(fd, content, strlen(content));
        close(fd);
    }
}


void run_test_cmd(char *prog, char *arg1, char *arg2) {
    unlink("out.tmp");
    int pid = fork();
    if(pid == 0) {
        close(1); close(2);
        open("out.tmp", O_CREATE | O_WRONLY);
        dup(1);

        char *argv[4] = {prog, arg1, arg2, 0};
        exec(prog, argv);
        exit();
    }
    wait();
}



void run_editor_cmd(char *arg, char *input_keys) {
    unlink("out.tmp");
    create_file("in.tmp", input_keys);

    int editor_pid = fork();
    if(editor_pid == 0) {
        close(0); open("in.tmp", O_RDONLY);
        close(1); close(2);
        open("out.tmp", O_CREATE | O_WRONLY);
        dup(1);

        if(arg == 0) {
            char *argv_no[2] = {"editor", 0};
            exec("/bin/editor", argv_no);
        } else {
            char *argv[3] = {"editor", arg, 0};
            exec("/bin/editor", argv);
        }
        exit();
    }


    int timer_pid = fork();
    if(timer_pid == 0) {
        sleep(100);
        kill(editor_pid);
        exit();
    }

    int finished_pid = wait();

    if(finished_pid == editor_pid) {
        kill(timer_pid);
        wait();
    } else {
        printf(" [!] TIME-OUT: Editor se zaglavio (verovatno Enter/Backspace bag).\n");
    }

    unlink("in.tmp");
}

int main() {
    printf("\n====================================\n");
    printf("   ZAPOCINJEM AUTOMATSKO TESTIRANJE \n");
    printf("====================================\n");

    // ==========================================
    // 1. FREEBLOCKS (Max 1.0 poen)
    // ==========================================
    printf("\n--- Testiranje: freeblocks ---\n");

    run_test_cmd("/bin/freeblocks", 0, 0);
    read_file("out.tmp");
    printf("\n[ISPIS] freeblocks:\n%s\n", buf);
    ASSERT(contains(buf, "KB") || contains(buf, "blocks") || contains(buf, "slobodno"), "freeblocks bez argumenata (prikazuje blocks i KB)", 6);

    run_test_cmd("/bin/freeblocks", "-h", 0);
    read_file("out.tmp");
    printf("\n[ISPIS] freeblocks -h:\n%s\n", buf);
    ASSERT(contains(buf, "help") || contains(buf, "sluzi") || contains(buf, "usage"), "freeblocks -h prikazuje help", 2);

    run_test_cmd("/bin/freeblocks", "--help", 0);
    read_file("out.tmp");
    printf("\n[ISPIS] freeblocks --help:\n%s\n", buf);
    ASSERT(contains(buf, "help") || contains(buf, "sluzi") || contains(buf, "usage"), "freeblocks --help prikazuje help", 2);

    // ==========================================
    // 2. FILEINFO (Max 1.0 poen)
    // ==========================================
    printf("\n--- Testiranje: fileinfo ---\n");

    run_test_cmd("/bin/fileinfo", 0, 0);
    read_file("out.tmp");
    printf("\n[ISPIS] fileinfo:\n%s\n", buf);
    ASSERT(contains(buf, "help") || contains(buf, "usage"), "fileinfo bez argumenata prikazuje help", 2);

    run_test_cmd("/bin/fileinfo", "-h", 0);
    read_file("out.tmp");
    printf("\n[ISPIS] fileinfo -h:\n%s\n", buf);
    int fi_h1 = contains(buf, "help");

    run_test_cmd("/bin/fileinfo", "--help", 0);
    read_file("out.tmp");
    printf("\n[ISPIS] fileinfo --help:\n%s\n", buf);
    int fi_h2 = contains(buf, "help");
    ASSERT(fi_h1 && fi_h2, "fileinfo -h i --help ispravno rade", 2);

    run_test_cmd("/bin/fileinfo", "x_nepostojeci.txt", 0);
    read_file("out.tmp");
    printf("\n[ISPIS] fileinfo (nepostojeci fajl):\n%s\n", buf);
    ASSERT(contains(buf, "ne postoji") || contains(buf, "greska") || contains(buf, "uspeo") || contains(buf, "error"), "fileinfo hendluje nepostojecu datoteku", 3);

    create_file("test_info.txt", "123456789012345"); // fajl od 15 bajtova
    run_test_cmd("/bin/fileinfo", "test_info.txt", 0);
    read_file("out.tmp");
    printf("\n[ISPIS] fileinfo test_info.txt:\n%s\n", buf);
    ASSERT(contains(buf, "15") || contains(buf, "bajt") || contains(buf, "blok") || contains(buf, "adres"), "fileinfo ispisuje detalje za validan fajl", 3);


    // ==========================================
    // 3. EDITOR (Max 3.0 poena)
    // ==========================================
    printf("\n--- Testiranje: editor ---\n");

    // 3.1 Help logike
    run_editor_cmd(0, "$quit\n");
    read_file("out.tmp");
    printf("\n[ISPIS] editor (bez argumenata):\n%s\n", buf);
    int ed_h0 = contains(buf, "help") || contains(buf, "usage");

    run_editor_cmd("-h", "$quit\n");
    read_file("out.tmp");
    printf("\n[ISPIS] editor -h:\n%s\n", buf);
    int ed_h1 = contains(buf, "help");

    run_editor_cmd("--help", "$quit\n");
    read_file("out.tmp");
    int ed_h2 = contains(buf, "help");
    printf("\n[ISPIS] editor --help:\n%s\n", buf);
    ASSERT(ed_h0 && ed_h1 && ed_h2, "editor bez arg, -h i --help otvara help meni", 4);

    // 3.2 Kucanje, kreiranje fajla i Save komanda
    unlink("f_novi.txt");
    run_editor_cmd("f_novi.txt", "Kucam u novi fajl\n$save\n$quit\n");
    read_file("f_novi.txt");
    ASSERT(contains(buf, "Kucam u novi fajl"), "editor kucanje, kreiranje fajla i komanda save", 8);

    // 3.3 Backspace i Enter (i prepisivanje na postojeci fajl)
    run_editor_cmd("f_novi.txt", "Linija 2\nTestx\bi\n$save\n$quit\n");
    read_file("f_novi.txt");
    ASSERT(contains(buf, "Linija 2") && contains(buf, "Testi") && !contains(buf, "Testx"), "editor hendluje backspace i enter", 5);

    // 3.4 Izlazak uz nesacuvane izmene i dupla potvrda
    run_editor_cmd("f_upoz.txt", "Izmena koja nece biti sacuvana$quit\n$quit\n");
    read_file("out.tmp");
    ASSERT(contains(buf, "upozorenje") || contains(buf, "potvrd") || contains(buf, "nesa") || contains(buf, "save"), "editor ispisuje upozorenje za nesacuvane izmene (2x quit)", 8);

    // 3.5 Status linija / stats komanda
    //  Za nesacuvan fajl
    run_editor_cmd("f_temp.txt", "A$stats\n$quit\n$quit\n");
    read_file("out.tmp");
    int st1 = contains(buf, "nije") || contains(buf, "dostupn") || contains(buf, "sacuva");

    // Za sacuvan fajl
    run_editor_cmd("f_novi.txt", "$stats\n$quit\n");
    read_file("out.tmp");
    int st2 = contains(buf, "blok") || contains(buf, "adrese") || contains(buf, "0x") || contains(buf, "0");
    ASSERT(st1 && st2, "editor stats komanda (za sacuvane i nesacuvane fajlove)", 5);


    // ==========================================
    // CISCENJE SMECA I REZULTATI
    // ==========================================
    unlink("out.tmp");
    unlink("in.tmp");
    unlink("test_info.txt");
    unlink("f_novi.txt");
    unlink("f_upoz.txt");
    unlink("f_temp.txt");

    printf("\n====================================\n");
    printf("UKUPAN BROJ BODOVA: %d.%d / 5.0\n", total_pts/10, total_pts%10);
    printf("====================================\n");

    exit();
}
