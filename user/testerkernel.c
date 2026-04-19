#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "kernel/file.h"


#define ASSERT(cond, msg, points_x10) \
if(cond) { \
    printf("[OK] %s (+%d.%d pts)\n", msg, (points_x10)/10, (points_x10)%10); \
    total_pts += points_x10; \
} else { \
    printf("[FAIL] %s\n", msg); \
}

int total_pts = 0;
char big_buf[8000];
char local_buf[8500];

void test_cursor() {
    printf("\n--- Testiranje Kursora (1.0 pts) ---\n");
    int old_pos = get_cursor();
    int res;

    // 1. Test sinhronizacije
    res = set_cursor(500);
    ASSERT(res == 0 && get_cursor() == 500, "set_cursor/get_cursor sinhronizacija (pos 500)", 4);

    // 2. Test donje granice (Pozicija 0)
    res = set_cursor(0);
    ASSERT(res == 0 && get_cursor() == 0, "Granicna vrednost: Pocetak ekrana (pos 0)", 2);

    // 3. Test gornje granice (Pozicija 1999)
    res = set_cursor(1999);
    ASSERT(res == 0 && get_cursor() == 1999, "Granicna vrednost: Kraj ekrana (pos 1999)", 2);

    // 4. Test nevalidnih vrednosti (Error handling)
    int s_neg = set_cursor(-1);
    int s_big = set_cursor(2000);

    set_cursor(0);

    ASSERT(s_neg == -1 && s_big == -1, "Rukovanje greskama (negativne i prevelike vrednosti)", 2);

    set_cursor(old_pos);
}

void test_get_free_blocks() {
    printf("\n--- Testiranje Slobodnih Blokova (1.0 pts) ---\n");

    // 1. Uzimamo pocetno stanje
    int b1 = get_free_blocks();
    if (b1 == -1) {
        printf("[FAIL] get_free_blocks vratio -1 (greska u sistemskom pozivu).\n");
        return;
    }

    // 2. Kreiramo fajl i upisujemo podatke
    int fd = open("test_free.tmp", O_CREATE | O_WRONLY);
    if(fd < 0) {
        printf("[FAIL] Standardni open nije uspeo, preskacem test.\n");
        return;
    }

    // 512 bajtova da osiguramo promenu u bitmapi
    char temp_data[512];
    memset(temp_data, 'x', 512);
    if(write(fd, temp_data, 512) != 512) {
        printf("[FAIL] Standardni write nije uspeo.\n");
        close(fd);
        return;
    }
    close(fd);

    // 3. Provera smanjenja (0.5 pts)
    int b2 = get_free_blocks();
    ASSERT(b1 > b2, "Smanjenje broja blokova nakon standardnog upisa", 5);

    // 4. Brisanje fajla koristeci unlink
    if(unlink("test_free.tmp") < 0) {
        printf("[FAIL] Standardni unlink nije uspeo.\n");
        return;
    }

    // 5. Provera vracanja blokova (0.5 pts)
    int b3 = get_free_blocks();
    ASSERT(b3 > b2 && b3 == b1, "Vracanje blokova na pocetno stanje nakon unlink poziva", 5);

    // 6. Provera konzistentnosti
    int bc1 = get_free_blocks();
    int bc2 = get_free_blocks();
    ASSERT(bc1 > 0 && bc1 == bc2, "Konzistentnost: uzastopni pozivi bez promena", 0);

    // 7. Alokacija vise blokova (3584 bajtova = 7 blokova )
    int bm1 = get_free_blocks();
    int fd_m = open("multi.tmp", O_CREATE | O_WRONLY);
    if(fd_m >= 0) {
        write(fd_m, big_buf, 3584);
        close(fd_m);
        int bm2 = get_free_blocks();
        ASSERT(bm1 - bm2 >= 7, "Alokacija vise blokova odjednom (3584 bajtova)", 0);
        unlink("multi.tmp");
    }
}

