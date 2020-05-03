#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/prctl.h>
#include <signal.h>
#include <time.h>
#include "utils.h"
#include "ipc.h"
#include "pa2345.h"
#include "common.h"
#include "process.h"
#include "banking.h"

timestamp_t local_time = 0;

void transfer(void *parent_data, local_id src, local_id dst,
              balance_t amount){}

int main(int argc, char *argv[]) {

  int n = parse_flag(argc, argv);
  int *balance = (int *) malloc(n * sizeof(int));
  get_balance(argc, argv, n, balance);

  const int parent_pid = getpid();
  FILE *event_file = fopen(events_log, "a");
  FILE *pipe_file = fopen(pipes_log, "w");

  Process processes[n + 1];               // process[0] is a parent process

  for (int i = 0; i <= n; ++i) {
    processes[i].id = i;
    processes[i].parent_pid = parent_pid;
    if (i != 0)
      processes[i].balance = balance[i - 1];
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

      int flags = fcntl(i_to_j[0], F_GETFL, 0);
      fcntl(i_to_j[0], F_SETFL, flags | O_NONBLOCK);
      flags = fcntl(i_to_j[1], F_GETFL, 0);
      fcntl(i_to_j[1], F_SETFL, flags | O_NONBLOCK);
      flags = fcntl(j_to_i[0], F_GETFL, 0);
      fcntl(j_to_i[0], F_SETFL, flags | O_NONBLOCK);
      flags = fcntl(j_to_i[1], F_GETFL, 0);
      fcntl(j_to_i[1], F_SETFL, flags | O_NONBLOCK);

      processes[i].channels[j][0] = j_to_i[0]; // i-th process read from j-th process
      processes[i].channels[j][1] = i_to_j[1]; // i-th process write to j-th process
      processes[j].channels[i][0] = i_to_j[0]; // j-th process read from i-th process
      processes[j].channels[i][1] = j_to_i[1]; // j-th process write to i-th process

//      printf("%d-th process read from %d-th process: %d\n", i, j , j_to_i[0]);
//      printf("%d-th process write to %d-th process: %d\n", i, j , i_to_j[1]);
//      printf("%d-th process read from %d-th process: %d\n", j, i , i_to_j[0]);
//      printf("%d-th process write to %d-th process: %d\n", j, i , j_to_i[1]);
      // TODO: find a way to rаemove dublicate rows in pipes.log
      fprintf(pipe_file, "opened pipe from process %d to process %d\n", i, j);
      fprintf(pipe_file, "opened pipe from process %d to process %d\n", j, i);
    }
  }

  fclose(pipe_file);

  for (int i = 1; i <= n; ++i) {
    if (fork() == 0) {
      prctl(PR_SET_PDEATHSIG, SIGHUP);
      processes[i].pid = getpid();
      Process p = processes[i];
      for (int j = 0; j <= n; ++j) {
        for (int k = 0; k <= n; ++k) {
          if (j == k) continue;
          if (j == i) {
            close(processes[k].channels[j][1]); // остальным процессам закрываем все пайпы на запись
          } else if (k == i) {
            close(processes[j].channels[k][0]); // у текущего процесса закрываем все пайпы на чтение
          } else {
            close(processes[j].channels[k][0]); // остальные папы просто закрываем
            close(processes[j].channels[k][1]);
          }
        }
      }
      working(p, event_file);
      return 0;
    }
  }

  // закрытие ненужных пайпов
  int i = 0;
  for (int j = 0; j <= n; ++j) {
    for (int k = 0; k <= n; ++k) {
      if (j == k) continue;
      if (j == i) {
        close(processes[k].channels[j][1]); // остальным процессам закрываем все пайпы на запись
      } else if (k == i) {
        close(processes[j].channels[k][0]); // у текущего процесса закрываем все пайпы на чтение
      } else {
        close(processes[j].channels[k][0]); // остальные папы просто закрываем
        close(processes[j].channels[k][1]);
      }
    }
  }

  // получение started
  Message received_mess;
  for (int i = 1; i <= n; ++i) {
    while(receive(&processes[0], i, &received_mess)>0);
    while (received_mess.s_header.s_type != STARTED) {
      receive(&processes[0], i, &received_mess);
    }
    printf("Parent received STARTED: %d from %d\n", received_mess.s_header.s_type, i); // debug print
  }

  sleep(1);

//  // sending STOP to all children
//  Message stop;

  // получение done
  Message received_mes;
  for (int i = 1; i <= n; ++i) {
    receive(&processes[0], i, &received_mes);
    while (received_mes.s_header.s_type != DONE) {
      receive(&processes[0], i, &received_mes);
    }
    printf("Parent received DONE: %d from %d\n", received_mes.s_header.s_type, i); // debug print
  }

  sleep(1);

  for (int i = 1; i <= n; ++i) {
    // я пока не уверена, какой именно надо закрыть
    close(processes[i].channels[0][1]);
    close(processes[i].channels[0][0]);
    close(processes[0].channels[i][1]);
    close(processes[0].channels[i][0]);
  }


  while (wait(NULL) > 0) {}

  printf(log_done_fmt, 0, 0, 0);
  fprintf(event_file, log_done_fmt, 0, 0, 0);

  fclose(event_file);

  return 0;
}
