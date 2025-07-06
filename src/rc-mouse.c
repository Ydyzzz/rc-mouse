#include "rc-mouse.h"
#include "mouse-hid.h"

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#define RC_MOUSE_SPEED (7)

LOG_MODULE_REGISTER(rc_mouse);

static int _hid_from_rc_state(mouse_hid_action_t *action, rc_mouse_state_t const *state) {
  if (state->current_action == RC_MOUSE_REPEAT) {
    return -1;
  }
  switch (state->current_action) {
  case RC_MOUSE_MOVE_UP:
    action->action = MOUSE_MOVE_Y;
    action->arg = -state->speed;
    break;
  case RC_MOUSE_MOVE_DOWN:
    action->action = MOUSE_MOVE_Y;
    action->arg = state->speed;
    break;
  case RC_MOUSE_MOVE_LEFT:
    action->action = MOUSE_MOVE_X;
    action->arg = -state->speed;
    break;
  case RC_MOUSE_MOVE_RIGHT:
    action->action = MOUSE_MOVE_X;
    action->arg = state->speed;
    break;
  default:
    LOG_WRN("unknown action");
    break;
  }
  return 0;
}

int rc_state_from_hid(rc_mouse_state_t *state, mouse_hid_action_t const *action) {

}

void rc_mouse_thread(void *rc_mouse_raw, void *arg1, void *arg2) {
  UNUSED(arg1);
  UNUSED(arg2);
  rc_mouse_input_t input = {0};
  mouse_hid_action_t action = {0};
  rc_mouse_state_t *rc_mouse = rc_mouse_raw;
  while (1) {
    k_msgq_get(rc_mouse->msgq, &input, K_FOREVER);
    if (input.type == RC_MOUSE_REPEAT) {
      if (rc_mouse->speed < 50) {
        rc_mouse->speed += RC_MOUSE_SPEED;
      }
      if (_hid_from_rc_state(&action, rc_mouse) != 0) {
        LOG_WRN("got incorrent mouse state");
        return;
      }
    } else {
      rc_mouse->speed = 1;
      rc_mouse->current_action = input.type;
      switch (input.type) {
      case RC_MOUSE_MOVE_UP:
        action.action = MOUSE_MOVE_Y;
        break;
      case RC_MOUSE_MOVE_DOWN:
        action.action = MOUSE_MOVE_Y;
        break;
      case RC_MOUSE_MOVE_LEFT:
        action.action = MOUSE_MOVE_X;
        break;
      case RC_MOUSE_MOVE_RIGHT:
        action.action = MOUSE_MOVE_X;
        break;
      case RC_MOUSE_CLICK_LEFT:
        action.action = MOUSE_CLICK_LEFT;
        break;
      case RC_MOUSE_CLICK_RIGHT:
        action.action = MOUSE_CLICK_RIGHT;
        break;
      default:
        // repeat unreachable
        LOG_WRN("incorrenct action");
        return;
        break;
      }
    }
    mouse_hid_do(&action);
  }
}

void rc_mouse_state_init(rc_mouse_state_t *this, struct k_msgq *msgq) {
  this->current_action = 0;
  this->speed = 1;
  this->msgq = msgq;
}

static struct k_thread rc_mouse_thread_data;
K_THREAD_STACK_DEFINE(rc_mouse_thread_stack, 500);
static rc_mouse_state_t rc_mouse_state;

k_tid_t rc_mouse_start_thread(struct k_msgq *queue) {
  rc_mouse_state_init(&rc_mouse_state, queue);
  k_tid_t tid = k_thread_create(&rc_mouse_thread_data,
      rc_mouse_thread_stack,
      K_THREAD_STACK_SIZEOF(rc_mouse_thread_stack),
      rc_mouse_thread,
      &rc_mouse_state, NULL, NULL,
      1, 0, K_NO_WAIT);
  return tid;
}
