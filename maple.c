//       .\^/.
//    . |`|/| .
//    |\|\|'|/|
// .--'-\`|/-''--.
//  \`-._\|./.-'/
//   >`-._|/.-'<
//  '~|/~~|~~\|~'
//        |
//     maple.c
//  nuxsh - v0.2

#include <dirent.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_ITEMS 1024
#define MAX_PATH_LEN 4096

typedef struct {
  char name[MAX_PATH_LEN];
  int is_dir;
  int marked_for_deletion;
} FileItem;

FileItem items[MAX_ITEMS];
int num_items = 0;
int current_selection = 0;

void draw_interface(char *current_dir);
void list_files(char *current_dir);
void navigate(char *current_dir);
void delete_marked_files(char *current_dir);

int main() {
  initscr();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(FALSE);
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);

  char current_dir[MAX_PATH_LEN];
  getcwd(current_dir, sizeof(current_dir));

  while (1) {
    list_files(current_dir);
    draw_interface(current_dir);
    navigate(current_dir);
  }

  endwin();
  return 0;
}

void list_files(char *current_dir) {
  struct dirent *entry;
  DIR *dp = opendir(current_dir);
  struct stat file_stat;

  if (dp == NULL) {
    return;
  }

  num_items = 0;
  while ((entry = readdir(dp)) && num_items < MAX_ITEMS) {
    if (strcmp(entry->d_name, ".") == 0) {
      continue;
    }

    char full_path[MAX_PATH_LEN];
    snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, entry->d_name);

    stat(full_path, &file_stat);

    strncpy(items[num_items].name, entry->d_name, MAX_PATH_LEN - 1);
    items[num_items].is_dir = S_ISDIR(file_stat.st_mode);
    items[num_items].marked_for_deletion = 0;

    num_items++;
  }

  closedir(dp);
}

void draw_interface(char *current_dir) {
  clear();

  mvprintw(0, 0, "Current Directory: %s", current_dir);

  for (int i = 0; i < num_items; i++) {
    if (i == current_selection) {
      attron(A_REVERSE);
    }
    if (items[i].marked_for_deletion) {
      attron(COLOR_PAIR(1));
    }

    if (items[i].is_dir) {
      mvprintw(i + 2, 2, "├── %s/", items[i].name);
    } else {
      mvprintw(i + 2, 2, "├── %s", items[i].name);
    }

    if (items[i].marked_for_deletion) {
      attroff(COLOR_PAIR(1));
    }
    if (i == current_selection) {
      attroff(A_REVERSE);
    }
  }

  refresh();
}

void navigate(char *current_dir) {
  int ch = getch();

  switch (ch) {
  case KEY_UP:
  case 'k':
    if (current_selection > 0) {
      current_selection--;
    }
    break;
  case KEY_DOWN:
  case 'j':
    if (current_selection < num_items - 1) {
      current_selection++;
    }
    break;
  case KEY_RIGHT:
  case 'l':
  case 10: // Enter key
    if (items[current_selection].is_dir) {
      if (strcmp(items[current_selection].name, "..") == 0) {
        char *last_slash = strrchr(current_dir, '/');
        if (last_slash != NULL && last_slash != current_dir) {
          *last_slash = '\0';
        }
      } else {
        strcat(current_dir, "/");
        strcat(current_dir, items[current_selection].name);
      }
      current_selection = 0;
    }
    break;
  case KEY_LEFT:
  case 'h':
  case KEY_BACKSPACE:
  {
    char *last_slash = strrchr(current_dir, '/');
    if (last_slash != NULL && last_slash != current_dir) {
      *last_slash = '\0';
    }
    current_selection = 0;
  } break;
  case 'd':
    items[current_selection].marked_for_deletion = !items[current_selection].marked_for_deletion;
    break;
  case 'D':
    delete_marked_files(current_dir);
    break;
  case 'q':
    endwin();
    exit(0);
    break;
  }
}

void delete_marked_files(char *current_dir) {
  for (int i = 0; i < num_items; i++) {
    if (items[i].marked_for_deletion) {
      char full_path[MAX_PATH_LEN];
      snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, items[i].name);
      
      if (items[i].is_dir) {
        rmdir(full_path);
      } else {
        remove(full_path);
      }
    }
  }
  list_files(current_dir);
}
