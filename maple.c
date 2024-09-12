//       .\^/.
//    . |`|/| .
//    |\|\|'|/|
// .--'-\`|/-''--.
//  \`-._\|./.-'/
//   >`-._|/.-'<
//  '~|/~~|~~\|~'
//        |
//     maple.c
//  nuxsh - v0.4

#include <dirent.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>

#define MAX_ITEMS 1024
#define MAX_PATH_LEN 4096
#define MAX_SEARCH_LEN 256

typedef struct {
  char name[MAX_PATH_LEN];
  int is_dir;
} FileItem;

FileItem items[MAX_ITEMS];
int num_items = 0;
int current_selection = 0;
int show_hidden = 0;
char search_term[MAX_SEARCH_LEN] = "";
int scroll_offset = 0;

void draw_interface(char *current_dir);
void list_files(char *current_dir);
void navigate(char *current_dir);
void toggle_hidden_files(char *current_dir);
void search_files(char *current_dir);
int file_matches_search(const char *filename);
void delete_file(char *current_dir);
void open_file(char *current_dir);
void go_to_parent_dir(char *current_dir);
void go_to_home_dir(char *current_dir);

int main() {
  initscr();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(FALSE);
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);

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
    if (!show_hidden && entry->d_name[0] == '.') {
      continue;
    }

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    if (!file_matches_search(entry->d_name)) {
      continue;
    }

    char full_path[MAX_PATH_LEN];
    snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, entry->d_name);

    stat(full_path, &file_stat);

    strncpy(items[num_items].name, entry->d_name, MAX_PATH_LEN - 1);
    items[num_items].is_dir = S_ISDIR(file_stat.st_mode);

    num_items++;
  }

  closedir(dp);
}

void draw_interface(char *current_dir) {
  clear();

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  mvprintw(0, 0, "Current Directory: %s", current_dir);
  mvprintw(1, 0, "Search: %s", search_term);

  int start_y = 3;
  int end_y = max_y - 2;
  int displayable_items = end_y - start_y;

  if (current_selection < scroll_offset) {
    scroll_offset = current_selection;
  } else if (current_selection >= scroll_offset + displayable_items) {
    scroll_offset = current_selection - displayable_items + 1;
  }

  for (int i = scroll_offset; i < num_items && i < scroll_offset + displayable_items; i++) {
    int y = i - scroll_offset + start_y;
    if (i == current_selection) {
      attron(A_REVERSE);
    }

    if (items[i].is_dir) {
      attron(COLOR_PAIR(2));
      mvprintw(y, 2, "├── %s/", items[i].name);
      attroff(COLOR_PAIR(2));
    } else {
      mvprintw(y, 2, "├── %s", items[i].name);
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
      strcat(current_dir, "/");
      strcat(current_dir, items[current_selection].name);
      current_selection = 0;
      scroll_offset = 0;
    } else {
      open_file(current_dir);
    }
    break;
  case KEY_LEFT:
  case 'h':
    go_to_parent_dir(current_dir);
    break;
  case 'd':
    delete_file(current_dir);
    break;
  case 'H':
    toggle_hidden_files(current_dir);
    break;
  case '/':
    search_files(current_dir);
    break;
  case '~':
    go_to_home_dir(current_dir);
    break;
  case 'q':
  case 27:
    endwin();
    exit(0);
    break;
  }
}

void delete_file(char *current_dir) {
  char full_path[MAX_PATH_LEN];
  snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, items[current_selection].name);
  
  if (items[current_selection].is_dir) {
    rmdir(full_path);
  } else {
    remove(full_path);
  }
  
  list_files(current_dir);
  if (current_selection >= num_items && num_items > 0) {
    current_selection = num_items - 1;
  }
}

void toggle_hidden_files(char *current_dir) {
  show_hidden = !show_hidden;
  current_selection = 0;
  scroll_offset = 0;
  list_files(current_dir);
}

void search_files(char *current_dir) {
  echo();
  curs_set(TRUE);
  mvprintw(1, 8, "                                        ");
  mvprintw(1, 8, "");
  getnstr(search_term, MAX_SEARCH_LEN);
  noecho();
  curs_set(FALSE);
  current_selection = 0;
  scroll_offset = 0;
  list_files(current_dir);
}

int file_matches_search(const char *filename) {
  if (strlen(search_term) == 0) {
    return 1;
  }
  
  char lower_filename[MAX_PATH_LEN];
  char lower_search[MAX_SEARCH_LEN];
  
  for (int i = 0; filename[i]; i++) {
    lower_filename[i] = tolower(filename[i]);
  }
  lower_filename[strlen(filename)] = '\0';
  
  for (int i = 0; search_term[i]; i++) {
    lower_search[i] = tolower(search_term[i]);
  }
  lower_search[strlen(search_term)] = '\0';
  
  return strstr(lower_filename, lower_search) != NULL;
}

void open_file(char *current_dir) {
  char full_path[MAX_PATH_LEN];
  snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, items[current_selection].name);
  
  char command[MAX_PATH_LEN + 10];
  snprintf(command, sizeof(command), "xdg-open '%s'", full_path);
  
  endwin();
  system(command);
  refresh();
}

void go_to_parent_dir(char *current_dir) {
  char *last_slash = strrchr(current_dir, '/');
  if (last_slash != NULL && last_slash != current_dir) {
    *last_slash = '\0';
  }
  current_selection = 0;
  scroll_offset = 0;
}

void go_to_home_dir(char *current_dir) {
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  strncpy(current_dir, homedir, MAX_PATH_LEN);
  current_dir[MAX_PATH_LEN - 1] = '\0';
  current_selection = 0;
  scroll_offset = 0;
}
