#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "utils.h"

bool parse_int(const char *str, int *var) {
  char *end;
  *var = (int) strtol(str, &end, 10);
  if (end == str) return false;
  return true;
}

int parse_flag(int argc, char *argv[]) {
  int opt, n;
  while ((opt = getopt(argc, argv, "p:")) != -1) {
    switch (opt) {
      case 'p':
        if (!parse_int(optarg, &n)) {
          printf("-p parameter must be integter!\n");
          exit(1);
        }
    }
  }
  return n;
}
