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

int forks[11];
int dirty[11];
int reqf[11];
int requested[11] = {0};

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
    if (p->channels[i][0] != -1 && i != p->id && forks[i] == 0 && requested[i] == 0) {
//      printf("%d request fork from %d\n", p->id, i);
      requested[i] = 1;
      send(p, i, &message);
    }
  }
  return 0;
}

void process_mutex(Process *p) {
  int iter_max = p->id * 5;
  int iter = 1;
  int done_counter = 0;

  /* выясняем сколько процессов (может можно как-то лучше) */
  int n = 0;

  for (int i = 0; i <= 11; ++i) {
    if (p->channels[i][0] == -1 && i != p->id) {
      n = i - 1;
      break;
    }
  }

  // Изначальная инициализация вилок
  // процесс владеет всеми общими вилками процессов с бОльшим номером
  // и готов их отдать
  for (int i = 1; i < 11; ++i) {
    if (i == p->id || i > n) {
      forks[i] = -1;
      dirty[i] = -1;
      reqf[i] = -1;
    } else if (i > p->id) {
        forks[i] = 1;
        dirty[i] = 1;
        reqf[i] = 0;
    } else {
      forks[i] = 0;
      dirty[i] = 0;
      reqf[i] = 0;
    }
  }

  /* отправляем запрос на занятие критической области */
//  request_cs(p);

  int received_rep = 0;
  Message received_mes;
  for (;;) {
    if(iter <= iter_max ){
      request_cs(p);
    }
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
//        printf("%d received request from %d\n", p->id, i);
        reqf[i] = 1;
      }
      /* пришло сообщение об отпускании критической зоны, выселяем процесс из очереди */
      if (received_mes.s_header.s_type == CS_REPLY) {
        forks[i] = 1;
        dirty[i] = 0;
        requested[i] = 0;
//        printf("%d received reply from %d\n", p->id, i);
        received_rep++;
      }

      /* пришло сообщение об окончании работы -- считаем количество работающих потоков */
      if (received_mes.s_header.s_type == DONE)
        done_counter++;

      if (done_counter == n)
        return;

      /* если мы первые в очереди -- делаем работу и отправляем релиз */

      // подсчет вилок
      int fork_count = 0;
      for (int i = 1; i <= n; ++i){
        if (i != p->id) {
          if (reqf[i] == 1) {
            if (forks[i] == 1 && dirty[i] == 1) {
              forks[i] = 0;
              dirty[i] = 0;
              reqf[i] = 0;
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
//              printf("%d send fork to %d\n", p->id, i);
            }
          }
          fork_count += forks[i];
        }
      }
//      printf("%d -- fork count %d\n", p->id, fork_count);
      if (iter <= iter_max && fork_count == n - 1) {
        char *buff = (char *) malloc(strlen(log_loop_operation_fmt) + sizeof(int) * 3);
        sprintf(buff, log_loop_operation_fmt, p->id, iter++, iter_max);
        print(buff);

        for (int k = 1; k <= n; ++k){
          if (k != p->id){
            dirty[k] = 1;
          }
        }

        for (int i = 1; i <= n; ++i){
          if (i != p->id) {
            if (reqf[i] == 1) {
              if (forks[i] == 1 && dirty[i] == 1) {
                forks[i] = 0;
                dirty[i] = 0;
                reqf[i] = 0;
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
//                printf("%d send fork to %d\n", p->id, i);
              }
            }
          }
        }

        if (iter > iter_max) {
          // Отправляем сообщение DONE
          report_done(p);                              /* пишем done */
          done_counter++;
          printf("Proc %d завершил работу.\n", p->id);
          if (done_counter == n)
            return;
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
