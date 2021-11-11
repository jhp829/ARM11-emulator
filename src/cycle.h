#ifndef CYCLE
#define CYCLE
#define PC_INDEX 15

typedef enum execution_condition {
  SKIP,
  STOP,
  CONTINUE
} exec_cond;

void cycle(State *arm_state);

#endif