void test_get_file_blocks() {
    printf("\n--- Testiranje get_file_blocks (2.0 pts) ---\n");
    struct fileblks fb;
    int fd;

    // 1. Test malog fajla (0.5 pts)
    // Upisujemo 100 bajtova - last_block_free = 512 - 100 = 412
    fd = open("small.txt", O_CREATE | O_WRONLY);
    write(fd, big_buf, 100);
    close(fd);

    fd = open("small.txt", O_RDONLY);
    if (get_file_blocks(fd, &fb) == 0) {
        ASSERT(fb.num_blocks == 1 && fb.last_block_free == 412, "Mali fajl: num_blocks i last_block_free", 3);
        ASSERT(fb.blocks[0] > 0, "Validnost adrese: blocks[0] je validan index na disku", 0);
    }

    close(fd);
    unlink("small.txt");

    // 2. Test fajla koji je tacno velicine jednog bloka (0.5 pts)
    // last_block_free treba da bude 0
    fd = open("exact.txt", O_CREATE | O_WRONLY);
    write(fd, big_buf, 512);
    close(fd);

    fd = open("exact.txt", O_RDONLY);
    if (get_file_blocks(fd, &fb) == 0) {
        ASSERT(fb.num_blocks == 1 && fb.last_block_free == 0, "Granica: Fajl od tacno 512B (last_free=0)", 3);
    }
    close(fd);
    unlink("exact.txt");

    // 3. Test indirektnih blokova (1.0 pts)
    // Upisujemo 14 blokova (12 direktnih + 2 indirektna)
    // 14 * 512 = 7168 bajtova
    fd = open("big.txt", O_CREATE | O_WRONLY);
    write(fd, big_buf, 7168);
    close(fd);

    fd = open("big.txt", O_RDONLY);
    if (get_file_blocks(fd, &fb) == 0) {
        int indirect_ok = (fb.blocks[12] > 0 && fb.blocks[13] > 0);
        ASSERT(fb.num_blocks == 14 && indirect_ok, "Indirektni: Detekcija blokova preko 12", 8);
    } else {
        printf("[FAIL] get_file_blocks vratio gresku za veliki fajl.\n");
    }
    close(fd);
    unlink("big.txt");

    // 4. Test nevalidnog deskriptora
    // int res_err = get_file_blocks(999, &fb);
    // ASSERT(res_err == -1, "Error handling: Nevalidan FD vraca -1", 0);

    // 4. Test nevalidnog deskriptora
    unlink("pass.tmp");
    if(fork() == 0) {
        int res_err = get_file_blocks(999, &fb);
        if(res_err == -1) {
            int tmp = open("pass.tmp", O_CREATE | O_WRONLY);
            write(tmp, "1", 1);
            close(tmp);
        }
        exit();
    }
    wait();
    fd = open("pass.tmp", O_RDONLY);
    if(fd >= 0) {
        ASSERT(1, "Error handling: Nevalidan FD vraca -1", 0);
        close(fd);
        unlink("pass.tmp");
    } else {
        ASSERT(0, "Error handling: Nevalidan FD izazvao TRAP 14 ili nije -1", 0);
    }


    // 5. Provera oslobadjanja blokova (Sanity check)
    // Proveravamo da li se broj slobodnih blokova vraca na pocetno stanje nakon brisanja
    int b_before = get_free_blocks();

    fd = open("cleanup.tmp", O_CREATE | O_WRONLY);
    write(fd, big_buf, 1024);
    close(fd);

    int b_with_file = get_free_blocks();
    unlink("cleanup.tmp");
    int b_after = get_free_blocks();

    ASSERT(b_with_file < b_before && b_after == b_before, "Oslobadjanje resursa: Blokovi uspesno vraceni u bitmapu", 0);

    // 6. Test praznog fajla (0.2 pts)
    fd = open("empty.txt", O_CREATE | O_WRONLY);
    if (get_file_blocks(fd, &fb) == 0) {
        ASSERT(fb.num_blocks == 0, "Prazan fajl: num_blocks je 0", 2);
    }
    close(fd);
    unlink("empty.txt");

    // 7. Test zatvorenog deskriptora (0.2 pts)
    fd = open("closed.txt", O_CREATE | O_WRONLY);
    close(fd);
    int res_closed = get_file_blocks(fd, &fb);
    ASSERT(res_closed == -1, "Greska: Zatvoren deskriptor vraca -1", 2);
    unlink("closed.txt");

    // 8. Test tacne granice direktnih blokova (12 blokova)
    fd = open("twelve.txt", O_CREATE | O_WRONLY);
    for(int i = 0; i < 12; i++) write(fd, big_buf, 512);
    close(fd);
    fd = open("twelve.txt", O_RDONLY);
    if(get_file_blocks(fd, &fb) == 0) {
        // blocks[12] je adresa indirektnog bloka, za 12 blokova mora biti 0
        ASSERT(fb.num_blocks == 12 && fb.blocks[12] == 0, "Granica: Tacno 12 direktnih blokova", 1);
    }
    close(fd);
    unlink("twelve.txt");

    // 9. Test prosledjivanja NULL pokazivaca

    // fd = open("null_test.txt", O_CREATE | O_WRONLY);
    // int res_null = get_file_blocks(fd, 0);
    // ASSERT(res_null == -1, "Zastita: NULL pokazivac strukture vraca -1", 1);
    // close(fd);
    // unlink("null_test.txt");

    // 9. Test prosledjivanja NULL pokazivaca
    fd = open("null_test.txt", O_CREATE | O_WRONLY);
    close(fd);
    fd = open("null_test.txt", O_RDONLY);
    unlink("pass.tmp");
    if(fork() == 0) {
        int res_null = get_file_blocks(fd, 0);
        if(res_null == -1) {
            int tmp = open("pass.tmp", O_CREATE | O_WRONLY);
            write(tmp, "1", 1);
            close(tmp);
        }
        exit();
    }
    wait();
    int fd_check = open("pass.tmp", O_RDONLY);
    if(fd_check >= 0) {
        ASSERT(1, "Zastita: NULL pokazivac strukture vraca -1", 1);
        close(fd_check);
        unlink("pass.tmp");
    } else {
        ASSERT(0, "Zastita: NULL pokazivac izazvao TRAP 14 ili nije -1", 1);
    }
    close(fd);
    unlink("null_test.txt");


    // 10. Provera oslobadjanja
    b_before = get_free_blocks();
    fd = open("cleanup.tmp", O_CREATE | O_WRONLY);
    write(fd, big_buf, 1024);
    close(fd);
    unlink("cleanup.tmp");
    b_after = get_free_blocks();
    ASSERT(b_after == b_before, "Sanity: Sistem ne gubi blokove nakon brisanja", 0);
}

