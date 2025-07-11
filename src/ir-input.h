#pragma once

#include "nec-decoder.h"

#include <stdint.h>

struct ir_input_handler;
typedef void (*ir_input_handler_f)(struct ir_input_handler*, uint32_t, uint32_t);
typedef void (*on_command_f)(uint8_t, uint8_t);
typedef void (*on_repeat_f)();

typedef struct ir_input_handler {
  ir_input_handler_f on_incoming;
  on_command_f on_command;
  on_repeat_f on_repeat;
  nec_decoder_t nec_decoder;
  nec_decoder_message_t nec_decoder_message;

  uint32_t val;
  uint32_t tick;
} ir_input_handler_t;

void ir_input_handler_init(ir_input_handler_t *handler);
