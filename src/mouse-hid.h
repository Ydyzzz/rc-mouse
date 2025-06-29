#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>

typedef enum mouse_hid_action_type {
  MOUSE_MOVE_X,
  MOUSE_MOVE_Y,
  MOUSE_CLICK_LEFT,
  MOUSE_CLICK_RIGHT,
} mouse_hid_action_type_e;

typedef struct mouse_hid_action {
  mouse_hid_action_type_e action;
  int arg;
} mouse_hid_action_t;

int mouse_hid_init(char const *device_name);
void mouse_hid_do( mouse_hid_action_t const *movement);
