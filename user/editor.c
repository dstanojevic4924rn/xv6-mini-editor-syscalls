#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

#define MAX_FILE_SIZE ((12+128)*512)
#define SCREEN_ROWS 22
#define SCREEN_COLS 80
#define STATUS_ROW 23
#define CMD_ROW 24

static char buffer[MAX_FILE_SIZE];
static int bufsize = 0;
static int cursor = 0;
static int modified = 0;
static char filename[256];

void set_cursor_pos(int row, int col) {
    set_cursor(row * SCREEN_COLS + col);
}

void clear_screen() {
    for (int row = 0; row < 25; row++) {
        set_cursor_pos(row, 0);
        for (int col = 0; col < SCREEN_COLS; col++)
            printf(" ");
    }
}

void draw_header() {
    set_cursor_pos(0, 0);
    printf("File: %s", filename);
    int len = strlen(filename) + 6;
    for (int i = len; i < SCREEN_COLS; i++) printf(" ");
}

void draw_status() {
    set_cursor_pos(STATUS_ROW, 0);
    int freeblk = get_free_blocks();
    printf("Free blocks: %d   [$] command", freeblk);
    for (int i = 0; i < SCREEN_COLS - 25; i++) printf(" ");
}

void index_to_pos(int idx, int *row, int *col) {
    *row = 1;
    *col = 0;
    for (int i = 0; i < idx && i < bufsize; i++) {
        if (buffer[i] == '\n') {
            (*row)++;
            *col = 0;
        } else {
            (*col)++;
            if (*col >= SCREEN_COLS) {
                (*row)++;
                *col = 0;
            }
        }
    }
}

void full_refresh() {
    for (int row = 1; row <= SCREEN_ROWS; row++) {
        set_cursor_pos(row, 0);
        for (int col = 0; col < SCREEN_COLS; col++)
            printf(" ");
    }

    int pos = 0, row = 1, col = 0;
    while (pos < bufsize && row <= SCREEN_ROWS) {
        set_cursor_pos(row, col);
        if (buffer[pos] == '\n') {
            row++;
            col = 0;
            if (row <= SCREEN_ROWS)
                set_cursor_pos(row, col);
        } else {
            printf("%c", buffer[pos]);
            col++;
            if (col >= SCREEN_COLS) {
                col = 0;
                row++;
                if (row <= SCREEN_ROWS)
                    set_cursor_pos(row, col);
            }
        }
        pos++;
    }

    int cr, cc;
    index_to_pos(cursor, &cr, &cc);
    if (cr > SCREEN_ROWS) cr = SCREEN_ROWS;
    if (cc >= SCREEN_COLS) cc = SCREEN_COLS - 1;
    set_cursor_pos(cr, cc);
}

void insert_char(char c) {
    if (bufsize >= MAX_FILE_SIZE - 1) return;
    for (int i = bufsize; i > cursor; i--)
        buffer[i] = buffer[i-1];
    buffer[cursor] = c;
    bufsize++;
    cursor++;
    modified = 1;
    draw_header();
    draw_status();
    full_refresh();
}

void delete_char() {
    if (cursor > 0) {
        for (int i = cursor-1; i < bufsize-1; i++)
            buffer[i] = buffer[i+1];
        bufsize--;
        cursor--;
        modified = 1;
        draw_header();
        draw_status();
        full_refresh();
    }
}

void insert_newline() {
    insert_char('\n');
}

void save_file() {
    int ret = write_path(filename, buffer, bufsize);
    if (ret == 0) {
        modified = 0;
        draw_header();
        draw_status();
        full_refresh();
    } else if (ret == -3) {
        set_cursor_pos(CMD_ROW, 0);
        printf("No free blocks on disk!           ");
    } else {
        set_cursor_pos(CMD_ROW, 0);
        printf("Write error!                      ");
    }
}

void show_stats() {
    if (modified) {
        set_cursor_pos(CMD_ROW, 0);
        printf("File not saved yet - stats unavailable.  ");
        return;
    }
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        set_cursor_pos(CMD_ROW, 0);
        printf("Cannot open file for stats.              ");
        return;
    }
    struct fileblks fb;
    if (get_file_blocks(fd, &fb) < 0) {
        set_cursor_pos(CMD_ROW, 0);
        printf("get_file_blocks failed.                  ");
    } else {
        set_cursor_pos(STATUS_ROW, 0);
        printf("Blocks: %d, last block free: %d          ",
               fb.num_blocks, fb.last_block_free);
        set_cursor_pos(CMD_ROW, 0);
        printf("First 5 block addresses: ");
        for (int i = 0; i < fb.num_blocks && i < 5; i++)
            printf("%d ", fb.blocks[i]);
        printf("          ");
    }
    close(fd);
    draw_header();
    draw_status();
    full_refresh();
}

void command_mode() {
    char cmd[32];
    int i = 0;
    set_cursor_pos(CMD_ROW, 0);
    printf(": ");
    while (1) {
        char ch;
        read(0, &ch, 1);
        if (ch == '\n') {
            cmd[i] = '\0';
            break;
        } else if (ch == '\b' && i > 0) {
            i--;
            set_cursor_pos(CMD_ROW, 2 + i);
            printf(" ");
            set_cursor_pos(CMD_ROW, 2 + i);
        } else if (ch >= 32 && ch < 127 && i < 31) {
            cmd[i++] = ch;
            printf("%c", ch);
        }
    }
    if (strcmp(cmd, "save") == 0) {
        save_file();
    } else if (strcmp(cmd, "stats") == 0) {
        show_stats();
    } else if (strcmp(cmd, "quit") == 0) {
        if (modified) {
            set_cursor_pos(CMD_ROW, 0);
            printf("Unsaved changes! Type 'quit' again to exit.  ");
            char confirm[32];
            int j = 0;
            set_cursor_pos(CMD_ROW, 0);
            printf(": ");
            while (1) {
                char ch;
                read(0, &ch, 1);
                if (ch == '\n') {
                    confirm[j] = '\0';
                    break;
                } else if (ch == '\b' && j > 0) {
                    j--;
                    set_cursor_pos(CMD_ROW, 2 + j);
                    printf(" ");
                    set_cursor_pos(CMD_ROW, 2 + j);
                } else if (ch >= 32 && ch < 127 && j < 31) {
                    confirm[j++] = ch;
                    printf("%c", ch);
                }
            }
            if (strcmp(confirm, "quit") == 0)
                exit();
        } else {
            exit();
        }
    } else {
        set_cursor_pos(CMD_ROW, 0);
        printf("Unknown command: %s          ", cmd);
    }
    draw_header();
    draw_status();
    full_refresh();
}

int main(int argc, char *argv[]) {
    if (argc < 2 || (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("editor <filename>\n");
        exit();
    }
    strcpy(filename, argv[1]);

    int n = read_path(filename, buffer);
    if (n >= 0) {
        bufsize = n;
        buffer[bufsize] = '\0';
    } else if (n == -1) {
        bufsize = 0;
    } else if (n == -2) {
        printf("Cannot edit device file.\n");
        exit();
    } else {
        printf("Read error.\n");
        exit();
    }
    modified = 0;
    cursor = 0;

    clear_screen();
    draw_header();
    draw_status();
    full_refresh();

    char ch;
    while (1) {
        read(0, &ch, 1);
        if (ch == '$') {
            command_mode();
        } else if (ch == '\b' || ch == 127) {
            delete_char();
        } else if (ch == '\n') {
            insert_newline();
        } else if (ch >= 32 && ch < 127) {
            insert_char(ch);
        }
    }
    exit();
}
