#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "ipc.h"
#include "common.h"
#include "pa1.h"
#include "process.h"

int working(Process p, FILE *event_file) {
  printf(log_started_fmt, p.id, p.pid, p.parent_pid);
  fprintf(event_file, log_started_fmt, p.id, p.pid, p.parent_pid);

  // creating message
  Message message;
  sprintf(message.s_payload, log_started_fmt, p.id, p.pid, p.parent_pid);

  MessageHeader header;
  header.s_local_time = time(NULL);
  header.s_payload_len = strlen(message.s_payload);
  header.s_type = STARTED;
  header.s_magic = MESSAGE_MAGIC;
  message.s_header = header;
  // ------------------------

  send_multicast(&p, &message);

  // ждем, пока все потоки напишут STARTED
  Message received_mes;
  for (int i = 1; i <= 11; ++i) {
    if (p.channels[i][0] == -1 || p.id == i) continue;
    receive(&p, i, &received_mes);
    close(p.channels[i][0]);  // average process do not need to receive anything more from others
  }

  //пишем done
  sprintf(message.s_payload, log_done_fmt, p.id);

  MessageHeader header_done;
  header_done.s_local_time = time(NULL);
  header_done.s_payload_len = strlen(message.s_payload);
  header_done.s_type = DONE;
  header_done.s_magic = MESSAGE_MAGIC;
  message.s_header = header_done;

  send(&p, 0, &message);
  printf("%s", message.s_payload);

  fprintf(event_file, log_done_fmt, p.id);
  return 0;
}
