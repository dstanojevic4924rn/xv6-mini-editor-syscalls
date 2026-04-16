// test_syscalls.c
// Prosireni unit testovi za OS2026 D2

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int total = 0;
int passed = 0;

#define PASS(name) do { printf("[PASS] %s\n", name); passed++; total++; } while(0)
#define FAIL(name, msg) do { printf("[FAIL] %s: %s\n", name, msg); total++; } while(0)
#define CHECK(cond, name, msg) do { total++; if(cond) { printf("[PASS] %s\n", name); passed++; } else { printf("[FAIL] %s: %s\n", name, msg); } } while(0)

#define BSIZE 512


// =================== get_free_blocks ===================

void test_gfb_returns_positive() {
    int n = get_free_blocks();
    CHECK(n > 0, "gfb_returns_positive", "mora biti > 0");
}

void test_gfb_consistent() {
    int a = get_free_blocks();
    int b = get_free_blocks();
    CHECK(a == b, "gfb_consistent", "razliciti rezultati bez write-a");
}

void test_gfb_decreases_on_write() {
    int before = get_free_blocks();
    char buf[2048];
    for(int i = 0; i < 2048; i++) buf[i] = 'X';
    int ret = write_path("gfb1.tmp", buf, 2048);
    if(ret != 0){ FAIL("gfb_decreases", "write failed"); unlink("gfb1.tmp"); return; }
    int after = get_free_blocks();
    CHECK(after < before, "gfb_decreases_on_write", "nije opao");
    unlink("gfb1.tmp");
}

void test_gfb_increases_on_delete() {
    char buf[2048];
    for(int i = 0; i < 2048; i++) buf[i] = 'Y';
    write_path("gfb2.tmp", buf, 2048);
    int before = get_free_blocks();
    unlink("gfb2.tmp");
    int after = get_free_blocks();
    CHECK(after > before, "gfb_increases_on_delete", "nije porastao");
}


// =================== read_path ===================

void test_rp_nonexistent() {
    char buf[100];
    int ret = read_path("nonexistent_abc.xyz", buf);
    CHECK(ret == -1, "rp_nonexistent", "ne vraca -1");
}

void test_rp_reads_full() {
    char *msg = "Zdravo xv6 OS test!";
    int len = strlen(msg);
    write_path("rp1.tmp", msg, len);

    char buf[100];
    int n = read_path("rp1.tmp", buf);
    if(n != len){ FAIL("rp_reads_full", "duzina"); unlink("rp1.tmp"); return; }
    for(int i = 0; i < len; i++){
        if(buf[i] != msg[i]){ FAIL("rp_reads_full", "sadrzaj"); unlink("rp1.tmp"); return; }
    }
    PASS("rp_reads_full");
    unlink("rp1.tmp");
}

void test_rp_empty_file() {
    write_path("rp2.tmp", "", 0);
    char buf[10];
    int n = read_path("rp2.tmp", buf);
    CHECK(n == 0, "rp_empty", "nije 0");
    unlink("rp2.tmp");
}

void test_rp_large_indirect() {
    char *buf = malloc(8000);
    for(int i = 0; i < 8000; i++) buf[i] = 'A' + (i % 26);
    int w = write_path("rp_big.tmp", buf, 8000);
    if(w != 0){ FAIL("rp_large_indirect", "write failed"); free(buf); unlink("rp_big.tmp"); return; }

    char *rbuf = malloc(8000);
    int n = read_path("rp_big.tmp", rbuf);
    if(n != 8000){ FAIL("rp_large_indirect", "duzina"); free(buf); free(rbuf); unlink("rp_big.tmp"); return; }
    for(int i = 0; i < 8000; i++){
        if(rbuf[i] != buf[i]){ FAIL("rp_large_indirect", "sadrzaj"); free(buf); free(rbuf); unlink("rp_big.tmp"); return; }
    }
    PASS("rp_large_indirect");
    free(buf); free(rbuf);
    unlink("rp_big.tmp");
}

