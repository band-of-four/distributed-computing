#include <unistd.h>
#include <stdint.h>

typedef struct {
  int id;
  int pid;
  int* channels[2];
} __attribute__((packed)) Process;

