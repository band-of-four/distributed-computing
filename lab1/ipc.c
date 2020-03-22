#include "ipc.h"
#include "process.h"

int send(void *self, local_id dst, const Message *msg) {
  Process* process = (Process*) self;
  write(process.channels[dst][1], msg, sizeof(msg));
  return 0;
}

int send_multicast(void * self, const Message * msg){
  Process* process = (Process*) self;
  for (int i = 0; i < 11; ++i){
    if (process->channels[i] != -1 && i != process->id){
      send(process, i, msg);
    }
  }
  return 0;
}