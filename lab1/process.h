#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  int id;
  int pid;
  int parent_pid;
  int channels[11][2];
} __attribute__((packed)) Process;

int working(Process p, FILE *file_pipe);
