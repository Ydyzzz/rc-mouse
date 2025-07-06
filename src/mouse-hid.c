#include "mouse-hid.h"

#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#define MOUSE_BTN_LEFT		0
#define MOUSE_BTN_RIGHT		1

typedef enum mouse_hid_report_idx {
	MOUSE_BTN_REPORT_IDX = 0,
	MOUSE_X_REPORT_IDX = 1,
	MOUSE_Y_REPORT_IDX = 2,
	MOUSE_WHEEL_REPORT_IDX = 3,
	MOUSE_REPORT_COUNT = 4,
} mouse_hid_report_idx_e;


typedef struct mouse_hid {
	struct device const *device;
  struct k_sem ep_write_sem;
  enum usb_dc_status_code status;
} mouse_hid_t;

static mouse_hid_t mouse;


LOG_MODULE_REGISTER(mouse_hid, LOG_LEVEL_INF);

static void rwup_if_suspended() {
	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (mouse.status == USB_DC_SUSPEND) {
			usb_wakeup_request();
			return;
		}
	}
}

static void _mouse_hid_int_in_ready(const struct device *dev)
{
	ARG_UNUSED(dev);
  LOG_INF("mouse hid is ready ready");
}

static void _mouse_hid_usb_status(enum usb_dc_status_code status, const uint8_t *param) {
  ARG_UNUSED(param);
  mouse.status = status;
}


static uint8_t const mouse_hid_desc[] = HID_MOUSE_REPORT_DESC(2);
static struct hid_ops const mouse_hid_ops = {
  .int_in_ready = _mouse_hid_int_in_ready,
};

int mouse_hid_init(char const *device_name) {
  int ret;

  mouse.device = device_get_binding(device_name);
  if (!mouse.device) {
    LOG_ERR("unable to get USB device");
    return -1;
  }

  usb_hid_register_device(mouse.device, mouse_hid_desc, sizeof(mouse_hid_desc), &mouse_hid_ops);

	usb_hid_init(mouse.device);

	ret = usb_enable(_mouse_hid_usb_status);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}
  return 0;
}


void mouse_hid_do(const mouse_hid_action_t *action) {
  static int click_state = 0;
	UDC_STATIC_BUF_DEFINE(report, MOUSE_REPORT_COUNT);
  memset(report, 0, sizeof(report));
  mouse_hid_report_idx_e report_idx = 0;
  uint8_t value = 0;

  if (action->action == MOUSE_CLICK_LEFT || action->action == MOUSE_CLICK_RIGHT) {
    click_state ^= 1;
    rwup_if_suspended();
    report_idx = MOUSE_BTN_REPORT_IDX;
    WRITE_BIT(value, action->action == MOUSE_CLICK_LEFT ? MOUSE_BTN_LEFT: MOUSE_BTN_RIGHT, click_state);
  } else {
    report_idx = action->action == MOUSE_MOVE_Y? MOUSE_Y_REPORT_IDX: MOUSE_X_REPORT_IDX;
    value = action->arg;
  }
  report[report_idx] = value;
  hid_int_ep_write(mouse.device, report, MOUSE_REPORT_COUNT, NULL);
}
