#include "stdio.h"

#define GARAGE_COUNT 2

#define SWITCH_GARAGE_CLOSED 0
#define SWITCH_GARAGE_NOT_CLOSED 1

typedef enum {
    GARAGE_STATE_CLOSED,
    GARAGE_STATE_CLOSING,
    GARAGE_STATE_OPENING,
    GARAGE_STATE_OPEN
} garage_door_state_t;

typedef struct {
    uint8_t reedSwitchState;
    garage_door_state_t state;
    int secondsActivated;
    uint8_t doorSignaled;
} garage_state_t;


extern garage_state_t garage_state[GARAGE_COUNT];
extern void (*garage_state_changed)(int garage, garage_door_state_t state);

void init_garage_state(void);
void garage_trigger_door_signal(int garage);