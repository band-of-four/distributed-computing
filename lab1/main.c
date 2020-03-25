#include <stdio.h>
#include <fcntl.h>
#include "utils.h"
#include "pa1.h"
#include "ipc.h"
#include "common.h"
#include "process.h"

int main(int argc, char *argv[]) {

  int n = parse_flag(argc, argv); // TODO: при n = 0 кидает seqfault
  const int parent_pid = getpid();
  FILE *file_pipe = fopen(pipes_log, "a"); // TODO: есл файл существует, то продолжает запись (получаются лишние строчки)

  Process child_processes[n];

  for (int i = 0; i < n; ++i) {
    child_processes[i].id = i + 1;
    child_processes[i].parent_pid = parent_pid;

    for (int k = 0; k < 11; ++k) {
      child_processes[i].channels[k][0] = -1;
      child_processes[i].channels[k][1] = -1;
    }

    for (int j = i + 1; j < n; ++j) {

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
      working(p, file_pipe);
      return 0;
    }
  }

  // проверить получение всех DONE
  printf(log_done_fmt, 0);
  fprintf(file_pipe, log_done_fmt, 0);

  fclose(file_pipe);
}
