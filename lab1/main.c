#include <stdio.h>
#include "pa1.h"
#include "process.h"

int main(int argc, char *argv[]) {

  int n = parse_flag(argc, argv);
  const int parent_pid = getpid();

  for (int i = 0; i < n; ++i) {
    if (fork() == 0) {
      printf(log_started_fmt, i + 1, getpid(), parent_pid);
      printf(log_done_fmt, i + 1);
    } else {
      return 0;
    }
  }
}
