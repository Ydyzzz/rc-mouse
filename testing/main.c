#include <zephyr/ztest.h>
#include <nec-decoder.h>

#include "caputed_ir_1.h"

nec_decoder_signal_t signal_start = { 
  .form = NEC_DECODER_SIGNAL_FORM_SPACE,
  .duration = 580
};

nec_decoder_signal_t signal_val0 = { 
  .form = NEC_DECODER_SIGNAL_FORM_PULSE,
  .duration = 571
};

nec_decoder_signal_t signal_val1 = {
  .form = NEC_DECODER_SIGNAL_FORM_PULSE,
  .duration = 1687
};

ZTEST(nec_decoder, test_init) {
  nec_decoder_t nec_decoder;

  nec_decoder_init(&nec_decoder);

  zassert_equal(nec_decoder.state, NEC_DECODER_READY_TO_DECODE);
  zassert_equal(nec_decoder.bit_no, 0);
}

ZTEST(nec_decoder, test_feed_burst) {
  nec_decoder_t nec_decoder;
  nec_decoder_message_t nec_message;

  nec_decoder_init(&nec_decoder);

  nec_decoder_signal_t signal_burst_1 = { 
    .form = NEC_DECODER_SIGNAL_FORM_SPACE,
    .duration = 9050
  };

  nec_decoder_signal_t signal_burst_2 = {
    .form = NEC_DECODER_SIGNAL_FORM_PULSE,
    .duration = 4501,
  };

  nec_decoder_result_e result_1 = nec_decoder_feed(&nec_decoder, &nec_message, &signal_burst_1);
  nec_decoder_state_e state_1 = nec_decoder.state;

  nec_decoder_result_e result_2 = nec_decoder_feed(&nec_decoder, &nec_message, &signal_burst_2);
  nec_decoder_state_e state_2 = nec_decoder.state;


  zassert_equal(state_1, NEC_DECODER_BURST_1_PASSED);
  zassert_equal(state_2, NEC_DECODER_BURST_2_PASSED);
  zassert_equal(result_1, NEC_DECODER_NEEDMORE);
  zassert_equal(result_2, NEC_DECODER_NEEDMORE);
  zassert_equal(nec_decoder.bit_no, 0);
}

ZTEST(nec_decoder, test_decode_address_success) {
  nec_decoder_t nec_decoder;
  nec_decoder_message_t nec_message;
  nec_decoder_result_e nec_result;

  nec_decoder_init(&nec_decoder);

  // Let's consider burst already passed
  nec_decoder.state = NEC_DECODER_BURST_2_PASSED;

  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);

  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);

  zassert_equal(nec_result, NEC_DECODER_NEEDMORE);
  zassert_equal(nec_decoder.state, NEC_DECODER_COMMAND_DECODING);
  zassert_equal(nec_decoder.address, 0x55aa);
  zassert_equal(nec_message.address, 0xaa);
}

// address != address_inv
ZTEST(nec_decoder, test_decode_address_failed_error_address) {
  nec_decoder_t nec_decoder;
  nec_decoder_message_t nec_message = { .address = 0, .command = 0 };
  nec_decoder_result_e nec_result;

  nec_decoder_init(&nec_decoder);

  // Let's consider burst already passed
  nec_decoder.state = NEC_DECODER_BURST_2_PASSED;

  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);

  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);

  zassert_equal(nec_result, NEC_DECODER_FAILED);
  zassert_equal(nec_decoder.state, NEC_DECODER_FAILED_TO_DECODE);
  zassert_equal(nec_decoder.address, 0x54aa);
  zassert_equal(nec_message.address, 0);

}

// pass invalid bit
ZTEST(nec_decoder, test_decode_address_failed_to_decode_bit) {
  nec_decoder_t nec_decoder;
  nec_decoder_message_t nec_message = { .address = 0, .command = 0 };
  nec_decoder_result_e nec_result;

  nec_decoder_init(&nec_decoder);

  // Let's consider burst already passed
  nec_decoder.state = NEC_DECODER_BURST_2_PASSED;

  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);

  zassert_equal(nec_result, NEC_DECODER_FAILED);

}

ZTEST(nec_decoder, test_decode_command_success) {
  nec_decoder_t nec_decoder;
  nec_decoder_message_t nec_message;
  nec_decoder_result_e nec_result;

  nec_decoder_init(&nec_decoder);

  // Let's consider address already passed
  nec_decoder.state = NEC_DECODER_COMMAND_DECODING;

  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);

  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);
  // 1
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val1);
  // 0
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_start);
  nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal_val0);

  zassert_equal(nec_result, NEC_DECODER_MESSAGE_DECODED);
  zassert_equal(nec_decoder.state, NEC_DECODER_WAIT_FOR_REPEAT);
  zassert_equal(nec_decoder.command, 0x55aa);
  zassert_equal(nec_message.command, 0xaa);
}

