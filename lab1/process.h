#include <unistd.h>
#include <stdint.h>

typedef struct {
  int id;
  int pid;
  int* channels;
} __attribute__((packed)) Process;

