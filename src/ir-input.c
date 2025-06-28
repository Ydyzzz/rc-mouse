#include "ir-input.h"
#include "nec-decoder.h"

#include <stdint.h>
#include <zephyr/sys_clock.h>
#include <zephyr/logging/log.h>

#include <stdio.h>

LOG_MODULE_REGISTER(ir_input, LOG_LEVEL_INF);

// FWD
static void _ir_input_on_data_received(ir_input_handler_t *this, uint32_t val, uint32_t tick);

static void _ir_input_on_data_reset(ir_input_handler_t *this, uint32_t val, uint32_t tick) {
  this->val = val;
  this->tick = tick;
  this->on_incoming = _ir_input_on_data_received;
}

static void _ir_input_on_data_received(ir_input_handler_t *this, uint32_t val, uint32_t tick) {
  uint32_t dt = 0;
  if (tick > this->tick) {
    dt = k_cyc_to_us_ceil32(tick) - k_cyc_to_us_ceil32(this->tick);
  } else {
    LOG_INF("tick overrun");
    dt = k_cyc_to_us_ceil32(UINT32_MAX - tick) + k_cyc_to_us_ceil32(this->tick);
  }
  nec_decoder_signal_t signal = {
    .form = this->val == 1? NEC_DECODER_SIGNAL_FORM_PULSE: NEC_DECODER_SIGNAL_FORM_SPACE,
    .duration = dt,
  };

  nec_decoder_result_e nec_result = nec_decoder_feed(&this->nec_decoder, &this->nec_decoder_message, &signal);
  if (nec_result == NEC_DECODER_FAILED) {
    if (val == 1) {
      ir_input_handler_init(this);
      return;
    }
  }
  if (nec_result == NEC_DECODER_MESSAGE_DECODED) {
    this->on_message(this->nec_decoder_message.address, this->nec_decoder_message.command);
    LOG_INF("decoded: address=%d, command=%d\n", this->nec_decoder_message.address, this->nec_decoder_message.command);
    // set timer
  }
  if (nec_result == NEC_DECODER_REPEAT_DECODED) {
    LOG_INF("decoded: repeat");
  }

  this->val = val;
  this->tick = tick;
}

void ir_input_handler_init(ir_input_handler_t *handler) {
  handler->on_incoming = _ir_input_on_data_reset;
  handler->tick = 0;
  handler->val = 0;
  nec_decoder_init(&handler->nec_decoder);
}
