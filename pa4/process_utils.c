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

typedef struct
{
  int id;
  timestamp_t time;
} QueueItem;

int compare(const void *p, const void *q) {
  QueueItem* a1 = (QueueItem*)p;
  QueueItem* a2 = (QueueItem*)q;
  if (a1->time == a2->time)
    return a1->id < a2->id;
  else
    return a1->time < a2->time;
}

/* logging STARTED to a console and event file */
void log_started(Process* p, FILE* event_file) {
  printf(log_started_fmt, p->id - 1, p->id, p->pid, p->parent_pid, 0);
  fprintf(event_file, log_started_fmt, p->id - 1, p->id, p->pid, p->parent_pid, 0);
}

/* logging DONE to a console and event file */
void log_done(Process* p, FILE* event_file) {
  printf(log_done_fmt, p->id - 1, p->id, 0);
  fprintf(event_file, log_done_fmt, p->id - 1, p->id, 0);
}

/* send STARTED to all other processes */
void broadcast_started(Process* p) {
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
  sprintf(message_done.s_payload, log_done_fmt, p->id - 1, p->id, 0);
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

/* send CS_REQUEST to all other processes */
void broadcast_cs_request(Process* p) {
  Message message;
  local_time++;
  // ------------------------
  MessageHeader header;
  header.s_local_time = get_lamport_time();
  header.s_payload_len = 0;
  header.s_type = CS_REQUEST;
  header.s_magic = MESSAGE_MAGIC;
  message.s_header = header;
  // ------------------------
  send_multicast(p, &message);
}


// из этого метода процесс выходит
// только тогда, когда наступает
// его очередь входить в
// критическую область
void process_mutex(Process *p) {
  QueueItem queue[12];
  int capacity = 0;
  /* заносим этот процесс в очередь */
  timestamp_t t = get_lamport_time();
  queue[0].id = p->id;
  queue[0].time = t;
  capacity = 1;
  /* отправляем запрос на занятие критической области */
  broadcast_cs_request(p);

  /* выясняем сколько процессов (может можно как-то лучше) */
  int n = 0;
  for (int i = 0; i <= 11; ++i) {
    if (p->channels[i][0] == -1) {
      n = i - 1;
      break;
    }
  }

  int received_req = 0;
  int received_rep = 0;
  Message received_mes;
  /* пока не получим все request и reply обрабатываем их */
  while (received_req != n - 1 && received_rep != n - 1) {
    for (int i = 0; i <= 11; ++i) {
      received_mes.s_header.s_type = -1;
      if (p->channels[i][0] != -1 && i != p->id)
        receive(&p, i, &received_mes);
      else continue;
      /* если получили reply заносим в очередь процесс-отправитель */
      if (received_mes.s_header.s_type == CS_REPLY) {
        queue[capacity].id = i;
        queue[capacity].time = received_mes.s_header.s_local_time;
        capacity++;
      }
      /* если получили request отправляем свои данные */
      if (received_mes.s_header.s_type == CS_REQUEST) {
        received_req++;
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
        send(&p, i, &message);
      }
    }
  }
  /* сортируем очередь по приоритету <временная метка, номер потока> */
  qsort((void*)queue, capacity, sizeof(QueueItem), compare);

  /* если текущий процесс является самым приоритетным */
  /* выходим из функции, позволяя ему делать полезную работу */
  if (queue[0].id == p->id)
    return;

  /* иначе ожидаем сообщений об освобождении от остальных */
  /* процессов, проверяя не стал ли текущий самым приоритетным */
  /* после удаления отправившего сообщение процесса */
  int current_p = queue[0].id;

  for (;;) {
    for (int i = 0; i <= 11; ++i) {
      received_mes.s_header.s_type = -1;
      if (p->channels[i][0] != -1 && i != p->id)
        receive(&p, i, &received_mes);
      else continue;
      /* пришло сообщение об отпускании критической зоны */
      if (received_mes.s_header.s_type == CS_RELEASE) {
        /* выселяем из очереди */
        for (int j = 0; j < capacity; ++j) {
          if (queue[j].id == i) {
            /* процессы с id = -1 не учитываются при подсчете самого */
            /* приоритетного */
            queue[j].id = -1;
          }
        }
        /* вычисляем новый самый приоритетный процесс, */
        /* если это текущий - выходим и делаем дела */
        for (int j = 0; j < capacity; ++j) {
          if (queue[j].id != -1)
            current_p = queue[j].id;
          if (current_p == p->id)
            return;
        }
      }
    }
  }
}

/* send CS_RELEASE to all other processes */
void broadcast_cs_release(Process* p) {
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
  send_multicast(p, &message);
}
