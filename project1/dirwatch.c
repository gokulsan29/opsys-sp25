#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

struct winsize ws;

void
get_terminal_info()
{
  int ret = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  if (ret < 0) {
    printf("dirwatch: Error in reading terminal info: %s", strerror(errno));
    exit(1);
  }
}

void
clear_screen()
{
  for (int r = 0;r < ws.ws_row;r++) {
    for (int c = 0;c < ws.ws_col;c++) {
      putchar(' ');
    }
    putchar('\n');
  }
}

void
print_dashes()
{
  for (int c = 0;c < ws.ws_col;c++) {
    putchar('-');
  }
  putchar('\n');
}

void
print_header(char const* pathname)
{
  time_t now = time(NULL);
  printf("Contents of %s on %s\n\n", pathname, ctime(&now));
  printf("%-20s %10s %-5s %-4s %-20s %s\n", "NAME", "SIZE", "TYPE", "MODE", "OWNER", "CONTENTS");
  print_dashes();
}

char const*
entry_type_to_str(unsigned char type)
{
  switch (type)
  {
    case DT_DIR: return "dir";
    case DT_LNK: return "link";
    case DT_REG: return "file";
    default:
      printf("Unsupported entry type: %d", type);
      exit(1);
  }
}

void
print_dir_entry(struct dirent* entry, char const* dirpath)
{
  char path[PATH_MAX];
  char const* filename = entry->d_name;
  if ((strlen(filename) + strlen(dirpath) + 2) > PATH_MAX) {
    printf("dirwatch: path to file %s is too long\n", filename);
    exit(1);
  }
  snprintf(path, PATH_MAX, "%s/%s", dirpath, filename);
  struct stat st;
  int status = lstat(path, &st);
  if (status < 0) {
    printf("dirwatch: Error getting info for file %s: %s", filename, strerror(errno));
    exit(1);
  }
  errno = 0;
  struct passwd *user = getpwuid(st.st_uid);
  char const* owner_name;
  if (user == NULL) {
    if (errno != 0) {
      printf("dirwatch: Error getting info for owner for file %s: %s",
             filename, strerror(errno));
      exit(1);
    }
    owner_name = "(user-not-found)";
  }
  owner_name = user->pw_name;
  printf("%-20s %8ld B %-5s %04o %-20s %s\n",
         filename,
         st.st_size,
         entry_type_to_str(entry->d_type),
         st.st_mode & 07777,
         owner_name,
         "CONTENTS");
}

void
dirwatch(char const* pathname)
{
  DIR* dir_ptr = opendir(pathname);
  if (!dir_ptr) {
    printf("dirwatch: Unable to open %s: %s", pathname, strerror(errno));
    exit(1);
  }
  get_terminal_info();
  clear_screen();
  print_header(pathname);
  struct dirent* entry;
  while (1) {
    errno = 0;
    entry = readdir(dir_ptr);
    if (entry == NULL) {
      if (errno != 0) {
        printf("dirwatch: Error while reading dir %s: %s", pathname, strerror(errno));
        exit(1);
      }
      break;
    }
    char const* filename = entry->d_name;
    if (!strcmp(filename, ".") || !strcmp(filename, "..")) {
      continue;
    }
    print_dir_entry(entry, pathname);
  }
  print_dashes();
}

int
main(int argc, char* argv[])
{
  if (argc != 2) {
    printf("Usage: ./dirwatch <pathname>\n");
    exit(1);
  }
  char const* pathname = argv[1];
  while (1) {
    dirwatch(pathname);
    sleep(3);
  }
}

