#include "ipc.h"
#include "common.h"
#include "process.h"

/* send STARTED to all other processes */
void started_broadcast(Process* p);

/* ждем, пока все потоки напишут STARTED */
void get_all_started(Process* p);
