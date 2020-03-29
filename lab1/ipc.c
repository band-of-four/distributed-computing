#include <string.h>
#include <stdlib.h>
#include "ipc.h"
#include "process.h"

int send(void *self, local_id dst, const Message *msg) {
  Process *process = (Process *) self;
  if (write(process->channels[dst][1], &msg->s_header, sizeof(MessageHeader)) < sizeof(MessageHeader)) {
    printf("Error while sending header: from = %d, to = %d\n", process->id, process->channels[dst][1]);
    exit(1);
  }
  if (write(process->channels[dst][1], msg->s_payload, msg->s_header.s_payload_len) < msg->s_header.s_payload_len){
    printf("Error while sending payload: from = %d, to = %d\n", process->id, process->channels[dst][1]);
    exit(1);
  }
  printf("%d send to %d. Len: %d, msg: %s", process->id, dst, msg->s_header.s_payload_len, msg->s_payload);
  // TODO: log events
  return 0;
}

int send_multicast(void *self, const Message *msg) {
  Process *process = (Process *) self;
  for (int i = 1; i <= 11; ++i) {
    if (process->channels[i][0] != -1 && i != process->id) {
      send(process, i, msg);
    }
  }
  // TODO: log events
  return 0;
}

int receive(void *self, local_id from, Message *msg) {
  Process *process = (Process *) self;
  MessageHeader *header = (MessageHeader *) malloc(sizeof(header));
  int header_size = 0;
  while (header_size < sizeof(MessageHeader)) {
    header_size += read(process->channels[from][0], header + header_size, sizeof(MessageHeader));
  }
  printf("Id: %d, from: %d, type: %d, len %d\n", process->id, from, header->s_type, header->s_payload_len);
  char *buffer = (char *) malloc(header->s_payload_len);
  int message_size = 0;
  while (message_size < header->s_payload_len){
    message_size += read(process->channels[from][0], buffer, header->s_payload_len); // hangs here;
  }
  msg->s_header = *header;
  memcpy(&(msg->s_header), header, sizeof(MessageHeader));
  memcpy(msg->s_payload, buffer, header->s_payload_len);
  printf("%d from %d Message: %s\n",  process->id, from, msg->s_payload);
  // TODO: log events
  return 0;
}

int receive_any(void *self, Message *msg) {
  Process *process = (Process *) self;
  for (int i = 0; i <= 11; ++i) {
    if (process->channels[i][0] != -1 && i != process->id) {
      receive(process, i, msg);
    }
  }
  // TODO: log events
  return 0;
}
