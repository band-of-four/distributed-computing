#include "ipc.h"
#include "common.h"
#include "process.h"
#include "banking.h"

/* logging STARTED to a console and event file */
void log_started(Process* p, FILE* event_file);

/* logging DONE to a console and event file */
void log_done(Process* p, FILE* event_file);

/* send STARTED to all other processes */
void broadcast_started(Process* p);

/* ждем, пока все потоки напишут STARTED */
void await_started(Process* p);

/* пишем done */
void report_done(Process* p);

/* закрываем все пайпы */
void close_pipes(Process* p);

