#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "ipc.h"
#include "process.h"
#include <fcntl.h>

int send(void *self, local_id dst, const Message *msg) {
  Process *process = (Process *) self;
    if(fcntl(process->channels[dst][1], F_GETFD) == -1 || errno == EBADF){
        printf("Descriptor: %d is closed \n", process->channels[dst][1]);
    }
  if (write(process->channels[dst][1], &msg->s_header, sizeof(MessageHeader)) < sizeof(MessageHeader)) {
    printf("Error while sending header: from = %d, to = %d (descriptor = %d)\n", process->id, dst, process->channels[dst][1]);
    exit(1);
  }
  int w = write(process->channels[dst][1], msg->s_payload, msg->s_header.s_payload_len);
  if (w < msg->s_header.s_payload_len) {
    printf("Error while sending payload: from = %d, to = %d (descriptor = %d)\n", process->id, dst, process->channels[dst][1]);
    printf("Expected: %d symbols, actual: %d symbols.\n", msg->s_header.s_payload_len, w);
    printf("Description: %s.\n", strerror(errno));
    if(fcntl(process->channels[dst][1], F_GETFD) == -1 || errno == EBADF){
        printf("Descriptor: %d is closed \n", process->channels[dst][1]);
    }
//    exit(1);
  }
//   printf("%d send to %d into %d. Type: %d Len: %d, msg: %s\n", process->id, dst, process->channels[dst][1], msg->s_header.s_type, msg->s_header.s_payload_len, msg->s_payload);
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
    header_size += read(process->channels[from][0], header + header_size, sizeof(MessageHeader)-header_size);
//    printf("SIZE: %d\n", header_size);
    if (header_size <= 0) {
      return 1;
    }
  }
//  printf("Id: %d, from: %d pipe: %d, type: %d, len %d\n", process->id, from, process->channels[from][0], header->s_type, header->s_payload_len);
  char *buffer = (char *) malloc(header->s_payload_len);
  int message_size = 0;
  while (message_size < header->s_payload_len){
    int readed = read(process->channels[from][0], buffer, header->s_payload_len-message_size);
    if(readed>0){
      message_size += readed;
    }
     // hangs here;
  }
  msg->s_header = *header;
  memcpy(&(msg->s_header), header, sizeof(MessageHeader));
  memcpy(msg->s_payload, buffer, header->s_payload_len);
//  printf("%d from %d Message: %s\n",  process->id, from, buffer);
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
