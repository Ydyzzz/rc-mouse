#pragma once

#include <stdint.h>

typedef uint8_t nec_decoder_command_t;
typedef uint8_t nec_decoder_address_t;
typedef uint16_t nec_decoder_decoding_address_t;
typedef uint16_t nec_decoder_decoding_command_t;
typedef uint32_t nec_decoder_duration_usec_t;

typedef enum nec_decoder_state {
  NEC_DECODER_READY_TO_DECODE,
  NEC_DECODER_FAILED_TO_DECODE,
  NEC_DECODER_BURST_1_PASSED,
  NEC_DECODER_BURST_2_PASSED,
  NEC_DECODER_COMMAND_DECODING,
  NEC_DECODER_WAIT_FOR_REPEAT,
  NEC_DECODER_REPEAT_1,
  NEC_DECODER_REPEAT_2,
  NEC_DECODER_REPEAT_3,
  NEC_DECODER_REPEAT_4,
} nec_decoder_state_e;

typedef enum nec_decoder_result {
  NEC_DECODER_MESSAGE_DECODED,
  NEC_DECODER_NEEDMORE,
  NEC_DECODER_FAILED,
  NEC_DECODER_NEEDMORE_TO_REPEAT,
  NEC_DECODER_REPEAT_DECODED,

  // Internal states
  __NEC_DECODER_UINT16_DECODED,
} nec_decoder_result_e;

typedef enum nec_decoder_signal_form {
  NEC_DECODER_SIGNAL_FORM_PULSE,
  NEC_DECODER_SIGNAL_FORM_SPACE,
} nec_decoder_signal_form_e;

typedef enum nec_signal_result {
  NEC_SIGNAL_CONVERTER_SUCCESS,
  NEC_SIGNAL_CONVERTER_FAILED,
} nec_signal_result_e;

typedef struct nec_decoder_signal {
  nec_decoder_signal_form_e form;
  nec_decoder_duration_usec_t duration;
} nec_decoder_signal_t;

typedef struct nec_decoder {
  nec_decoder_state_e state;
  int8_t bit_no;
  int8_t prev_is_start;
  nec_decoder_decoding_address_t address;
  nec_decoder_decoding_command_t command;
} nec_decoder_t;


typedef struct nec_decoder_message { 
  nec_decoder_address_t address;
  nec_decoder_command_t command;
} nec_decoder_message_t;

typedef struct nec_signal_converter {
  nec_decoder_signal_t signal;
  int64_t tick0;
  int64_t tick1;
  int level;
} nec_signal_converter_t;

void nec_decoder_init(nec_decoder_t *decoder);
nec_decoder_result_e nec_decoder_feed(nec_decoder_t *decoder, nec_decoder_message_t *message, nec_decoder_signal_t const *signal);

void nec_signal_converter_init(nec_signal_converter_t *converter);
nec_signal_result_e nec_convert_to_signal(nec_signal_converter_t *converter, int level, int64_t tick);