void test_rp_binary_content() {
    char data[20];
    for(int i = 0; i < 20; i++) data[i] = i;
    write_path("rp_bin.tmp", data, 20);

    char buf[20];
    int n = read_path("rp_bin.tmp", buf);
    if(n != 20){ FAIL("rp_binary", "duzina"); unlink("rp_bin.tmp"); return; }
    for(int i = 0; i < 20; i++){
        if(buf[i] != (char)i){ FAIL("rp_binary", "sadrzaj"); unlink("rp_bin.tmp"); return; }
    }
    PASS("rp_binary_content");
    unlink("rp_bin.tmp");
}

void test_rp_device() {
    char *devpath = 0;
    unlink("rp_dev.tmp");
    if(mknod("rp_dev.tmp", 1, 1) == 0){
        devpath = "rp_dev.tmp";
    } else {
        struct stat st;
        if(stat("console", &st) == 0 && st.type == T_DEV){
            devpath = "console";
        }
    }
    if(devpath == 0){ FAIL("rp_device", "nema T_DEV"); return; }

    char buf[10];
    int ret = read_path(devpath, buf);
    CHECK(ret == -2, "rp_device", "ne vraca -2");
    if(strcmp(devpath, "rp_dev.tmp") == 0) unlink("rp_dev.tmp");
}


// =================== write_path ===================

void test_wp_creates_new() {
    unlink("wp1.tmp");
    int ret = write_path("wp1.tmp", "hello", 5);
    CHECK(ret == 0, "wp_creates_new", "nije kreirao");
    unlink("wp1.tmp");
}

void test_wp_overwrites() {
    write_path("wp2.tmp", "LONG ORIGINAL CONTENT", 21);
    int ret = write_path("wp2.tmp", "short", 5);
    if(ret != 0){ FAIL("wp_overwrites", "write failed"); unlink("wp2.tmp"); return; }

    char buf[100];
    int n = read_path("wp2.tmp", buf);
    CHECK(n == 5, "wp_overwrites", "nije truncovao");
    unlink("wp2.tmp");
}

void test_wp_overwrite_content() {
    write_path("wp3.tmp", "AAAAAAAAAA", 10);
    write_path("wp3.tmp", "BB", 2);
    char buf[10];
    int n = read_path("wp3.tmp", buf);
    if(n != 2){ FAIL("wp_overwrite_content", "duzina"); unlink("wp3.tmp"); return; }
    if(buf[0] != 'B' || buf[1] != 'B'){ FAIL("wp_overwrite_content", "sadrzaj"); unlink("wp3.tmp"); return; }
    PASS("wp_overwrite_content");
    unlink("wp3.tmp");
}

void test_wp_freed_blocks() {
    char big[3072];
    for(int i = 0; i < 3072; i++) big[i] = 'A';
    write_path("wp4.tmp", big, 3072);
    int with_big = get_free_blocks();

    write_path("wp4.tmp", "x", 1);
    int with_small = get_free_blocks();

    CHECK(with_small > with_big, "wp_freed_blocks", "nije oslobodio");
    unlink("wp4.tmp");
}

void test_wp_device() {
    char *devpath = 0;
    unlink("wp_dev.tmp");
    if(mknod("wp_dev.tmp", 1, 1) == 0){
        devpath = "wp_dev.tmp";
    } else {
        struct stat st;
        if(stat("console", &st) == 0 && st.type == T_DEV){
            devpath = "console";
        }
    }
    if(devpath == 0){ FAIL("wp_device", "nema T_DEV"); return; }

    int ret = write_path(devpath, "x", 1);
    CHECK(ret == -2, "wp_device", "ne vraca -2");
    if(strcmp(devpath, "wp_dev.tmp") == 0) unlink("wp_dev.tmp");
}

void test_wp_zero_bytes() {
    int ret = write_path("wp5.tmp", "", 0);
    CHECK(ret == 0, "wp_zero_bytes", "ne moze 0");
    unlink("wp5.tmp");
}

