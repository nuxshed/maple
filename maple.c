//       .\^/.
//    . |`|/| .
//    |\|\|'|/|
// .--'-\`|/-''--.
//  \`-._\|./.-'/
//   >`-._|/.-'<
//  '~|/~~|~~\|~'
//        |
//     maple.c
//  nuxsh - v0.5

#include <dirent.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>
#include <time.h>
#include <errno.h>

#define MAX_ITEMS 1024
#define MAX_PATH_LEN 4096
#define MAX_SEARCH_LEN 256

typedef struct {
  char name[MAX_PATH_LEN];
  int is_dir;
  off_t size;
  time_t mtime;
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
void preview_file(char *current_dir);

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
  if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
    endwin();
    fprintf(stderr, "Error getting current directory\n");
    return 1;
  }

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
    mvprintw(0, 0, "Error: Cannot open directory %s", current_dir);
    refresh();
    return;
  }

  num_items = 0;
  errno = 0;
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

    if (stat(full_path, &file_stat) == -1) {
      continue;
    }

    if (num_items >= MAX_ITEMS) {
      mvprintw(0, 0, "Warning: Directory has too many items, some are not shown");
      refresh();
      break;
    }

    strncpy(items[num_items].name, entry->d_name, MAX_PATH_LEN - 1);
    items[num_items].name[MAX_PATH_LEN - 1] = '\0';
    items[num_items].is_dir = S_ISDIR(file_stat.st_mode);
    items[num_items].size = file_stat.st_size;
    items[num_items].mtime = file_stat.st_mtime;

    num_items++;
  }

  if (errno != 0) {
    mvprintw(0, 0, "Error reading directory: %s", strerror(errno));
    refresh();
  }

  closedir(dp);
}

void draw_interface(char *current_dir) {
  clear();

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  mvprintw(0, 0, "%s", current_dir);
  mvprintw(1, 0, "search: %s", search_term);

  int start_y = 3;
  int end_y = max_y - 2;
  int displayable_items = end_y - start_y;

  if (current_selection < scroll_offset) {
    scroll_offset = current_selection;
  } else if (current_selection >= scroll_offset + displayable_items) {
    scroll_offset = current_selection - displayable_items + 1;
  }

  int size_width = 10;
  int time_width = 12;
  int name_width = max_x - size_width - time_width - 7;

  for (int i = scroll_offset; i < num_items && i < scroll_offset + displayable_items; i++) {
    int y = i - scroll_offset + start_y;
    char size_str[32];
    if (items[i].is_dir) {
      snprintf(size_str, sizeof(size_str), "<DIR>");
    } else if (items[i].size < 1024) {
      snprintf(size_str, sizeof(size_str), "%ldB", items[i].size);
    } else if (items[i].size < 1024 * 1024) {
      snprintf(size_str, sizeof(size_str), "%.1fK", items[i].size / 1024.0);
    } else if (items[i].size < 1024 * 1024 * 1024) {
      snprintf(size_str, sizeof(size_str), "%.1fM", items[i].size / 1024.0 / 1024.0);
    } else {
      snprintf(size_str, sizeof(size_str), "%.1fG", items[i].size / 1024.0 / 1024.0 / 1024.0);
    }

    char time_str[32];
    strftime(time_str, sizeof(time_str), "%b %d %H:%M ", localtime(&items[i].mtime));

    if (i == current_selection) {
      attron(A_REVERSE);
    }

    if (items[i].is_dir) {
      attron(COLOR_PAIR(2));
    }
    
    char display_name[MAX_PATH_LEN];
    strncpy(display_name, items[i].name, sizeof(display_name) - 1);
    display_name[sizeof(display_name) - 1] = '\0';
    
    if ((int)strlen(display_name) > name_width) {
      display_name[name_width - 3] = '.';
      display_name[name_width - 2] = '.';
      display_name[name_width - 1] = '.';
      display_name[name_width] = '\0';
    }
    
    mvprintw(y, 2, " %-*s %*s  %*s", 
             name_width, display_name,
             size_width, size_str,
             time_width, time_str);
    
    if (items[i].is_dir) {
      attroff(COLOR_PAIR(2));
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
  case 10:
    if (num_items > 0 && items[current_selection].is_dir) {
      char new_path[MAX_PATH_LEN];
      snprintf(new_path, sizeof(new_path), "%s/%s", current_dir, items[current_selection].name);
      strncpy(current_dir, new_path, MAX_PATH_LEN - 1);
      current_dir[MAX_PATH_LEN - 1] = '\0';
      current_selection = 0;
      scroll_offset = 0;
    } else if (num_items > 0) {
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
  case 'p':
    preview_file(current_dir);
    break;
  }

  draw_interface(current_dir);
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
  move(1, 8);
  
  char temp_search[MAX_SEARCH_LEN];
  if (getnstr(temp_search, MAX_SEARCH_LEN - 1) == OK) {
    strncpy(search_term, temp_search, MAX_SEARCH_LEN - 1);
    search_term[MAX_SEARCH_LEN - 1] = '\0';
  }
  
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
  snprintf(command, sizeof(command), "xdg-open \"%s\"", full_path);
  
  def_prog_mode();
  endwin();
  int ret = system(command);
  if (ret == -1) {
    mvprintw(0, 0, "Error executing command");
    refresh();
  }
  reset_prog_mode();
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

void preview_file(char *current_dir) {
  if (num_items == 0 || items[current_selection].is_dir) {
    return;
  }

  char full_path[MAX_PATH_LEN];
  snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, items[current_selection].name);

  FILE *file = fopen(full_path, "r");
  if (!file) {
    mvprintw(0, 0, "Error opening file for preview");
    refresh();
    return;
  }

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  WINDOW *preview_win = newwin(max_y - 4, max_x - 4, 2, 2);
  wrefresh(preview_win);

  char line[256];
  int line_num = 0;
  int ch;
  int scroll_offset = 0;

  while (1) {
    werase(preview_win);

    fseek(file, 0, SEEK_SET);
    for (int i = 0; i < scroll_offset; i++) {
      fgets(line, sizeof(line), file);
    }

    line_num = 1;
    while (fgets(line, sizeof(line), file) && line_num < max_y - 5) {
      if (strlen(line) > max_x - 10) {
        line[max_x - 10] = '\0';
      }
      mvwprintw(preview_win, line_num, 1, "%4d  %s", line_num + scroll_offset, line);
      line_num++;
    }

    box(preview_win, 0, 0);
    wrefresh(preview_win);

    ch = wgetch(preview_win);
    if (ch == 'p' || ch == 27) {
      break;
    } else if (ch == KEY_UP || ch == 'k') {
      if (scroll_offset > 0) {
        scroll_offset--;
      }
    } else if (ch == KEY_DOWN || ch == 'j') {
      scroll_offset++;
    }
  }

  fclose(file);
  delwin(preview_win);
  clear();
  refresh();
}