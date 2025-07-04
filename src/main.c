
#include "nec-decoder.h"
#include "ir-input.h"
#include "mouse-hid.h"
#include "rc-mouse.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/time_units.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usb_hid.h>

#include <zephyr/logging/log.h>

#include <string.h>


LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

K_MSGQ_DEFINE(rc_mouse_msg_queue, sizeof(rc_mouse_input_t), 20, 1);
static const struct gpio_dt_spec ir_input = GPIO_DT_SPEC_GET(DT_NODELABEL(ir_input), gpios);
static struct gpio_callback ir_input_callback_data;
static ir_input_handler_t ir_input_handler;

void ir_input_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
   uint64_t cyc = k_cycle_get_32();
   int v = gpio_pin_get_dt(&ir_input);
   ir_input_handler.on_incoming(&ir_input_handler, v, cyc);
}


int ir_input_init() {
  int ret;

  if (!gpio_is_ready_dt(&ir_input)) {
    LOG_ERR("IR input device '%s' is not ready", ir_input.port->name);
    return -1;
  }

  ret = gpio_pin_configure_dt(&ir_input, GPIO_INPUT);
  if (ret != 0) {
    LOG_ERR("error %d: failed to configure '%s' pin %d", ret, ir_input.port->name, ir_input.pin);
    return -1;
  }

  ret = gpio_pin_interrupt_configure_dt(&ir_input, GPIO_INT_EDGE_BOTH);
  if (ret != 0) {
    LOG_ERR("error %d: failed to configure interrupt on '%s' pin %d", ret, ir_input.port->name, ir_input.pin);
    return -1;
  }

  gpio_init_callback(&ir_input_callback_data, ir_input_callback, BIT(ir_input.pin));
  gpio_add_callback_dt(&ir_input, &ir_input_callback_data);

  return 0;
}


// ISR context
void on_command(uint8_t address, uint8_t command) {
  ARG_UNUSED(address);
  rc_mouse_input_t input = {0};
  if (command == 90) {
    input.type = RC_MOUSE_MOVE_RIGHT;
  } else if (command == 8) {
    input.type = RC_MOUSE_MOVE_LEFT;
  } else if (command == 82) {
    input.type = RC_MOUSE_MOVE_DOWN;
  } else if (command == 24) {
    input.type = RC_MOUSE_MOVE_UP;
  } else {
    return;
  }
  if (k_msgq_put(&rc_mouse_msg_queue, &input, K_NO_WAIT) != 0) {
    LOG_ERR("can not put rc input");
  }
  //mouse_hid_action_t action = {
  //  .action = MOUSE_MOVE_Y,
  //  .arg = 20,
  //};
  //mouse_hid_do(&action);
}

// ISR context
void on_repeat() {
  rc_mouse_input_t input = {0};
  input.type = RC_MOUSE_REPEAT;
  if (k_msgq_put(&rc_mouse_msg_queue, &input, K_NO_WAIT) != 0) {
    LOG_ERR("can not put rc input");
  }
}

int main(void) {
  LOG_INF("rc-mouse");

  ir_input_handler_init(&ir_input_handler);
  ir_input_handler.on_command = on_command;
  ir_input_handler.on_repeat = on_repeat;

  if (ir_input_init() != 0) {
    return -1;
  }

  if (mouse_hid_init("HID_0") != 0) {
    return -1;
  }

  rc_mouse_start_thread(&rc_mouse_msg_queue);
  LOG_INF("rc-mouse thread started");

  while (true) {
    // to allow log thread out logs
    k_sleep(K_MSEC(100));
  }
	return 0;
}