void test_wp_large_indirect() {
    char *buf = malloc(8000);
    for(int i = 0; i < 8000; i++) buf[i] = (i % 256);
    int ret = write_path("wp_big.tmp", buf, 8000);
    CHECK(ret == 0, "wp_large_indirect", "nije upisao veliki fajl");
    free(buf);
    unlink("wp_big.tmp");
}


// =================== get_file_blocks ===================

void test_gfbl_valid() {
    write_path("gfbl1.tmp", "data", 4);
    int fd = open("gfbl1.tmp", 0);
    if(fd < 0){ FAIL("gfbl_valid", "open"); unlink("gfbl1.tmp"); return; }

    struct fileblks fb;
    int ret = get_file_blocks(fd, &fb);
    if(ret != 0){ FAIL("gfbl_valid", "ret"); close(fd); unlink("gfbl1.tmp"); return; }
    if(fb.num_blocks != 1){ FAIL("gfbl_valid", "num_blocks"); close(fd); unlink("gfbl1.tmp"); return; }
    if(fb.blocks[0] <= 0){ FAIL("gfbl_valid", "blocks[0]"); close(fd); unlink("gfbl1.tmp"); return; }
    if(fb.last_block_free != 508){ FAIL("gfbl_valid", "last_block_free"); close(fd); unlink("gfbl1.tmp"); return; }
    PASS("gfbl_valid");
    close(fd); unlink("gfbl1.tmp");
}

void test_gfbl_invalid_fd() {
    struct fileblks fb;
    int ret = get_file_blocks(999, &fb);
    CHECK(ret == -1, "gfbl_invalid_fd", "ne vraca -1");
}

void test_gfbl_negative_fd() {
    struct fileblks fb;
    int ret = get_file_blocks(-1, &fb);
    CHECK(ret == -1, "gfbl_negative_fd", "ne vraca -1");
}

void test_gfbl_closed_fd() {
    write_path("gfbl_c.tmp", "x", 1);
    int fd = open("gfbl_c.tmp", 0);
    close(fd);
    struct fileblks fb;
    int ret = get_file_blocks(fd, &fb);
    CHECK(ret == -1, "gfbl_closed_fd", "ne vraca -1");
    unlink("gfbl_c.tmp");
}

void test_gfbl_multi_block() {
    char buf[1500];
    for(int i = 0; i < 1500; i++) buf[i] = 'M';
    write_path("gfbl2.tmp", buf, 1500);

    int fd = open("gfbl2.tmp", 0);
    struct fileblks fb;
    int ret = get_file_blocks(fd, &fb);
    if(ret != 0){ FAIL("gfbl_multi", "ret"); close(fd); unlink("gfbl2.tmp"); return; }
    CHECK(fb.num_blocks == 3, "gfbl_multi_block", "num_blocks nije 3");
    close(fd); unlink("gfbl2.tmp");
}

void test_gfbl_exact_block_size() {
    char buf[512];
    for(int i = 0; i < 512; i++) buf[i] = 'E';
    write_path("gfbl_e.tmp", buf, 512);

    int fd = open("gfbl_e.tmp", 0);
    struct fileblks fb;
    int ret = get_file_blocks(fd, &fb);
    if(ret != 0){ FAIL("gfbl_exact", "ret"); close(fd); unlink("gfbl_e.tmp"); return; }
    CHECK(fb.last_block_free == 0, "gfbl_exact_block_size",
          "last_block_free nije 0 za fajl tacno BSIZE");
    close(fd); unlink("gfbl_e.tmp");
}

void test_gfbl_empty_file() {
    write_path("gfbl_em.tmp", "", 0);
    int fd = open("gfbl_em.tmp", 0);
    struct fileblks fb;
    int ret = get_file_blocks(fd, &fb);
    if(ret != 0){ FAIL("gfbl_empty", "ret"); close(fd); unlink("gfbl_em.tmp"); return; }
    CHECK(fb.num_blocks == 0, "gfbl_empty_file", "num_blocks nije 0 za prazan");
    close(fd); unlink("gfbl_em.tmp");
}

