#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "banking.h"
#include "pa2345.h"
#include "ipc.h"
#include "common.h"
#include "process.h"

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
  // ------------------------
  MessageHeader header;
  header.s_local_time = get_physical_time();
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
  header_done.s_local_time = get_physical_time();
  header_done.s_payload_len = strlen(message_done.s_payload);
  header_done.s_type = DONE;
  header_done.s_magic = MESSAGE_MAGIC;
  message_done.s_header = header_done;;

  send(p, 0, &message_done);
}

/* отправляем BALANCE_HISTORY */
void report_balance_history(Process* p, BalanceHistory* balanceHistory) {
  int len = balanceHistory->s_history_len* sizeof(BalanceState) + sizeof(balanceHistory->s_id) + sizeof(balanceHistory->s_history_len);
  Message balance_history_message;
  MessageHeader balance_history_header;
  balance_history_header.s_local_time = get_physical_time();
  balance_history_header.s_payload_len = len;
  balance_history_header.s_type = BALANCE_HISTORY;
  balance_history_header.s_magic = MESSAGE_MAGIC;
  balance_history_message.s_header = balance_history_header;
  memcpy(&balance_history_message.s_payload, balanceHistory, len);
  //report_balance_history_debug_print(p, balanceHistory);
  send(p, 0, &balance_history_message);
}

void report_balance_history_debug_print(Process* p, BalanceHistory* balanceHistory) {
  printf("==========>\n");
  for (int j = 0; j < balanceHistory->s_history_len; ++j){
    BalanceState balanceState =  balanceHistory->s_history[j];
    printf("OT Proc -- %d, State -- %d, Balance -- %d\n", p->id, j, balanceState.s_balance);
  }
  printf("<==========\n");
}

/* закрываем все пайпы */
void close_pipes(Process* p) {
  for (int j = 1; j < 11; ++j)
    close(p->channels[j][0]);
}

/* отправляем аск-сообщение */
timestamp_t report_ack(Process* p, TransferOrder* order) {
  printf("Process %d new balance $%d. (+%d)\n", p->id, p->balance, order->s_amount);
  Message message;

  MessageHeader header;
  header.s_local_time = get_physical_time();
  header.s_payload_len = sizeof(TransferOrder);
  header.s_type = ACK;
  header.s_magic = MESSAGE_MAGIC;
  message.s_header = header;
  memcpy(message.s_payload, order, sizeof(TransferOrder));

  send(p, 0, &message);

  return header.s_local_time;
}

/* отправляем деньги */
timestamp_t send_transfer(Process* p, TransferOrder* order) {
  Message message;

  MessageHeader header;
  header.s_local_time = get_physical_time();
  header.s_payload_len = sizeof(TransferOrder);
  header.s_type = TRANSFER;
  header.s_magic = MESSAGE_MAGIC;
  message.s_header = header;
  memcpy(message.s_payload, order, sizeof(TransferOrder));
  //printf("TRANSFER SUM = $%d. TIME = %d\n", order->s_amount, header.s_local_time);
  send(p, order->s_dst, &message);

  return header.s_local_time;
}
