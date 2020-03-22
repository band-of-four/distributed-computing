typedef struct {
  uint8_t id;
  int     out_pipes[11];      // hardcoded max size 
  int     in_pipes[11];
} __attribute__((packed)) Process;