void test_read_path() {
    printf("\n--- Testiranje read_path (2.0 pts) ---\n");
    int n, fd;

    // 1. Nepostojeca putanja
    n = read_path("fajl_koji_ne_postoji_123", local_buf);
    ASSERT(n == -1, "Nepostojeca putanja vraca -1", 3);

    // 2. T_DEV tip
    n = read_path("/dev/console", local_buf);
    ASSERT(n == -2, "T_DEV (console) vraca -2", 3);

    // 3. Prazan fajl
    fd = open("empty.tmp", O_CREATE | O_WRONLY);
    close(fd);
    n = read_path("empty.tmp", local_buf);
    ASSERT(n == 0, "Prazan fajl vraca 0 procitanih bajtova", 2);
    unlink("empty.tmp");

    // 4. Mali fajl
    char *msg = "Sistemsko Programiranje 2026";
    fd = open("small.tmp", O_CREATE | O_WRONLY);
    write(fd, msg, strlen(msg));
    close(fd);
    memset(local_buf, 0, sizeof(local_buf));
    n = read_path("small.tmp", local_buf);
    ASSERT(n == strlen(msg) && strcmp(local_buf, msg) == 0, "Ispravno citanje i integritet malog fajla", 3);
    unlink("small.tmp");

    // 5. Binarna bezbednost
    char binary_data[] = { 'A', 0, 'B', 0, 'C' };
    fd = open("bin.tmp", O_CREATE | O_WRONLY);
    write(fd, binary_data, 5);
    close(fd);
    memset(local_buf, 0, sizeof(local_buf));
    n = read_path("bin.tmp", local_buf);
    int bin_ok = (n == 5 && local_buf[0] == 'A' && local_buf[1] == 0 && local_buf[2] == 'B');
    ASSERT(bin_ok, "Binarna bezbednost (ne staje na nulu)", 3);
    unlink("bin.tmp");

    // 6. Veliki fajl (Indirektni blokovi - 8000 bajtova)
    fd = open("big.tmp", O_CREATE | O_WRONLY);
    for(int i = 0; i < 8000; i++) big_buf[i] = 'X' + (i % 3);
    write(fd, big_buf, 8000);
    close(fd);
    memset(local_buf, 0, sizeof(local_buf));
    n = read_path("big.tmp", local_buf);
    int big_ok = (n == 8000 && local_buf[0] == 'X' && local_buf[7999] == 'X' + (7999 % 3));
    ASSERT(big_ok, "Citanje preko indirektnih blokova (8000 bajtova)", 3);
    unlink("big.tmp");

    // 7. Zastita kernela: NULL pokazivac
    // Sistemski poziv ne sme da srusi kernel, vec da vrati -1
    // n = read_path("console", 0);
    // ASSERT(n == -1 || n == -2, "Zastita: Prosledjivanje NULL bafera", 3);

    // 7. Zastita kernela: NULL pokazivac
    unlink("pass.tmp");
    if(fork() == 0) {
        n = read_path("console", 0);
        if(n == -1 || n == -2) {
            int tmp = open("pass.tmp", O_CREATE | O_WRONLY);
            write(tmp, "1", 1);
            close(tmp);
        }
        exit();
    }
    wait();
    int fd_c = open("pass.tmp", O_RDONLY);
    if(fd_c >= 0) {
        ASSERT(1, "Zastita: Prosledjivanje NULL bafera", 3);
        close(fd_c);
        unlink("pass.tmp");
    } else {
        ASSERT(0, "Zastita: Prosledjivanje NULL bafera izazvalo TRAP 14", 3);
    }


}

