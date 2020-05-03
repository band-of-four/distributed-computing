#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "banking.h"
#include "pa2345.h"
#include "ipc.h"
#include "common.h"
#include "process.h"
#include "banking.h"

/* logging STARTED to a console and event file */
void log_started(Process* p, FILE* event_file) {
  printf(log_started_fmt, p->id - 1, p->id, p->pid, p->parent_pid, p->balance);
  fprintf(event_file, log_started_fmt, p->id - 1, p->id, p->pid, p->parent_pid, p->balance);
}

/* logging DONE to a console and event file */
void log_done(Process* p, FILE* event_file) {
  printf(log_done_fmt, p->id - 1, p->id, p->balance);
  fprintf(event_file, log_done_fmt, p->id - 1, p->id, p->balance);
}

/* send STARTED to all other processes */
void broadcast_started(Process* p) {
  Message message;
  sprintf(message.s_payload, log_started_fmt, p->id - 1, p->id, p->pid, p->parent_pid, p->balance);
  local_time++;
  // ------------------------
  MessageHeader header;
  header.s_local_time = get_lamport_time();
  header.s_payload_len = strlen(message.s_payload);
  header.s_type = STARTED;
  header.s_magic = MESSAGE_MAGIC;
  message.s_header = header;
  // ------------------------
  send_multicast(p, &message);
}

/* ждем, пока все потоки напишут STARTED */
void await_started(Process* p) {
  Message received_mes;
  for (int i = 1; i <= 11; ++i) {
    if (p->channels[i][0] == -1 || p->id == i) continue;
    received_mes.s_header.s_type = -1;
    while (received_mes.s_header.s_type != STARTED)
      receive(p, i, &received_mes);
  }
}

/* пишем done */
void report_done(Process* p) {
  Message message_done;
  MessageHeader header_done;
  sprintf(message_done.s_payload, log_done_fmt, p->id - 1, p->id, p->balance);
  local_time++;
  header_done.s_local_time = get_lamport_time();
  header_done.s_payload_len = strlen(message_done.s_payload);
  header_done.s_type = DONE;
  header_done.s_magic = MESSAGE_MAGIC;
  message_done.s_header = header_done;;

  send(p, 0, &message_done);
}

/* закрываем все пайпы */
void close_pipes(Process* p) {
  for (int j = 1; j < 11; ++j)
    close(p->channels[j][0]);
}
