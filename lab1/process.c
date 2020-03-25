#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include "ipc.h"
#include "common.h"
#include "pa1.h"
#include "process.h"

int working(Process p, FILE *file_pipe) {
  printf(log_started_fmt, p.id, p.pid, p.parent_pid);
  fprintf(file_pipe, log_started_fmt, p.id, p.pid, p.parent_pid);

  // creating message
  Message message;
  sprintf(message.s_payload, log_started_fmt, p.id, p.pid, p.parent_pid);

  MessageHeader header;
  header.s_local_time = time(NULL);
  header.s_payload_len = sizeof(message.s_payload);
  header.s_type = STARTED;
  header.s_magic = MESSAGE_MAGIC;

  message.s_header = header;
  // ------------------------

  send_multicast(&p, &message);

  // ждем, пока все потоки напишут STARTED
  Message received_mes;
  for (int i = 0; i < 11; ++i) {
    if (p.channels[i][0] != -1) {
      while (true){
        receive(&p, i, &received_mes);
        if (received_mes.s_header.s_type == STARTED) {
          continue;
          // TODO: закрыть пайпы
        }
      }
    }
  }

  //пишем done
  header.s_type = DONE;
  header.s_local_time = time(NULL);
  sprintf(message.s_payload, log_done_fmt, p.id);

  send(&p, 0, &message);

  printf(log_done_fmt, p.id);
  fprintf(file_pipe, log_done_fmt, p.id);
  return 0;
}
