# [rc-mouse] – Zephyr RTOS

rc-mouse is an experimental USB mouse NEC remote controlled device.

## Supported hardware

- stm32_min_dev (bluepill)

## Commands setting

All available commands are listed in ``src/rc-command.h`` file.

Example:

```C

#pragma once

#define RC_COMMAND_RIGHT      (90)
#define RC_COMMAND_LEFT       (8)
#define RC_COMMAND_UP         (24)
#define RC_COMMAND_DOWN       (82)
#define RC_COMMAND_LEFT_CLICK (28)

```

## Building & flashing

- [Setup a Zephyr RTOS environment](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- Clone the repo

```bash
git clone *repo-url* 
```

- Build the firmware

```bash
west build -p always -b stm32_min_dev rc-mouse
```

- Flash the device

```bash
west flash
```

- Suffer & enjoy

## Run tests

To test NEC decoder run the following command from zephyr environment folder

```bash
./zephyr/scripts/twister  -T rc-mouse/testing
```
