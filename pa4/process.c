#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "pa2345.h"
#include "ipc.h"
#include "common.h"
#include "process_utils.h"

timestamp_t get_lamport_time(){
  return local_time;
}

int working(Process p, FILE *event_file) {
  log_started(&p, event_file);  // logging STARTED to a console and event file
  broadcast_started(&p);        // send STARTED to all other processes
  await_started(&p);            // ждем, пока все потоки напишут STARTED

  // выполняем полезную работу
  while (true) {
    for (int i = 0; i <= 11; ++i) {
      // Если такого канала не существует -- пропускаем
      if (p.channels[i][0] == -1 || p.id == i) continue;
      Message received_mes;

      // Если ничего не прочитали -- пропускаем
      if (receive(&p, i, &received_mes) == 1) continue;

      // Если пришло сообщение стоп -- отправляем DONE и выходим
      if (received_mes.s_header.s_type == STOP) {
        report_done(&p);                              /* пишем done */
        log_done(&p, event_file);                     /* logging DONE to a console and event file */
        close_pipes(&p);                              /* закрываем все пайпы */
        return 0;
      };

    }
  }
  return 0;
}
