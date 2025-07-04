#pragma once

#include "mouse-hid.h"

#include <zephyr/kernel.h>

typedef enum rc_mouse_input_type {
  RC_MOUSE_MOVE_UP,
  RC_MOUSE_MOVE_DOWN,
  RC_MOUSE_MOVE_LEFT,
  RC_MOUSE_MOVE_RIGHT,
  RC_MOUSE_CLICK_LEFT,
  RC_MOUSE_CLICK_RIGHT,
  RC_MOUSE_REPEAT,
} rc_mouse_input_type_e;

typedef struct rc_mouse_input {
  rc_mouse_input_type_e type;
  uint32_t arg;
} rc_mouse_input_t;

typedef struct rc_mouse_state {
  rc_mouse_input_type_e current_action;
  int speed;
  struct k_msgq *msgq;
} rc_mouse_state_t;

void rc_mouse_state_init(rc_mouse_state_t *this, struct k_msgq *msgq);
k_tid_t rc_mouse_start_thread(struct k_msgq *queue);
