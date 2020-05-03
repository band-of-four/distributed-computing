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

const struct option long_options = {"mutexl", no_argument, 0, 'm'};

int option_index;
bool mutexl = false;

int parse_flag(int argc, char *argv[]) {
  int opt, n;
  while ((opt = getopt_long(argc, argv, "p:", &long_options, &option_index)) != -1) {
    switch (opt) {
      case 'p':
        if (!parse_int(optarg, &n)) {
          printf("-p parameter must be integter!\n");
          exit(1);
        }
        break;
      case 'm':
        printf("Used mutexl\n");
        mutexl = true;
        break;
      default:
        printf("Sorry :(");
        exit(-1);
    }
  }
  return n;
}

bool get_balance(int argc, char *argv[], int n, int *balance) {
  for (int i = argc - n; i < argc; ++i) {
    parse_int(argv[i], &balance[i - (argc - n)]);
  }
  return true;
}
