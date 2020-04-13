#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "banking.h"
#include "pa2345.h"
#include "ipc.h"
#include "common.h"
#include "process.h"

/* send STARTED to all other processes */
void started_broadcast(Process* p) {
  // creating message
  Message message;
  sprintf(message.s_payload, log_started_fmt, p->id - 1, p->id, p->pid, p->parent_pid, p->balance);

  MessageHeader header;
  header.s_local_time = get_physical_time();
  header.s_payload_len = strlen(message.s_payload);
  header.s_type = STARTED;
  header.s_magic = MESSAGE_MAGIC;
  message.s_header = header;
  // ------------------------

  send_multicast(p, &message);
}

// ждем, пока все потоки напишут STARTED
void get_all_started(Process* p) {
  Message received_mes;
  for (int i = 1; i <= 11; ++i) {
    if (p->channels[i][0] == -1 || p->id == i) continue;
    received_mes.s_header.s_type = -1;
    while (received_mes.s_header.s_type != STARTED)
      receive(p, i, &received_mes);
  }
}
