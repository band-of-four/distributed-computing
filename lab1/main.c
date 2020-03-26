#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include "utils.h"
#include "pa1.h"
#include "ipc.h"
#include "common.h"
#include "process.h"

int main(int argc, char *argv[]) {

  int n = parse_flag(argc, argv); // TODO: при n = 0 кидает seqfault
  const int parent_pid = getpid();
  FILE *file_pipe = fopen(pipes_log,
                          "a"); // TODO: если файл существует, то продолжает запись (получаются лишние строчки)
  
  Process processes[n+1];               // process[0] is a parent process

  printf("DONE = %d\n", DONE);

  for (int i = 0; i <= n; ++i) {
    processes[i].id = i;
    processes[i].parent_pid = parent_pid;

    for (int k = i; k <= 11; ++k) {
      processes[i].channels[k][0] = -1;
      processes[i].channels[k][1] = -1;
    }

    for (int j = i + 1; j <= n; ++j) {

      int p[2];
      if (pipe(p) < 0) {
        printf("Error while creating pipe.\n");
        return 1;
      }

      processes[i].channels[j][0] = p[0]; // i-th process read from j-th process
      processes[i].channels[j][1] = p[1]; // i-th process write to j-th process
      processes[j].channels[i][0] = p[1]; // j-th process read from i-th process
      processes[j].channels[i][1] = p[0]; // j-th process write to i-th process
    }
  }

  for (int i = 1; i <= n; ++i) {
    if (fork() == 0) {
      processes[i].pid = getpid();
      Process p = processes[i];
      working(p, file_pipe);
      return 0;
    }
  }

  Message received_mes;
  for (int i = 1; i <= n; ++i) {
    while (true) {
      receive(&processes[0], i, &received_mes);
      if (received_mes.s_header.s_type == DONE) {
        break;
        close(processes[0].channels[i][0]);
      }
    }
  }

  printf(log_done_fmt, 0);
  fprintf(file_pipe, log_done_fmt, 0);

  // TODO: закрыть пайпы
  fclose(file_pipe);
}
