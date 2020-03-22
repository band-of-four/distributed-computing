#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include "process.h"

bool parse_int(const char* str, int* var) {
  char* end;
  *var = (int)strtol(str, &end, 10);
  if (end == str) return false;
  return true;
}

int main(int argc, char* argv[]) {

  int opt, n;

  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch (opt) {
      case 'p':
        if (!parse_int(optarg, &n)) {
          printf("-p parameter must be integter!");
          return 1;
        }
    }
  }

  if (fork() == 0) {
    printf("%d\n", n);
  } else {
    printf("%d\n", n);
  }
}