void test_write_path() {
    printf("\n--- Testiranje write_path (2.0 pts) ---\n");

    int res, fd;

    // 1. Kreiranje nove datoteke (0.2 pts)
    res = write_path("new_file.tmp", "Sistemi", 7);
    ASSERT(res == 0, "Uspesno kreiranje nove datoteke", 2);
    unlink("new_file.tmp");

    // 2. Upis 0 bajtova
    // Provera da li moze da napravi prazan fajl
    res = write_path("zero.tmp", "", 0);
    ASSERT(res == 0, "Upis 0 bajtova (prazan fajl)", 1);
    unlink("zero.tmp");

    // 3. T_DEV tip  (0.2 pts)
    // Ne sme se dozvoliti upis u konzolu preko write_path
    res = write_path("/dev/console", "test", 4);
    ASSERT(res == -2, "T_DEV (console) vraca -2", 3);

    // 4. Prebrisavanje i oslobadjanje blokova - itrunc (0.4 pts)
    // Prvo napravimo veliki fajl (npr. 15 blokova)
    for(int i = 0; i < 8000; i++) big_buf[i] = 'A';
    write_path("trunc.tmp", big_buf, 8000);
    int blocks_before = get_free_blocks();

    // Prebrisujemo ga malim sadrzajem (1 blok)
    // write_path mora pozvati itrunc i osloboditi prethodne blokove
    write_path("trunc.tmp", "Mali", 4);
    int blocks_after = get_free_blocks();
    ASSERT(blocks_after > blocks_before, "Prebrisavanje uspesno oslobadja stare blokove", 4);
    unlink("trunc.tmp");

    // 5. Provera slobodnih blokova - greska -3 (0.3 pts)
    // Pokusavamo da upisemo vise nego sto ima mesta na disku
    int free_blocks = get_free_blocks();
    // Pokusaj upisa: (slobodni blokovi + 5) * 512 bajtova
    // Koristimo veliki broj da osiguramo -3 povratnu vrednost
    res = write_path("too_big.tmp", big_buf, (free_blocks + 5) * 512);
    ASSERT(res == -3, "Nedostatak prostora vraca -3 pre izmene", 3);

    // 6. Binarna bezbednost i integritet (0.3 pts)
    char binary_data[] = { 'X', 0, 'Y', 0, 'Z' };
    write_path("bin_w.tmp", binary_data, 5);

    fd = open("bin_w.tmp", O_RDONLY);
    char check_buf[5];
    read(fd, check_buf, 5);
    close(fd);
    int bin_ok = (check_buf[0] == 'X' && check_buf[1] == 0 && check_buf[2] == 'Y');
    ASSERT(bin_ok, "Integritet binarnih podataka (upis nule)", 2);
    unlink("bin_w.tmp");

    // 7. Upis preko indirektnih blokova (0.3 pts)
    for(int i = 0; i < 8000; i++) big_buf[i] = 'B';
    res = write_path("indirect_w.tmp", big_buf, 8000);

    struct fileblks fb;
    fd = open("indirect_w.tmp", O_RDONLY);
    get_file_blocks(fd, &fb);
    close(fd);
    // 8000/512 = 15.6 -> 16 blokova
    ASSERT(res == 0 && fb.num_blocks == 16, "Upis preko indirektnih blokova (8000 bajtova)", 3);
    unlink("indirect_w.tmp");

    // 8. Zastita kernela: Nevalidna putanja (0.1 pts)
    res = write_path("/direktorijum_koji_ne_postoji/f.txt", "x", 1);
    ASSERT(res == -1, "Nevalidna putanja vraca -1", 1);

    // 9. Inode Leak Test (0.2 pts)
    // Uzastopno kreiranje i brisanje da vidimo da li sistem ostaje bez inode-ova
    int leak_ok = 1;
    for(int i = 0; i < 100; i++) {
        if(write_path("leak.tmp", "a", 1) != 0) {
            leak_ok = 0;
            break;
        }
        unlink("leak.tmp");
    }
    ASSERT(leak_ok, "Stabilnost sistema nakon 100 uzastopnih upisa/brisanja", 1);
}


