#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char buffer[1760];
int buf_len = 0;
char filename[64];
int is_modified = 0;
int quit_confirmed = 0;
int cursor_pos = 0;

char cmd_msg[80];
int show_stats = 0;

struct fileblks {
    int blocks[140];
    int num_blocks;
    int last_block_free;
};
struct fileblks current_fb;

void draw_ui() {
    char row[81];
    int i, j;

    set_cursor(0);
    char *p = "--- EDITOR: ";
    int k = 0;
    while(*p) row[k++] = *p++;
    p = filename;
    while(*p) row[k++] = *p++;
    p = " ---";
    while(*p) row[k++] = *p++;
    while(k < 80) row[k++] = ' ';
    row[80] = 0;
    printf("%s", row);

    int cur_r = 1;
    int cur_c = 0;
    int t_pos = 80;
    set_cursor(80);

    for(i=0; i<buf_len; i++){
        if(i == cursor_pos) t_pos = cur_r * 80 + cur_c;
        if(buffer[i] == '\n'){
            while(cur_c < 80) row[cur_c++] = ' ';
            row[80] = 0;
            printf("%s", row);
            cur_r++;
            cur_c = 0;
            if(cur_r > 22) break;
            set_cursor(cur_r * 80);
        } else {
            row[cur_c++] = buffer[i];
            if(cur_c == 80){
                row[80] = 0;
                printf("%s", row);
                cur_r++;
                cur_c = 0;
                if(cur_r > 22) break;
                set_cursor(cur_r * 80);
            }
        }
    }

    if(cursor_pos == buf_len) t_pos = cur_r * 80 + cur_c;

    if(cur_r <= 22){
        while(cur_c < 80) row[cur_c++] = ' ';
        row[80] = 0;
        printf("%s", row);
        cur_r++;
    }

    for(i=0; i<80; i++) row[i] = ' ';
    row[80] = 0;
    while(cur_r <= 22){
        set_cursor(cur_r * 80);
        printf("%s", row);
        cur_r++;
    }

    set_cursor(23 * 80);
    for(i=0; i<80; i++) row[i] = ' ';
    row[80] = 0;
    printf("%s", row);

    set_cursor(23 * 80);
    if(show_stats == 1){
        printf("Blokovi: ");
        for(j=0; j<5 && j<current_fb.num_blocks; j++){
            printf("%d ", current_fb.blocks[j]);
        }
    } else if(show_stats == 2){
        printf("Statistika nije dostupna");
    } else {
        int free_blocks = get_free_blocks();
        printf("Slobodno: %d blk | $: Komande (save, stats, quit)", free_blocks);
    }

    set_cursor(24 * 80);
    for(i=0; i<79; i++) row[i] = ' ';
    row[79] = 0;
    printf("%s", row);

    set_cursor(24 * 80);
    if(cmd_msg[0] != 0) printf("%s", cmd_msg);

    if(t_pos >= 23 * 80) t_pos = 23 * 80 - 1;
    set_cursor(t_pos);
}

void handle_command() {
    char cmd_buffer[64];
    cmd_buffer[0] = 0;
    int cmd_len = 0;
    char row[80];
    int i;

    quit_confirmed = 0;

    while(1){
        set_cursor(24 * 80);
        for(i=0; i<79; i++) row[i] = ' ';
        row[79] = 0;
        printf("%s", row);

        set_cursor(24 * 80);
        printf("Komanda: %s", cmd_buffer);

        char c;
        if(read(0, &c, 1) <= 0) break;

        if(c == '\n') {
            break;
        } else if(c == 127 || c == '\b') {
            if(cmd_len > 0) cmd_buffer[--cmd_len] = 0;
        } else if(c >= 32 && c <= 126) {
            if(cmd_len < 63) {
                cmd_buffer[cmd_len++] = c;
                cmd_buffer[cmd_len] = 0;
            }
        }
    }

    if(strcmp(cmd_buffer, "quit") == 0){
        if(!is_modified || quit_confirmed){
            set_cursor(0);
            for(i=0; i<80; i++) row[i] = ' ';
            row[80] = 0;
            for(i=0; i<24; i++) {
                set_cursor(i * 80);
                printf("%s", row);
            }
            row[79] = 0;
            set_cursor(24 * 80);
            printf("%s", row);
            set_cursor(0);
            exit();
        } else {
            strcpy(cmd_msg, "Nije sacuvano! Kucaj 'quit' ponovo.");
            quit_confirmed = 1;
            return;
        }
    } else {
        quit_confirmed = 0;

        if(strcmp(cmd_buffer, "save") == 0){
            int ret = write_path(filename, buffer, buf_len);
            if(ret == 0){
                is_modified = 0;
                show_stats = 0;
                strcpy(cmd_msg, "Sacuvano uspesno.");
            } else if (ret == -3){
                strcpy(cmd_msg, "Nema mesta na disku!");
            } else {
                strcpy(cmd_msg, "Greska pri cuvanju!");
            }
        }
        else if(strcmp(cmd_buffer, "stats") == 0){
            if(!is_modified){
                int fd = open(filename, 0);
                if(fd >= 0){
                    if(get_file_blocks(fd, &current_fb) == 0){
                        show_stats = 1;
                    } else {
                        show_stats = 2;
                    }
                    close(fd);
                } else {
                    show_stats = 2;
                }
            } else {
                show_stats = 2;
            }
            cmd_msg[0] = 0;
        }
        else if (cmd_len == 0){
            cmd_msg[0] = 0;
        }
        else {
            strcpy(cmd_msg, "Nepoznata komanda.");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("editor <filename>\n");
        exit();
    }

    strcpy(filename, argv[1]);
    cmd_msg[0] = 0;

    int n = read_path(filename, buffer);
    if (n >= 0) {
        if (n > 1760) n = 1760;
        buf_len = n;
        cursor_pos = n;
    } else {
        buf_len = 0;
        cursor_pos = 0;
    }

    char row[81];
    int i;
    for(i=0; i<80; i++) row[i] = ' ';
    row[80] = 0;
    for(i=0; i<24; i++) {
        set_cursor(i * 80);
        printf("%s", row);
    }
    row[79] = 0;
    set_cursor(24 * 80);
    printf("%s", row);
    set_cursor(0);

    while(1) {
        draw_ui();

        char c;
        if(read(0, &c, 1) <= 0) break;

        if(c == '$') {
            handle_command();
        }
        else if(c == 127 || c == '\b') {
            if(cursor_pos > 0) {
                for(i = cursor_pos - 1; i < buf_len - 1; i++) {
                    buffer[i] = buffer[i + 1];
                }
                cursor_pos--;
                buf_len--;
                is_modified = 1;
                quit_confirmed = 0;
                cmd_msg[0] = 0;
                show_stats = 0;
            }
        }
        else if(c == '\n' || (c >= 32 && c <= 126)) {
            if(buf_len < 1760) {
                for(i = buf_len; i > cursor_pos; i--) {
                    buffer[i] = buffer[i - 1];
                }
                buffer[cursor_pos++] = c;
                buf_len++;
                is_modified = 1;
                quit_confirmed = 0;
                cmd_msg[0] = 0;
                show_stats = 0;
            }
        }
    }

    exit();
}
