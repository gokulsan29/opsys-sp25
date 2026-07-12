#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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
      printf("dirwatch: Unsupported entry type: %d\n", type);
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
    printf("dirwatch: Error getting info for file %s: %s\n", filename, strerror(errno));
    exit(1);
  }
  errno = 0;
  struct passwd *user = getpwuid(st.st_uid);
  char const* owner_name;
  if (user == NULL) {
    if (errno != 0) {
      printf("dirwatch: Error getting info for owner for file %s: %s\n",
             filename, strerror(errno));
      exit(1);
    }
    owner_name = "(user-not-found)";
  }
  owner_name = user->pw_name;
  char contents[PATH_MAX];
  unsigned char file_type = entry->d_type;
  if (file_type == DT_DIR) {
    strcpy(contents, "(directory)");
  }
  else if (file_type == DT_LNK) {
    int bytes_read = readlink(path, contents, PATH_MAX);
    if (bytes_read < 0) {
      printf("dirwatch: Error reading link %s: %s\n", filename, strerror(errno));
      exit(1);
    }
  }
  else if (file_type == DT_REG) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
      printf("dirwatch: Error opening file %s: %s\n", filename, strerror(errno));
      exit(1);
    }
    int bytes_read = read(fd, contents, sizeof(contents));
    if (bytes_read < 0) {
      printf("dirwatch: Error reading from file %s: %s\n", filename, strerror(errno));
      close(fd);
      exit(1);
    }
    for (int i = 0;i < bytes_read;i++) {
      if (contents[i] == '\n') {
        contents[i] = '\0';
        break;
      }
      if (!isprint((unsigned char)contents[i])) {
        contents[i] = '#';
      }
    }
    contents[PATH_MAX - 1] = '\0';
    close(fd);
  }
  else {
    printf("Unsupported entry type: %d\n", file_type);
    exit(1);
  }
  int contents_len = ws.ws_col - 64;
  if (contents_len < 0) {
    contents_len = 0;
  }
  printf("%-20.20s %8ld B %-5.5s %04o %-20.20s %.*s\n",
         filename,
         st.st_size,
         entry_type_to_str(file_type),
         st.st_mode & 07777,
         owner_name,
         contents_len,
         contents);
}

void
print_footer(size_t files, size_t dirs, size_t links)
{
  char const* files_str = files == 1 ? "file" : "files";
  char const* dirs_str = dirs == 1 ? "directory" : "directories";
  char const* links_str = links == 1 ? "symlink" : "symlinks";
  printf("total: %ld %s, %ld %s and %ld %s\n",
         files,
         files_str,
         dirs,
         dirs_str,
         links,
         links_str);
}

void
dirwatch(char const* pathname)
{
  DIR* dir_ptr = opendir(pathname);
  if (!dir_ptr) {
    printf("dirwatch: Unable to open %s: %s\n", pathname, strerror(errno));
    exit(1);
  }
  get_terminal_info();
  clear_screen();
  print_header(pathname);
  int max_entries = ws.ws_row - 9;
  int curr_entry = 0;
  size_t files = 0, dirs = 0, links = 0;
  struct dirent* entry;
  while (1) {
    if (curr_entry >= max_entries) {
      printf("(truncated)\n");
      break;
    }
    errno = 0;
    entry = readdir(dir_ptr);
    if (entry == NULL) {
      if (errno != 0) {
        printf("dirwatch: Error while reading dir %s: %s\n", pathname, strerror(errno));
        exit(1);
      }
      break;
    }
    char const* filename = entry->d_name;
    if (!strcmp(filename, ".") || !strcmp(filename, "..")) {
      continue;
    }
    print_dir_entry(entry, pathname);
    switch (entry->d_type) {
      case DT_REG: files++; break;
      case DT_DIR: dirs++; break;
      case DT_LNK: links++; break;
    }
    curr_entry++;
  }
  print_dashes();
  printf("\n");
  print_footer(files, dirs, links);
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

