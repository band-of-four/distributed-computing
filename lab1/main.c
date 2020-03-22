#include <stdio.h>
#include "utils.h"
#include "pa1.h"
#include "process.h"

int main(int argc, char *argv[]) {

  int n = parse_flag(argc, argv);
  const int parent_pid = getpid();

  Process child_processes[n];

  for (int i = 0; i < n; ++i) {
    child_processes[i].id = i + 1;
    child_processes[i].parent_pid = parent_pid;

    for (int j = i; j < n; ++j) {
      if (i == j) continue;

      int p[2];
      if (pipe(p) < 0) {
        printf("Error while creating pipe.\n");
        return 1;
      }

      child_processes[i].channels[j][0] = p[0]; // i-th process read from j-th process
      child_processes[i].channels[j][1] = p[1]; // i-th process write to j-th process
      child_processes[j].channels[i][0] = p[1]; // j-th process read from i-th process
      child_processes[j].channels[i][1] = p[0]; // j-th process write to i-th process
    }
  }

  for (int i = 0; i < n; ++i) {
    if (fork() == 0) {
      child_processes[i].pid = getpid();
      Process p = child_processes[i];

      printf(log_started_fmt, p.id, p.pid, p.parent_pid);
      printf(log_done_fmt, p.id);
      return 0;
    }
  }
  printf(log_done_fmt, 0);
}
