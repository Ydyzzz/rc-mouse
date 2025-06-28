#include "nec-decoder.h"

#include <stdlib.h>

#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/time_units.h>

#if 1
#define ND_BURST_1_DURATION_US (9000)
#define ND_BURST_2_DURATION_US (4500)
#define ND_LOGICAL_0_DURATION_US (563)
#define ND_LOGICAL_1_DURATION_US (1687)
#define ND_JITTER_US (100)
#define ND_BURST_SIGNAL_FORM_1 NEC_DECODER_SIGNAL_FORM_SPACE
#define ND_BURST_SIGNAL_FORM_2 NEC_DECODER_SIGNAL_FORM_PULSE
#define ND_BIT_START NEC_DECODER_SIGNAL_FORM_SPACE
#define ND_BIT_VAL NEC_DECODER_SIGNAL_FORM_PULSE
#define ND_WAIT_FOR_REPEAT_DURATION_1_US (563)
#define ND_WAIT_FOR_REPEAT_FORM     NEC_DECODER_SIGNAL_FORM_SPACE
#define ND_WAIT_FOR_REPEAT_FORM_NEG NEC_DECODER_SIGNAL_FORM_PULSE
#else
#define ND_BURST_1_DURATION_US (9000)
#define ND_BURST_2_DURATION_US (4500)
#define ND_LOGICAL_0_DURATION_US (563)
#define ND_LOGICAL_1_DURATION_US (1687)
#define ND_JITTER_US (90)
#define ND_BURST_SIGNAL_FORM_1 NEC_DECODER_SIGNAL_FORM_PULSE
#define ND_BURST_SIGNAL_FORM_2 NEC_DECODER_SIGNAL_FORM_SPACE
#define ND_BIT_START NEC_DECODER_SIGNAL_FORM_PULSE
#define ND_BIT_VAL NEC_DECODER_SIGNAL_FORM_SPACE
#endif

#define ND_DUR_EQ(dur, testval) (abs((int) dur - testval) < ND_JITTER_US)

static nec_decoder_result_e _nec_decode_uint16(nec_decoder_t *decoder, uint16_t *field, nec_decoder_signal_t const *signal) {
  if (decoder->bit_no > 15) {
    return NEC_DECODER_FAILED;
  }

  if (signal->form == ND_BIT_START && !decoder->prev_is_start) {
    decoder->prev_is_start = 1;
  } else if (signal->form == ND_BIT_VAL && decoder->prev_is_start) {
    decoder->prev_is_start = 0;
    if (ND_DUR_EQ(signal->duration, ND_LOGICAL_1_DURATION_US)) {
      WRITE_BIT(*field, decoder->bit_no, 1);
    } else if (ND_DUR_EQ(signal->duration, ND_LOGICAL_0_DURATION_US)) {
      WRITE_BIT(*field, decoder->bit_no, 0);
    } else {
      decoder->state = NEC_DECODER_FAILED_TO_DECODE;
      return NEC_DECODER_FAILED;
    }
    ++decoder->bit_no;
  } else {
    decoder->state = NEC_DECODER_FAILED_TO_DECODE;
    return NEC_DECODER_FAILED;
  }

  if (decoder->bit_no == 16) {
    return __NEC_DECODER_UINT16_DECODED;
  }

  return NEC_DECODER_NEEDMORE;
}

static nec_decoder_result_e _nec_decoder_ready_to_decode_run(nec_decoder_t *decoder,
  nec_decoder_message_t *message,
  nec_decoder_signal_t const *signal) {

  if (decoder->bit_no == 0 && signal->form == ND_BURST_SIGNAL_FORM_1 &&
      ND_DUR_EQ(signal->duration, ND_BURST_1_DURATION_US)) {
    decoder->state = NEC_DECODER_BURST_1_PASSED;
  } else {
    decoder->state = NEC_DECODER_FAILED_TO_DECODE;
    return NEC_DECODER_FAILED;
  }
  return NEC_DECODER_NEEDMORE;
}

