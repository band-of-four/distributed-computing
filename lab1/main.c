#include <stdio.h>
#include "process.h"

int main(int argc, char* argv[]) {

    int n = parse_flag(argc, argv);


  if (fork() == 0) {
    printf("%d\n", n);
  } else {
    printf("%d\n", n);
  }
}