void test_read_write_integration() {
    printf("\n--- Integracioni Sanity Check (read_path + write_path) ---\n");
    char *fname = "sanity.tmp";
    char *data = "Integracioni test 2026";
    char buf[64];

    int b_before = get_free_blocks();

    int res_w = write_path(fname, data, strlen(data));

    int b_after_w = get_free_blocks();

    memset(buf, 0, sizeof(buf));
    int res_r = read_path(fname, buf);

    // - write_path vratio 0
    // - broj blokova se smanjio (b_after_w < b_before)
    // - sadrzaj se poklapa
    int match = (res_r == strlen(data) && strcmp(data, buf) == 0);
    int disk_changed = (b_after_w < b_before);

    if(!disk_changed) {
        printf("[DEBUG] Blokovi pre: %d, posle upisa: %d (Nema promene!)\n", b_before, b_after_w);
    }

    ASSERT(res_w == 0 && match && disk_changed, "Sinergija: write_path -> disk_change -> read_path", 0);

    unlink(fname);
}



int main() {
    printf("ZAPOCINJEM AUTOMATSKO OCENJIVANJE...\n");

    test_cursor();         // 1.0 pts (0.5+0.5)
    test_get_free_blocks();  // 1.0 pts
    test_get_file_blocks();  // 2.0 pts
    test_read_path(); // 2.0 pts
    test_write_path(); // 2.0 pts
    test_read_write_integration(); // 0.0 pts


    printf("\n====================================\n");
    printf("REZULTAT SISTEMSKIH POZIVA: %d.%d / 8.0\n", total_pts / 10, total_pts % 10);
    printf("Preostali poeni:\n");
    printf("- freeblocks (korisnicki program): 1.0\n");
    printf("- fileinfo   (korisnicki program): 1.0\n");
    printf("- editor     (korisnicki program): 3.0\n");
    printf("------------------------------------\n");
    printf("UKUPNO MOGUCE: 13.0 poena\n");
    printf("====================================\n");

    exit();
}
