#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include "utils.h"
#include "pa1.h"
#include "ipc.h"
#include "common.h"
#include "process.h"

int main(int argc, char *argv[]) {

  int n = parse_flag(argc, argv);
  const int parent_pid = getpid();
  FILE *file_pipe = fopen(pipes_log, "w");

  Process processes[n + 1];               // process[0] is a parent process

  for (int i = 0; i <= n; ++i) {
    processes[i].id = i;
    processes[i].parent_pid = parent_pid;

    for (int k = i; k <= 11; ++k) {
      processes[i].channels[k][0] = -1;
      processes[i].channels[k][1] = -1;
    }

    for (int j = i + 1; j <= n; ++j) {

      int j_to_i[2];
      if (pipe(j_to_i) < 0) {
        printf("Error while creating pipe.\n");
        return 1;
      }

      int i_to_j[2];
      if (pipe(i_to_j) < 0) {
        printf("Error while creating pipe.\n");
        return 1;
      }

      processes[i].channels[j][0] = j_to_i[0]; // i-th process read from j-th process
      processes[i].channels[j][1] = i_to_j[1]; // i-th process write to j-th process
      processes[j].channels[i][0] = i_to_j[0]; // j-th process read from i-th process
      processes[j].channels[i][1] = j_to_i[1]; // j-th process write to i-th process
    }
  }

  for (int i = 1; i <= n; ++i) {
    if (fork() == 0) {
      processes[i].pid = getpid();
      Process p = processes[i];
//      printf("\nprocesses pid = %d, pipes:\n", p.id);
//      for (int j = 0; j <= n; ++j) {
//          printf("\t\t%d-th process read from %d-th process: %d\n", i, j, p.channels[j][0]);
//          printf("\t\t%d-th process write to %d-th process: %d\n", i, j, p.channels[j][1]);
//      }
      working(p, file_pipe);
      return 0;
    }
  }

  Message received_mes;
  for (int i = 1; i <= n; ++i) {
    receive(&processes[0], i, &received_mes);
    close(processes[0].channels[i][0]);
  }

  printf(log_done_fmt, 0);
  fprintf(file_pipe, log_done_fmt, 0);

  // TODO: закрыть пайпы
  fclose(file_pipe);
}
