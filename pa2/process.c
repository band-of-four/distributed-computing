#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "pa2345.h"
#include "ipc.h"
#include "common.h"
#include "process_utils.h"

int working(Process p, FILE *event_file) {
  log_started(&p, event_file);  // logging STARTED to a console and event file
  broadcast_started(&p);        // send STARTED to all other processes
  await_started(&p);            // ждем, пока все потоки напишут STARTED

  // Создаем структуру BalanceHistory
  BalanceHistory balanceHistory;
  balanceHistory.s_id = p.id;
  balanceHistory.s_history_len = 0;
  // --------------------------------

  int counter = 0;

  // Начальное состояние
  BalanceState balanceState;
  balanceState.s_balance = p.balance;
  balanceState.s_time = 0;
  //printf("START balanceState.balance %d (proc -- %d)\n", balanceState.s_balance, p.id);
  balanceState.s_balance_pending_in = 0;

  // добавляем стейт в хистори
  memcpy(&balanceHistory.s_history[counter], &balanceState, sizeof(BalanceState));
  balanceHistory.s_history_len = counter + 1;
  counter++;
  // TODO: move to a process_utils

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
        report_done(&p);                              /* пишем done */
        log_done(&p, event_file);                     /* logging DONE to a console and event file */
        report_balance_history(&p, &balanceHistory);  /* отправляем BALANCE_HISTORY */
        close_pipes(&p);                              /* закрываем все пайпы */
        return 0;
      };

      // если пришло сообщение трансфер
      if (received_mes.s_header.s_type == TRANSFER) {
        TransferOrder *order = (TransferOrder * )received_mes.s_payload;
        // если сообщение на получение денег -- получаем деньги, отправляем сообщение аск
        if (order->s_src != p.id) {
          p.balance += order->s_amount;               /* меняем баланс */
          time = report_ack(&p, order);               /* отправляем аск-сообщение */
          fprintf(event_file, log_transfer_in_fmt, time, order->s_dst, order->s_amount, order->s_src); /* логи */
//          printf(log_transfer_in_fmt, time, order->s_dst, order->s_amount, order->s_src);
        } else { // если соощение на требование денег -- отправляем деньги
          p.balance -= order->s_amount;               /* меняем баланс */
          //printf("Process %d new balance $%d. (-%d)\n", p.id, p.balance, order->s_amount);
          time = send_transfer(&p, order);            /* отправляем деньги */
          fprintf(event_file, log_transfer_out_fmt, time, order->s_src, order->s_amount, order->s_dst); /* логи */
//          printf(log_transfer_out_fmt, time, order->s_src, order->s_amount, order->s_dst);
        }

        // Добавляем BalanceState
        BalanceState balanceState;
        balanceState.s_balance = p.balance;
        balanceState.s_time = time;
        balanceState.s_balance_pending_in = 0;

        for (int k = balanceHistory.s_history[counter - 1].s_time; k < time - 1; ++k) {
          // добавляем стейт в хистори
          memcpy(&balanceHistory.s_history[counter], &balanceHistory.s_history[counter - 1], sizeof(BalanceState));
          balanceHistory.s_history[counter].s_time = balanceHistory.s_history[counter - 1].s_time + 1;
//          printf("%d COPY to history state %d with time %d\n", p.id, counter, balanceHistory.s_history[counter].s_time);
          balanceHistory.s_history_len = counter + 1;
          counter++;
        }
//        printf("%d add to history state %d with time %d\n", p.id, counter, balanceState.s_time);
        memcpy(&balanceHistory.s_history[counter], &balanceState, sizeof(BalanceState));
        balanceHistory.s_history_len = counter + 1;
        counter++;
      }
    }
  }
  return 0;
}
