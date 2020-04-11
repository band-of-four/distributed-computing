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
    received_mes.s_header.s_type = -1;
    while (received_mes.s_header.s_type != STARTED)
      receive(&p, i, &received_mes);
  }

  // выполняем полезную работу
  while (true) {
    for (int i = 0; i <= 11; ++i) {
      // Если такого канала не существует -- пропускаем
      if (p.channels[i][0] == -1 || p.id == i) continue;
      Message received_mes;

      // Если ничего не прочитали -- отправляем DONE и выходим
      if (receive(&p, i, &received_mes) == 1) continue;

      // Если пришло сообщение стоп -- пропускаем
      if (received_mes.s_header.s_type == STOP) {
        //пишем done
        sprintf(message.s_payload, log_done_fmt, p.id - 1, p.id, p.balance);

        MessageHeader header_done;
        header_done.s_local_time = time(NULL);
        header_done.s_payload_len = strlen(message.s_payload);
        header_done.s_type = DONE;
        header_done.s_magic = MESSAGE_MAGIC;
        message.s_header = header_done;

        send(&p, 0, &message);

        fprintf(event_file, log_done_fmt, p.id - 1, p.id, p.balance);
        printf(log_done_fmt, p.id - 1, p.id, p.balance);
        // закрываем все пайпы
        for (int j = 0; j < 11; ++j) {
          if (p.channels[i][0] == -1 || p.id == i) continue;
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
          header.s_local_time = time(NULL);
          header.s_payload_len = sizeof(TransferOrder);
          header.s_type = ACK;
          header.s_magic = MESSAGE_MAGIC;
          message.s_header = header;
          memcpy(message.s_payload, order, sizeof(TransferOrder));

          send(&p, 0, &message);

        } else { // если соощение на требование денег -- отправляем деньги
          // меняем баланс
          p.balance -= order->s_amount;
          printf("Process %d new balance $%d. (-%d)\n", p.id, p.balance, order->s_amount);

          // отправляем деньги
          Message message;

          MessageHeader header;
          header.s_local_time = time(NULL);
          header.s_payload_len = sizeof(TransferOrder);
          header.s_type = TRANSFER;
          header.s_magic = MESSAGE_MAGIC;
          message.s_header = header;
          memcpy(message.s_payload, order, sizeof(TransferOrder));
          printf("TRANSFER SUM = $%d.\n", order->s_amount);
          send(&p, order->s_dst, &message);
        }
      }
    }
  }


  return 0;
}