static nec_decoder_result_e _nec_decoder_burst_1_passed(nec_decoder_t *decoder,
  nec_decoder_message_t *message,
  nec_decoder_signal_t const *signal) {
  if (decoder->bit_no == 0 && 
      signal->form == ND_BURST_SIGNAL_FORM_2 && 
      ND_DUR_EQ(signal->duration, ND_BURST_2_DURATION_US)) {
    decoder->state = NEC_DECODER_BURST_2_PASSED;
    decoder->prev_is_start = 0;
  } else {
    decoder->state = NEC_DECODER_FAILED_TO_DECODE;
    return NEC_DECODER_FAILED;
  }
  return NEC_DECODER_NEEDMORE;
}

static nec_decoder_result_e _nec_decoder_address_decoding(nec_decoder_t *decoder,
  nec_decoder_message_t *message,
  nec_decoder_signal_t const *signal) {
  if (decoder->bit_no > 15) {
    decoder->state = NEC_DECODER_FAILED_TO_DECODE;
    return NEC_DECODER_FAILED;
  }

  nec_decoder_result_e result = _nec_decode_uint16(decoder, &decoder->address, signal);

  if (result == __NEC_DECODER_UINT16_DECODED) {
    if ((((0xff & decoder->address)) ^ (0xff & (decoder->address >> 8))) == 0xff) {
      decoder->state = NEC_DECODER_COMMAND_DECODING;
      decoder->bit_no = 0;
      message->address = (nec_decoder_address_t) (decoder->address & 0xff);
      return NEC_DECODER_NEEDMORE;
    } else {
      decoder->state = NEC_DECODER_FAILED_TO_DECODE;
      return NEC_DECODER_FAILED;
    }
  }

  return result;
}

static nec_decoder_result_e _nec_decoder_command_decoding(nec_decoder_t *decoder,
  nec_decoder_message_t *message,
  nec_decoder_signal_t const *signal) {

  nec_decoder_result_e result = _nec_decode_uint16(decoder, &decoder->command, signal);

  if (result == __NEC_DECODER_UINT16_DECODED) {
    if ((((0xff & decoder->command)) ^ (0xff & (decoder->command >> 8))) == 0xff) {
      decoder->state = NEC_DECODER_WAIT_FOR_REPEAT;
      decoder->bit_no = 0;
      message->command = (nec_decoder_command_t) (decoder->command & 0xff);
      return NEC_DECODER_MESSAGE_DECODED;
    } else {
      decoder->state = NEC_DECODER_FAILED_TO_DECODE;
      return NEC_DECODER_FAILED;
    }
  }

  return NEC_DECODER_NEEDMORE;
}

static nec_decoder_result_e _nec_decoder_wait_for_repeat(nec_decoder_t *decoder,
    nec_decoder_message_t *message,
    nec_decoder_signal_t const *signal) {

  if (signal->form == ND_WAIT_FOR_REPEAT_FORM && ND_DUR_EQ(signal->duration, ND_WAIT_FOR_REPEAT_DURATION_1_US)) {
    decoder->state = NEC_DECODER_REPEAT_1;
  } else {
    decoder->state = NEC_DECODER_FAILED_TO_DECODE;
  }
  return NEC_DECODER_NEEDMORE_TO_REPEAT;
}

static nec_decoder_result_e _nec_decoder_repeat_1(nec_decoder_t *decoder,
    nec_decoder_message_t *message,
    nec_decoder_signal_t const *signal) {
  if (signal->form == ND_WAIT_FOR_REPEAT_FORM_NEG && (signal->duration > 37000 && signal->duration < 110000)) {
    decoder->state = NEC_DECODER_REPEAT_2;
  } else {
    decoder->state = NEC_DECODER_FAILED_TO_DECODE;
  }
  return NEC_DECODER_NEEDMORE_TO_REPEAT;
}

static nec_decoder_result_e _nec_decoder_repeat_2(nec_decoder_t *decoder,
    nec_decoder_message_t *message,
    nec_decoder_signal_t const *signal) {
  if (signal->form == ND_WAIT_FOR_REPEAT_FORM && ND_DUR_EQ(signal->duration, 9000)) {
    decoder->state = NEC_DECODER_REPEAT_3;
  } else {
    decoder->state = NEC_DECODER_FAILED_TO_DECODE;
  }
  return NEC_DECODER_NEEDMORE_TO_REPEAT;
}

