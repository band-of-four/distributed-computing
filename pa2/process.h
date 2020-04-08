#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  int id;
  int pid;
  int parent_pid;
  int channels[12][2];
  int balance;
} __attribute__((packed)) Process;

int working(Process p, FILE *event_file);
