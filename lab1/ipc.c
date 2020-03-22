#include "ipc.h"
#include "process.h"

int send(void * self, local_id dst, const Message * msg){
  Process process = (Process) self;
  write(process.channels[dst][1], msg, sizeof(msg));
  return 0;
}