static nec_decoder_result_e _nec_decoder_repeat_3(nec_decoder_t *decoder,
    nec_decoder_message_t *message,
    nec_decoder_signal_t const *signal) {
  if (signal->form == ND_WAIT_FOR_REPEAT_FORM_NEG && ND_DUR_EQ(signal->duration, 2300)) {
    decoder->state = NEC_DECODER_REPEAT_4;
  } else {
    decoder->state = NEC_DECODER_FAILED_TO_DECODE;
  }
  return NEC_DECODER_NEEDMORE_TO_REPEAT;
}

static nec_decoder_result_e _nec_decoder_repeat_4(nec_decoder_t *decoder,
    nec_decoder_message_t *message,
    nec_decoder_signal_t const *signal) {
  if (signal->form == ND_WAIT_FOR_REPEAT_FORM && ND_DUR_EQ(signal->duration, 540)) {
    decoder->state = NEC_DECODER_REPEAT_1;
  } else {
    decoder->state = NEC_DECODER_FAILED_TO_DECODE;
  }
  return NEC_DECODER_REPEAT_DECODED;
}
 

void nec_decoder_init(nec_decoder_t *decoder) {
  decoder->state = NEC_DECODER_READY_TO_DECODE;
  decoder->bit_no = 0;
  decoder->address = 0;
  decoder->command = 0;
  decoder->prev_is_start = 0;
}


nec_decoder_result_e nec_decoder_feed(nec_decoder_t *decoder,
    nec_decoder_message_t *message,
    nec_decoder_signal_t const *signal) {
  switch (decoder->state) {
  case NEC_DECODER_READY_TO_DECODE:
    return _nec_decoder_ready_to_decode_run(decoder, message, signal);
    break;
  case NEC_DECODER_BURST_1_PASSED:
    return _nec_decoder_burst_1_passed(decoder, message, signal);
    break;
  case NEC_DECODER_BURST_2_PASSED:
    return _nec_decoder_address_decoding(decoder, message, signal);
    break;
  case NEC_DECODER_COMMAND_DECODING:
    return _nec_decoder_command_decoding(decoder, message, signal);
    break;
  case NEC_DECODER_WAIT_FOR_REPEAT:
    return _nec_decoder_wait_for_repeat(decoder, message, signal);
    break;
  case NEC_DECODER_REPEAT_1:
    return _nec_decoder_repeat_1(decoder, message, signal);
    break;
  case NEC_DECODER_REPEAT_2:
    return _nec_decoder_repeat_2(decoder, message, signal);
    break;
  case NEC_DECODER_REPEAT_3:
    return _nec_decoder_repeat_3(decoder, message, signal);
    break;
  case NEC_DECODER_REPEAT_4:
    return _nec_decoder_repeat_4(decoder, message, signal);
    break;
  case NEC_DECODER_FAILED_TO_DECODE:
    return NEC_DECODER_FAILED;
    break;
  }
  return NEC_DECODER_FAILED;
}

void nec_signal_converter_init(nec_signal_converter_t *converter) {
  converter->tick0 = 0;
  converter->tick1 = 0;
  converter->level = 0;
}


nec_signal_result_e nec_convert_to_signal(nec_signal_converter_t *converter, int level, int64_t tick) {
  if (level == converter->level) {
    return NEC_SIGNAL_CONVERTER_FAILED;
  }
  converter->tick0 = converter->tick1;
  converter->tick1 = tick;

  converter->signal.duration = 	k_cyc_to_ns_ceil32(converter->tick1 - converter->tick0);
  converter->signal.form = converter->level? NEC_DECODER_SIGNAL_FORM_PULSE: NEC_DECODER_SIGNAL_FORM_SPACE;
  converter->level = level;
  return NEC_SIGNAL_CONVERTER_SUCCESS;
}
