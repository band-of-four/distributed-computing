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
  FILE *event_file = fopen(events_log, "a");
  FILE *pipe_file = fopen(pipes_log, "w");

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

//      printf("%d-th process read from %d-th process: %d\n", i, j , j_to_i[0]);
//      printf("%d-th process write to %d-th process: %d\n", i, j , i_to_j[1]);
//      printf("%d-th process read from %d-th process: %d\n", j, i , i_to_j[0]);
//      printf("%d-th process write to %d-th process: %d\n", j, i , j_to_i[1]);
      // TODO: find a way to remove dublicate rows in pipes.log
      fprintf(pipe_file, "opened pipe from process %d to process %d\n", i, j);
      fprintf(pipe_file, "opened pipe from process %d to process %d\n", j, i);
    }
  }

  fclose(pipe_file);

  for (int i = 1; i <= n; ++i) {
    if (fork() == 0) {
      processes[i].pid = getpid();
      Process p = processes[i];
      for (int j = 0; j <= n; ++j) {
        for (int k = 0; k <= n; ++k) {
          if (j == k) continue;
          if (j == i) {
//            printf("Process: %d: %d-th process close write pipe %d-th process: %d\n",i, j, k , processes[j].channels[k][1]);
            close(processes[k].channels[j][1]); // остальным процессам закрываем все пайпы на запись
          } else if (k == i) {
//            printf("Process: %d: %d-th process close read pipe %d-th process: %d\n",i, j, k , processes[j].channels[k][0]);
            close(processes[j].channels[k][0]); // у текущего процесса закрываем все пайпы на чтение
          } else {
//            printf("Process: %d: %d-th process close write pipe %d-th process: %d\n",i, j, k , processes[j].channels[k][1]);
//            printf("Process: %d: %d-th process close read pipe %d-th process: %d\n",i, j, k , processes[j].channels[k][0]);
            close(processes[j].channels[k][0]); // остальные папы просто закрываем
            close(processes[j].channels[k][1]);
          }
        }
      }
      working(p, event_file);
      return 0;
    }
  }

  Message received_mes;
  for (int i = 1; i <= n; ++i) {
    receive(&processes[0], i, &received_mes);
    close(processes[0].channels[i][0]);
  }

  printf(log_done_fmt, 0);
  fprintf(event_file, log_done_fmt, 0);

  // TODO: закрыть пайпы
  fclose(event_file);
}
