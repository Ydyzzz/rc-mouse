
#include "nec-decoder.h"
#include "ir-input.h"
#include "mouse-hid.h"

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

static const struct gpio_dt_spec ir_input = GPIO_DT_SPEC_GET(DT_NODELABEL(ir_input), gpios);
static struct gpio_callback ir_input_callback_data;
static ir_input_handler_t ir_input_handler;

void ir_input_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
   uint64_t cyc = k_cycle_get_32();
   int v = gpio_pin_get_dt(&ir_input);
   ir_input_handler.on_incoming(&ir_input_handler, v, cyc);
}

// TODO implement the different way
void on_message(uint8_t address, uint8_t command) {
  ARG_UNUSED(address);
  struct input_event ev;
  if (command == 90) {
    ev.code = INPUT_KEY_3;
  }
  if (command == 82) {
    ev.code = INPUT_KEY_0;
  }
  if (command == 8) {
    ev.code = INPUT_KEY_2;
  }
  if (command == 24) {
    ev.code = INPUT_KEY_1;
  }
  ev.value = 10;
  mouse_hid_action_t action = {
    .action = MOUSE_MOVE_Y,
    .arg = 20,
  };
  mouse_hid_do(&action);
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

int main(void) {
  LOG_INF("rc-mouse");

  ir_input_handler_init(&ir_input_handler);
  ir_input_handler.on_message = on_message;

  if (ir_input_init() != 0) {
    return -1;
  }

  if (mouse_hid_init("HID_0") != 0) {
    return -1;
  }

	while (true) {
    __asm__ volatile ("nop");
	}
	return 0;
}
