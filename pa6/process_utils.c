#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "banking.h"
#include "pa2345.h"
#include "ipc.h"
#include "common.h"
#include "process.h"
#include "banking.h"
#include "process_utils.h"

typedef struct {
  int id;
  timestamp_t time;
} QueueItem;

timestamp_t req_time = 0;

int compare(const void *p, const void *q) {
  QueueItem *a1 = (QueueItem *) p;
  QueueItem *a2 = (QueueItem *) q;
  if (a1->time == a2->time)
    return a1->id > a2->id;
  else
    return a1->time > a2->time;
}

/* logging STARTED to a console and event file */
void log_started(Process *p, FILE *event_file) {
  printf(log_started_fmt, p->id - 1, p->id, p->pid, p->parent_pid, 0);
  fprintf(event_file, log_started_fmt, p->id - 1, p->id, p->pid, p->parent_pid, 0);
}

/* logging DONE to a console and event file */
void log_done(Process *p, FILE *event_file) {
  printf(log_done_fmt, p->id - 1, p->id, 0);
  fprintf(event_file, log_done_fmt, p->id - 1, p->id, 0);
}

/* send STARTED to all other processes */
void broadcast_started(Process *p) {
  Message message;
  sprintf(message.s_payload, log_started_fmt, p->id - 1, p->id, p->pid, p->parent_pid, 0);
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
void await_started(Process *p) {
  Message received_mes;
  for (int i = 1; i <= 11; ++i) {
    if (p->channels[i][0] == -1 || p->id == i) continue;
    received_mes.s_header.s_type = -1;
    while (received_mes.s_header.s_type != STARTED)
      receive(p, i, &received_mes);
  }
}

/* пишем done */
void report_done(Process *p) {
  Message message_done;
  MessageHeader header_done;
  sprintf(message_done.s_payload, log_done_fmt, p->id - 1, p->id, 0);
  local_time++;
  header_done.s_local_time = get_lamport_time();
  header_done.s_payload_len = strlen(message_done.s_payload);
  header_done.s_type = DONE;
  header_done.s_magic = MESSAGE_MAGIC;
  message_done.s_header = header_done;;

  send_multicast(p, &message_done);
}

/* закрываем все пайпы */
void close_pipes(Process *p) {
  for (int j = 1; j < 11; ++j)
    close(p->channels[j][0]);
}

/* send CS_REQUEST to all other processes */
int request_cs(const void * self) {
  Process *p = (Process *) self;
  Message message;
  sprintf(message.s_payload, "f");
  local_time++;
  req_time = get_lamport_time();
  // ------------------------
  MessageHeader header;
  header.s_local_time = get_lamport_time();
  header.s_payload_len = 1;
  header.s_type = CS_REQUEST;
  header.s_magic = MESSAGE_MAGIC;
  message.s_header = header;
  // ------------------------
  for (int i = 1; i <= 11; ++i) {
    if (p->channels[i][0] != -1 && i != p->id) {
      send(p, i, &message);
    }
  }
  return 0;
}


// из этого метода процесс выходит
// только тогда, когда наступает
// его очередь входить в
// критическую область
void process_mutex(Process *p) {
  int iter_max = p->id * 5;
  int iter = 1;
  int done_counter = 0;

  /* отправляем запрос на занятие критической области */
  request_cs(p);

  /* выясняем сколько процессов (может можно как-то лучше) */
  int n = 0;

  for (int i = 0; i <= 11; ++i) {
    if (p->channels[i][0] == -1 && i != p->id) {
      n = i - 1;
      break;
    }
  }

  int queue_reply[n];
  int capacity_reply = 0;
  int received_rep = 0;
  Message received_mes;
  for (;;) {
    for (int i = 1; i <= 11; ++i) {

      /* если все завершили работу -- выходим */
      if (done_counter == n)
        return;
      /* если не существует такого процесса -- пропускаем */
      /* ждем сообщение */
      received_mes.s_header.s_type = -1;
      if ((p->channels[i][0] == -1 || i == p->id || receive(p, i, &received_mes) > 0)
        && done_counter != n - 1) continue;

      /* проверяем тип сообщения */

      /* если получили request отправляем свои данные и записываем в очередь */
      if (received_mes.s_header.s_type == CS_REQUEST) {
        if ((received_mes.s_header.s_local_time < req_time)
            || (received_mes.s_header.s_local_time == req_time && i < p->id)
            || iter > iter_max) {
          Message message;
          local_time++;
          // ------------------------
          MessageHeader header;
          header.s_local_time = get_lamport_time();
          header.s_payload_len = 0;
          header.s_type = CS_REPLY;
          header.s_magic = MESSAGE_MAGIC;
          message.s_header = header;
          // ------------------------
          send(p, i, &message);
        } else {
          queue_reply[capacity_reply++] = i;
        }
      }
      /* пришло сообщение об отпускании критической зоны, выселяем процесс из очереди */
      if (received_mes.s_header.s_type == CS_REPLY) {
        received_rep++;
      }
      /* пришло сообщение об окончании работы -- считаем количество работающих потоков */
      if (received_mes.s_header.s_type == DONE)
        done_counter++;

      if (done_counter == n)
        return;

      /* если мы первые в очереди -- делаем работу и отправляем релиз */
      if (iter <= iter_max && received_rep == n - 1) {
        char *buff = (char *) malloc(strlen(log_loop_operation_fmt) + sizeof(int) * 3);
        sprintf(buff, log_loop_operation_fmt, p->id, iter++, iter_max);
        print(buff);
        received_rep = 0;
        for (int k = 0; k < capacity_reply; ++k) {
          Message message;
          local_time++;
          // ------------------------
          MessageHeader header;
          header.s_local_time = get_lamport_time();
          header.s_payload_len = 0;
          header.s_type = CS_REPLY;
          header.s_magic = MESSAGE_MAGIC;
          message.s_header = header;
          // ------------------------
          send(p, queue_reply[k], &message);
        }
        capacity_reply = 0;
        if (iter > iter_max) {
          // Отправляем сообщение DONE
          report_done(p);                              /* пишем done */
          done_counter++;
          printf("Proc %d завершил работу.\n", p->id);
          if (done_counter == n)
            return;
        } else {
          request_cs(p);
          received_rep = 0;
        }
      }
    }
  }
}

/* send CS_RELEASE to all other processes */
int release_cs(const void * self){
  Process *p = (Process *) self;
  Message message;
  local_time++;
  // ------------------------
  MessageHeader header;
  header.s_local_time = get_lamport_time();
  header.s_payload_len = 0;
  header.s_type = CS_RELEASE;
  header.s_magic = MESSAGE_MAGIC;
  message.s_header = header;
  // ------------------------
  for (int i = 1; i <= 11; ++i) {
    if (p->channels[i][0] != -1 && i != p->id) {
      send(p, i, &message);
    }
  }
  return 0;
}
