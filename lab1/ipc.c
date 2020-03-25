#include <string.h>
#include <stdlib.h>
#include "ipc.h"
#include "process.h"

int send(void *self, local_id dst, const Message *msg) {
  Process* process = (Process*) self;
  write(process->channels[dst][1], msg, sizeof(msg));
  return 0;
}

int send_multicast(void * self, const Message * msg){
  Process* process = (Process*) self;
  for (int i = 0; i < 11; ++i){
    if (process->channels[i][0] != -1 && i != process->id){
      send(process, i, msg);
    }
  }
  return 0;
}

int receive(void * self, local_id from, Message * msg){
  Process* process = (Process*) self;
  char *buffer = (char*)malloc(sizeof(MessageHeader));
  read(process->channels[from][0], buffer, sizeof(MessageHeader));
  MessageHeader* header = (MessageHeader*)buffer;
  buffer = realloc(buffer, header->s_payload_len);
  read(process->channels[from][0], buffer, header->s_payload_len);
  msg->s_header = *header;
  memcpy(msg->s_payload, buffer, strlen(buffer));
  return 0;
}

int receive_any(void * self, Message * msg){
  Process* process = (Process*) self;
  for (int i = 0; i < 11; ++i) {
    if (process->channels[i][0] != -1 && i != process->id) {
      receive(process, i, msg);
    }
  }
  return 0;
}
