#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "banking.h"
#include "pa2345.h"
#include "ipc.h"
#include "common.h"
#include "process.h"

int working(Process p, FILE *event_file) {
  printf(log_started_fmt, p.id - 1, p.id, p.pid, p.parent_pid, p.balance);
  fprintf(event_file, log_started_fmt, p.id - 1, p.id, p.pid, p.parent_pid, p.balance);

  // creating message
  Message message;
  sprintf(message.s_payload, log_started_fmt, p.id - 1, p.id, p.pid, p.parent_pid, p.balance);

  MessageHeader header;
  header.s_local_time = get_physical_time();
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
    received_mes.s_header.s_type = -1;
    while (received_mes.s_header.s_type != STARTED)
      receive(&p, i, &received_mes);
  }

  // Создаем структуру BalanceHistory
  BalanceHistory balanceHistory;
  balanceHistory.s_id = p.id;
  balanceHistory.s_history_len = 0;

  int counter = 0;

  // Начальное состояние
  BalanceState balanceState;
  balanceState.s_balance = p.balance;
  balanceState.s_time = 0;
  printf("START balanceState.balance %d (proc -- %d)\n", balanceState.s_balance, p.id);
  balanceState.s_balance_pending_in = 0;

  // добавляем стейт в хистори
  memcpy(&balanceHistory.s_history[counter], &balanceState, sizeof(BalanceState));
  balanceHistory.s_history_len = counter + 1;
  counter++;

  // выполняем полезную работу
  while (true) {
    for (int i = 0; i <= 11; ++i) {
      int  time = 0;

      // Если такого канала не существует -- пропускаем
      if (p.channels[i][0] == -1 || p.id == i) continue;
      Message received_mes;

      // Если ничего не прочитали -- пропускаем
      if (receive(&p, i, &received_mes) == 1) continue;

      // Если пришло сообщение стоп -- отправляем DONE и выходим
      if (received_mes.s_header.s_type == STOP) {
        //пишем done

        Message message_done;
        MessageHeader header_done;
        header_done.s_local_time = get_physical_time();
        header_done.s_payload_len = strlen(message.s_payload);
        header_done.s_type = DONE;
        header_done.s_magic = MESSAGE_MAGIC;
        message_done.s_header = header_done;;
        sprintf(message_done.s_payload, log_done_fmt, p.id - 1, p.id, p.balance);

        send(&p, 0, &message_done);

        fprintf(event_file, log_done_fmt, p.id - 1, p.id, p.balance);
        printf(log_done_fmt, p.id - 1, p.id, p.balance);

        // отправляем BALANCE_HISTORY

        int len = balanceHistory.s_history_len* sizeof(BalanceState) + sizeof(balanceHistory.s_id) + sizeof(balanceHistory.s_history_len);
        Message balance_history_message;
        MessageHeader balance_history_header;
        balance_history_header.s_local_time = get_physical_time();
        balance_history_header.s_payload_len = len;
        balance_history_header.s_type = BALANCE_HISTORY;
        balance_history_header.s_magic = MESSAGE_MAGIC;
        balance_history_message.s_header = balance_history_header;
        memcpy(&balance_history_message.s_payload, &balanceHistory, len);

        printf("==========>\n");
        for (int j = 0; j < balanceHistory.s_history_len; ++j){
          BalanceState balanceState =  balanceHistory.s_history[j];
          printf("OT Proc -- %d, State -- %d, Balance -- %d\n", p.id, j, balanceState.s_balance);
        }
        printf("<==========\n");

        send(&p, 0, &balance_history_message);

        // закрываем все пайпы
        for (int j = 1; j < 11; ++j) {
          close(p.channels[j][0]);  // average process do not need to receive anything more from others
        }
        return 0;
      };

      // если пришло сообщение трансфер
      if (received_mes.s_header.s_type == TRANSFER) {
        TransferOrder *order = (TransferOrder * )received_mes.s_payload;
        // если сообщение на получение денег -- получаем деньги, отправляем сообщение аск
        if (order->s_src != p.id) {
          // меняем баланс
          p.balance += order->s_amount;
          printf("Process %d new balance $%d. (+%d)\n", p.id, p.balance, order->s_amount);

          // отправляем аск-сообщение
          Message message;

          MessageHeader header;
          header.s_local_time = get_physical_time();
          header.s_payload_len = sizeof(TransferOrder);
          header.s_type = ACK;
          header.s_magic = MESSAGE_MAGIC;
          message.s_header = header;
          memcpy(message.s_payload, order, sizeof(TransferOrder));

          send(&p, 0, &message);

          time = header.s_local_time;

          // логи
          fprintf(event_file, log_transfer_in_fmt, get_physical_time(), order->s_dst, order->s_amount, order->s_src);
        } else { // если соощение на требование денег -- отправляем деньги
          // меняем баланс
          p.balance -= order->s_amount;
          printf("Process %d new balance $%d. (-%d)\n", p.id, p.balance, order->s_amount);

          // отправляем деньги
          Message message;

          MessageHeader header;
          header.s_local_time = get_physical_time();
          header.s_payload_len = sizeof(TransferOrder);
          header.s_type = TRANSFER;
          header.s_magic = MESSAGE_MAGIC;
          message.s_header = header;
          memcpy(message.s_payload, order, sizeof(TransferOrder));
          printf("TRANSFER SUM = $%d. TIME = %d\n", order->s_amount, header.s_local_time);
          send(&p, order->s_dst, &message);

          time = header.s_local_time;

          //логгируем
          fprintf(event_file, log_transfer_out_fmt, get_physical_time(), order->s_src, order->s_amount, order->s_dst);
        }

        // Добавляем BalanceState
        BalanceState balanceState;
        balanceState.s_balance = p.balance;
        balanceState.s_time = time;
        balanceState.s_balance_pending_in = 0;

        for (int k = balanceHistory.s_history[counter - 1].s_time; k < time; ++k) {
          // добавляем стейт в хистори
          memcpy(&balanceHistory.s_history[counter], &balanceHistory.s_history[counter - 1], sizeof(BalanceState));
          balanceHistory.s_history[counter].s_time = balanceHistory.s_history[counter - 1].s_time + 1;
          balanceHistory.s_history_len = counter + 1;
          counter++;
        }
        memcpy(&balanceHistory.s_history[counter], &balanceState, sizeof(BalanceState));
        balanceHistory.s_history_len = counter + 1;
        counter++;

      }
    }
  }


  return 0;
}