void test_gfbl_indirect() {
    char *buf = malloc(8000);
    for(int i = 0; i < 8000; i++) buf[i] = (i % 256);
    if(write_path("gfbl_i.tmp", buf, 8000) != 0){
        FAIL("gfbl_indirect", "write failed");
        free(buf); unlink("gfbl_i.tmp"); return;
    }
    free(buf);

    int fd = open("gfbl_i.tmp", 0);
    struct fileblks fb;
    int ret = get_file_blocks(fd, &fb);
    if(ret != 0){ FAIL("gfbl_indirect", "ret"); close(fd); unlink("gfbl_i.tmp"); return; }
    // 8000/512 = 15.625 → 16 blokova
    CHECK(fb.num_blocks == 16, "gfbl_indirect", "num_blocks za 8KB nije 16");
    close(fd); unlink("gfbl_i.tmp");
}

void test_gfbl_last_block_free_partial() {
    write_path("gfbl_p.tmp", "XXXXXXXXXX", 10);
    int fd = open("gfbl_p.tmp", 0);
    struct fileblks fb;
    int ret = get_file_blocks(fd, &fb);
    if(ret != 0){ FAIL("gfbl_partial", "ret"); close(fd); unlink("gfbl_p.tmp"); return; }
    CHECK(fb.last_block_free == 502, "gfbl_last_block_free_partial",
          "nije 502 za 10-byte fajl");
    close(fd); unlink("gfbl_p.tmp");
}


// =================== get_cursor / set_cursor ===================

void test_cursor_get_in_range() {
    int pos = get_cursor();
    CHECK(pos >= 0 && pos < 25*80, "cursor_get_in_range", "van opsega");
}

void test_cursor_set_get() {
    int saved = get_cursor();
    int ret = set_cursor(100);
    if(ret != 0){ FAIL("cursor_set_get", "set failed"); set_cursor(saved); return; }
    int pos = get_cursor();
    CHECK(pos >= 100 && pos <= 101, "cursor_set_get", "pozicija pogresna");
    set_cursor(saved);
}

void test_cursor_invalid_negative() {
    int saved = get_cursor();
    int ret = set_cursor(-1);
    set_cursor(saved);
    CHECK(ret == -1, "cursor_invalid_negative", "ne vraca -1");
}

void test_cursor_invalid_too_large() {
    int saved = get_cursor();
    int ret = set_cursor(25*80);
    int ret2 = set_cursor(100000);
    set_cursor(saved);
    CHECK(ret == -1 && ret2 == -1, "cursor_invalid_too_large", "ne vraca -1");
}

void test_cursor_boundary_zero() {
    int saved = get_cursor();
    int ret = set_cursor(0);
    set_cursor(saved);
    CHECK(ret == 0, "cursor_boundary_zero", "ne prihvata 0");
}

void test_cursor_boundary_max() {
    int saved = get_cursor();
    int ret = set_cursor(25*80 - 1);
    set_cursor(saved);
    CHECK(ret == 0, "cursor_boundary_max", "ne prihvata 25*80-1");
}


// =================== MAIN ===================

int main(int argc, char *argv[]) {
    printf("\n========= PROSIRENI XV6 SYSCALL TESTS =========\n\n");

    printf("--- get_free_blocks ---\n");

    printf("\n--- read_path ---\n");
    test_rp_reads_full();
    test_rp_large_indirect();
    test_rp_binary_content();

    printf("\n--- write_path ---\n");
    test_wp_overwrite_content();
    test_wp_large_indirect();

    printf("\n--- get_file_blocks ---\n");
    test_gfbl_valid();
    test_gfbl_multi_block();
    test_gfbl_exact_block_size();
    test_gfbl_empty_file();
    test_gfbl_indirect();
    test_gfbl_last_block_free_partial();

    printf("\n--- cursor ---\n");
    test_cursor_set_get();
    test_cursor_invalid_negative();
    test_cursor_invalid_too_large();


    printf("\n========= REZULTAT =========\n");
    printf("Prosli: %d / %d\n", passed, total);
    if(passed == total)
        printf("SVI TESTOVI PROLAZE\n");
    else
        printf("NEKI TESTOVI PADAJU\n");

    exit();
}
