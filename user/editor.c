#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define ROWS 25
#define COLS 80
#define WORKSPACE_START 1
#define WORKSPACE_END 22
#define STATUS_ROW 23
#define COMMAND_ROW 24
#define MAX_BUF ((WORKSPACE_END - WORKSPACE_START + 1) * COLS)

// Globalne varijable
char buffer[MAX_BUF];
int buf_len = 0;
char filename[64];
int is_modified = 0;
int quit_confirmed = 0;
int cursor_pos = 0;

// Struktura za stats komandu
struct fileblks {
    int blocks[12 + 128];
    int num_blocks;
    int last_block_free;
};

void draw_ui() {
    // Postavljamo kursor na početak i iscrtavamo zaglavlje
    set_cursor(0);
    printf("--- EDITOR: %s ---", filename);
    for(int i = strlen(filename) + 14; i < COLS; i++) printf(" ");

    // Iscrtavanje radne površine (Redovi 1-22)
    // Razmak ' ' umesto '-' da ekran izgleda čisto
    set_cursor(COLS);
    for (int i = 0; i < MAX_BUF; i++) {
        if (i < buf_len) {
            if (buffer[i] == '\n') printf("\n");
            else printf("%c", buffer[i]);
        } else {
            printf(" ");
        }
    }

    // Statusna linija (Red 23)
    set_cursor(STATUS_ROW * COLS);
    int free_blocks = get_free_blocks();
    printf("Slobodno: %d blk | $: Komande (save, stats, quit)", free_blocks);
    for(int i = 50; i < COLS; i++) printf(" ");

    // Vraćamo hardverski kursor na mesto gde korisnik kuca
    set_cursor((WORKSPACE_START * COLS) + cursor_pos);
}

void clear_command_line() {
    set_cursor(COMMAND_ROW * COLS);
    for(int i = 0; i < COLS; i++) printf(" ");
    set_cursor(COMMAND_ROW * COLS);
}

void handle_command() {
    char cmd[64];
    int i = 0;

    clear_command_line();
    printf("Komanda: ");

    while(1) {
        char c;
        if(read(0, &c, 1) <= 0) break;
        if(c == '\n') {
            cmd[i] = '\0';
            break;
        } else if(c == 127 || c == '\b') {
            if(i > 0) {
                i--;
                printf("\b \b");
            }
        } else {
            if(i < 63) {
                cmd[i++] = c;
                printf("%c", c);
            }
        }
    }

    if(strcmp(cmd, "save") == 0) {
        if(write_path(filename, buffer, buf_len) == 0) {
            is_modified = 0;
            quit_confirmed = 0;
            clear_command_line();
            printf("Sacuvano uspesno.");
        } else {
            clear_command_line();
            printf("Greska pri cuvanju!");
        }
    }
    else if(strcmp(cmd, "quit") == 0) {
        if (!is_modified || quit_confirmed) {
            exit();
        } else {
            clear_command_line();
            printf("Nije sacuvano! Kucaj 'quit' ponovo za izlaz.");
            quit_confirmed = 1;
            return;
        }
    }
    else if(strcmp(cmd, "stats") == 0) {
        clear_command_line();
        if(!is_modified) {
            int fd = open(filename, 0);
            if(fd >= 0) {
                struct fileblks fb;
                if(get_file_blocks(fd, &fb) == 0) {
                    printf("Blokovi: ");
                    for(int j = 0; j < 5 && j < fb.num_blocks; j++) {
                        printf("%d ", fb.blocks[j]);
                    }
                }
                close(fd);
            }
        } else {
            printf("Statistika nije dostupna (fajl izmenjen).");
        }
    }

    if(strcmp(cmd, "quit") != 0) quit_confirmed = 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Koriscenje: editor <filename>\n");
        exit();
    }
    strcpy(filename, argv[1]);

    int n = read_path(filename, buffer);
    if (n >= 0) {
        buf_len = n;
        cursor_pos = n;
    } else {
        buf_len = 0;
        cursor_pos = 0;
    }

    while(1) {
        draw_ui();

        char c;
        if(read(0, &c, 1) <= 0) break;

        if(c == '$') {
            handle_command();
        } else if(c == 127 || c == '\b') {
            if(cursor_pos > 0) {
                cursor_pos--;
                buf_len--;
                is_modified = 1;
                quit_confirmed = 0;
            }
        } else if(c == '\n' || (c >= 32 && c <= 126)) {
            if(buf_len < MAX_BUF) {
                buffer[cursor_pos++] = c;
                if(cursor_pos > buf_len) buf_len = cursor_pos;
                is_modified = 1;
                quit_confirmed = 0;
            }
        }
    }

    exit();
}
