#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "pa2345.h"
#include "ipc.h"
#include "common.h"
#include "process_utils.h"
#include "utils.h"

timestamp_t get_lamport_time(){
  return local_time;
}

int working(Process p, FILE *event_file) {
  log_started(&p, event_file);  // logging STARTED to a console and event file
  broadcast_started(&p);        // send STARTED to all other processes
  await_started(&p);            // ждем, пока все потоки напишут STARTED

  // полезная работа
  int iter = p.id * 5;

  /* дожидаемся своей очереди */
  /* если необходимо */
  if (mutexl)
    process_mutex(&p);

  for (int i = 1; i <= iter; ++i) {
    char* buff = (char *) malloc(strlen(log_loop_operation_fmt) + sizeof(int) * 3);
    sprintf (buff, log_loop_operation_fmt, p.id,i, iter);
    print(buff);
  }

  /* оповещаем всех об освобождении */
  /* критической области если необходимо */
  if (mutexl)
    broadcast_cs_release(&p);

  // Отправляем сообщение DONE
  report_done(&p);                              /* пишем done */
  log_done(&p, event_file);                     /* logging DONE to a console and event file */
  close_pipes(&p);                              /* закрываем все пайпы */
  return 0;
}
