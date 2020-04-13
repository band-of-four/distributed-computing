#include "ipc.h"
#include "common.h"
#include "process.h"

/* send STARTED to all other processes */
void broadcast_started(Process* p);

/* ждем, пока все потоки напишут STARTED */
void await_started(Process* p);