ZTEST(nec_decoder, test_real_data) {
  nec_decoder_t nec_decoder;
  nec_decoder_message_t nec_message;
  nec_decoder_result_e nec_result;

  nec_decoder_init(&nec_decoder);

  for (int i = 0; i < data_count - 1; ++i) {
    int val = data[i].value;
    uint32_t dt = (data[i + 1].tick - data[i].tick) / 1000;
    //printf("val=%d, dt=%d\n", val, dt);
    nec_decoder_signal_t signal = {
      .form = val == 1? NEC_DECODER_SIGNAL_FORM_PULSE: NEC_DECODER_SIGNAL_FORM_SPACE,
      .duration = dt,
    };
    nec_result = nec_decoder_feed(&nec_decoder, &nec_message, &signal);
    if (nec_result == NEC_DECODER_FAILED) {
      //printf("failed\n");
    }
    if (nec_result == NEC_DECODER_MESSAGE_DECODED) {
      //printf("decoded: command=%d, address=%d\n", nec_message.command, nec_message.address);
      printf("prev_is_start=%d\n", nec_decoder.prev_is_start);
    }
  }
  zassert_equal(nec_message.command, 69);
  zassert_equal(nec_message.address, 0);
}

ZTEST(nec_decoder, test_repeat_signal) {
  nec_decoder_t nec_decoder;
  nec_decoder_message_t nec_message;

  nec_decoder_init(&nec_decoder);

  nec_decoder.state = NEC_DECODER_WAIT_FOR_REPEAT;
  nec_decoder.prev_is_start = 0;

  nec_decoder_signal_t signal_space0 = {
    .form = NEC_DECODER_SIGNAL_FORM_SPACE,
    .duration = 590,
  };

  nec_decoder_signal_t signal_pulse1 = {
    .form = NEC_DECODER_SIGNAL_FORM_PULSE,
    .duration = 39966,
  };

  nec_decoder_signal_t signal_space2 = {
    .form = NEC_DECODER_SIGNAL_FORM_SPACE,
    .duration = 9072,
  };

  nec_decoder_signal_t signal_pulse3 = {
    .form = NEC_DECODER_SIGNAL_FORM_PULSE,
    .duration = 2260,
  };

  nec_decoder_signal_t signal_space4 = {
    .form = NEC_DECODER_SIGNAL_FORM_SPACE,
    .duration = 570,
  };

  nec_decoder_result_e nec_result0 = nec_decoder_feed(&nec_decoder, &nec_message, &signal_space0);
  nec_decoder_state_e nec_state0 = nec_decoder.state;
  nec_decoder_result_e nec_result1 = nec_decoder_feed(&nec_decoder, &nec_message, &signal_pulse1);
  nec_decoder_state_e nec_state1 = nec_decoder.state;
  nec_decoder_result_e nec_result2 = nec_decoder_feed(&nec_decoder, &nec_message, &signal_space2);
  nec_decoder_state_e nec_state2 = nec_decoder.state;
  nec_decoder_result_e nec_result3 = nec_decoder_feed(&nec_decoder, &nec_message, &signal_pulse3);
  nec_decoder_state_e nec_state3 = nec_decoder.state;
  nec_decoder_result_e nec_result4 = nec_decoder_feed(&nec_decoder, &nec_message, &signal_space4);
  nec_decoder_state_e nec_state4 = nec_decoder.state;

  zassert_equal(nec_result0, NEC_DECODER_NEEDMORE_TO_REPEAT);
  zassert_equal(nec_state0, NEC_DECODER_REPEAT_1);
  zassert_equal(nec_result1, NEC_DECODER_NEEDMORE_TO_REPEAT);
  zassert_equal(nec_state1, NEC_DECODER_REPEAT_2);
  zassert_equal(nec_result2, NEC_DECODER_NEEDMORE_TO_REPEAT);
  zassert_equal(nec_state2, NEC_DECODER_REPEAT_3);
  zassert_equal(nec_result3, NEC_DECODER_NEEDMORE_TO_REPEAT);
  zassert_equal(nec_state3, NEC_DECODER_REPEAT_4);
  zassert_equal(nec_result4, NEC_DECODER_REPEAT_DECODED);
  zassert_equal(nec_state4, NEC_DECODER_REPEAT_1);

}

ZTEST_SUITE(nec_decoder, NULL, NULL, NULL, NULL, NULL);
