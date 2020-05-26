#ifndef PA3_PROCESS_H
#define PA3_PROCESS_H

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  int id;
  int pid;
  int parent_pid;
  int channels[12][2];
} __attribute__((packed)) Process;

int working(Process p, FILE *event_file);

timestamp_t local_time;


#endif //PA3_PROCESS_H
