
#include "nec-decoder.h"
#include "ir-input.h"

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
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const uint8_t hid_report_desc[] = HID_MOUSE_REPORT_DESC(2);
static enum usb_dc_status_code usb_status;

#define MOUSE_BTN_LEFT		0
#define MOUSE_BTN_RIGHT		1

enum mouse_report_idx {
	MOUSE_BTN_REPORT_IDX = 0,
	MOUSE_X_REPORT_IDX = 1,
	MOUSE_Y_REPORT_IDX = 2,
	MOUSE_WHEEL_REPORT_IDX = 3,
	MOUSE_REPORT_COUNT = 4,
};

K_MSGQ_DEFINE(mouse_msgq, MOUSE_REPORT_COUNT, 2, 1);
static K_SEM_DEFINE(ep_write_sem, 0, 1);

static inline void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	usb_status = status;
}

static ALWAYS_INLINE void rwup_if_suspended(void)
{
	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}
}

static void input_cb(struct input_event *evt, void *user_data)
{
	static uint8_t tmp[MOUSE_REPORT_COUNT];

	ARG_UNUSED(user_data);

	switch (evt->code) {
	case INPUT_KEY_0:
		if (evt->value) {
			tmp[MOUSE_Y_REPORT_IDX] -= 20U;
		}
		//rwup_if_suspended();
		//WRITE_BIT(tmp[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_LEFT, evt->value);
		break;
	case INPUT_KEY_1:
		//rwup_if_suspended();
		//WRITE_BIT(tmp[MOUSE_BTN_REPORT_IDX], MOUSE_BTN_RIGHT, evt->value);
		if (evt->value) {
			tmp[MOUSE_Y_REPORT_IDX] += 20U;
		}
		break;
	case INPUT_KEY_2:
		if (evt->value) {
			tmp[MOUSE_X_REPORT_IDX] -= 20U;
		}

		break;
	case INPUT_KEY_3:
		if (evt->value) {
			tmp[MOUSE_X_REPORT_IDX] += 20U;
		}

		break;
	default:
		LOG_INF("Unrecognized input code %u value %d",
			evt->code, evt->value);
		return;
	}

	if (k_msgq_put(&mouse_msgq, tmp, K_NO_WAIT) != 0) {
		LOG_ERR("Failed to put new input event");
	}

	tmp[MOUSE_X_REPORT_IDX] = 0U;
	tmp[MOUSE_Y_REPORT_IDX] = 0U;

}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

static void int_in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	k_sem_give(&ep_write_sem);
}

static const struct hid_ops ops = {
	.int_in_ready = int_in_ready_cb,
};

void move_fn(struct k_timer *timer_id) {
  struct input_event ev;
  ev.code = INPUT_KEY_2;
  ev.value = 10;
  input_cb(&ev, NULL);
}

K_TIMER_DEFINE(mouse_move, move_fn, NULL);


//static nec_decoder_t nec_decoder;
//static nec_decoder_message_t nec_message;
//static nec_signal_converter_t nec_converter;

//struct ir_input_node {
//  sys_snode_t node;
//  uint64_t tick;
//  int val;
//};
//
//sys_slist_t ir_input_list;

static struct gpio_callback ir_input_callback_data;

static ir_input_handler_t ir_input_handler;

void ir_input_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
   uint64_t tick = k_cycle_get_32();
   int v = gpio_pin_get_dt(&ir_input);
   ir_input_handler.on_incoming(&ir_input_handler, v, tick);
}

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
  input_cb(&ev, NULL);
}

int ir_input_init() {
  int ret;

  if (!gpio_is_ready_dt(&ir_input)) {
    LOG_ERR("IR input device %s is not ready", ir_input.port->name);
    return -1;
  }

  ret = gpio_pin_configure_dt(&ir_input, GPIO_INPUT);
  if (ret != 0) {
    LOG_ERR("Error %d: failed to configure %s pin %d", ret, ir_input.port->name, ir_input.pin);
    return -1;
  }

  ret = gpio_pin_interrupt_configure_dt(&ir_input, GPIO_INT_EDGE_BOTH);
  if (ret != 0) {
    LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", ret, ir_input.port->name, ir_input.pin);
    return -1;
  }

  gpio_init_callback(&ir_input_callback_data, ir_input_callback, BIT(ir_input.pin));
  gpio_add_callback_dt(&ir_input, &ir_input_callback_data);

  return 0;
}

int main(void) {
	const struct device *hid_dev;
	int ret;

  LOG_INF("rc-mouse");

  ir_input_handler_init(&ir_input_handler);
  ir_input_handler.on_message = on_message;

  if (ir_input_init() != 0) {
    return -1;
  }

  //while (1) {
  //  k_sleep(K_MSEC(500));
  //}

  // -----------------------------
	if (!gpio_is_ready_dt(&led0)) {
		LOG_ERR("LED device %s is not ready", led0.port->name);
		return -1;
	}


	hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL) {
		LOG_ERR("Cannot get USB HID Device");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure the LED pin, error: %d", ret);
		return 0;
	}

	usb_hid_register_device(hid_dev,
				hid_report_desc, sizeof(hid_report_desc),
				&ops);

	usb_hid_init(hid_dev);

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

  //k_timer_start(&mouse_move, K_SECONDS(1), K_SECONDS(1));
	while (true) {
		UDC_STATIC_BUF_DEFINE(report, MOUSE_REPORT_COUNT);

		k_msgq_get(&mouse_msgq, &report, K_FOREVER);

		ret = hid_int_ep_write(hid_dev, report, MOUSE_REPORT_COUNT, NULL);
		if (ret) {
			LOG_ERR("HID write error, %d", ret);
		} else {
			k_sem_take(&ep_write_sem, K_FOREVER);
			gpio_pin_toggle(led0.port, led0.pin);
		}
	}
	return 0;
}
