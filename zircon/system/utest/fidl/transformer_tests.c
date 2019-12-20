// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <lib/fidl/transformer.h>
#include <stdio.h>
#include <string.h>

#include <unittest/unittest.h>

#include "generated/transformer_tables.test.h"
#include "zircon/errors.h"
#include "zircon/types.h"

#define ASSERT_TRUE_NOMSG(x) ASSERT_TRUE(x, "")

bool cmp_payload(const uint8_t* actual, size_t actual_size, const uint8_t* expected,
                 size_t expected_size) {
  bool pass = true;
  for (size_t i = 0; i < actual_size && i < expected_size; i++) {
    if (actual[i] != expected[i]) {
      pass = false;
      printf("element[%zu]: actual=0x%x expected=0x%x\n", i, actual[i], expected[i]);
    }
  }
  if (actual_size != expected_size) {
    pass = false;
    printf("element[...]: actual.size=%zu expected.size=%zu\n", actual_size, expected_size);
  }
  return pass;
}

// This is a non-static global variable since it's also used by message_tests.cc.
const uint8_t sandwich1_case1_v1[] = {
    0x01, 0x02, 0x03, 0x04,  // Sandwich1.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich1.before (padding)

    0x03, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag, i.e. Sandwich1.the_union
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.padding
    0x08, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_bytes
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.env.presence
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.presence [cont.]

    0x05, 0x06, 0x07, 0x08,  // Sandwich1.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich1.after (padding)

    0x09, 0x0a, 0x0b, 0x0c,  // UnionSize8Aligned4.data, i.e. Sandwich1.the_union.data
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.data (padding)
};

// This is a non-static global variable since it's also used by message_tests.cc.
const uint8_t sandwich1_case1_old[] = {
    0x01, 0x02, 0x03, 0x04,  // Sandwich1.before

    0x02, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag, i.e. Sandwich1.the_union
    0x09, 0x0a, 0x0b, 0x0c,  // UnionSize8Aligned4.data

    0x05, 0x06, 0x07, 0x08,  // Sandwich1.after
};

static const uint8_t sandwich1_case1_with_hdr_v1[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x01, 0x02, 0x03, 0x04,  // Sandwich1.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich1.before (padding)

    0x03, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag, i.e. Sandwich1.the_union
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.padding
    0x08, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_bytes
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.env.presence
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.presence [cont.]

    0x05, 0x06, 0x07, 0x08,  // Sandwich1.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich1.after (padding)

    0x09, 0x0a, 0x0b, 0x0c,  // UnionSize8Aligned4.data, i.e. Sandwich1.the_union.data
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.data (padding)
};

static const uint8_t sandwich1_case1_with_hdr_old[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x01, 0x02, 0x03, 0x04,  // Sandwich1.before

    0x02, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag, i.e. Sandwich1.the_union
    0x09, 0x0a, 0x0b, 0x0c,  // UnionSize8Aligned4.data

    0x05, 0x06, 0x07, 0x08,  // Sandwich1.after
};

static const uint8_t sandwich4_with_hdr_case1_v1[] = {
    0x00, 0x00, 0x00, 0x00,  // Fake transaction header  0x00
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //

    0x01, 0x02, 0x03, 0x04,  // Sandwich4.before  0x10
    0x00, 0x00, 0x00, 0x00,  // Sandwich4.before (padding)

    0x04, 0x00, 0x00, 0x00,  // UnionSize36Alignment4.tag, i.e. Sandwich4.the_union
    0x00, 0x00, 0x00, 0x00,  // UnionSize36Alignment4.tag (padding)
    0x20, 0x00, 0x00, 0x00,  // UnionSize36Alignment4.env.num_bytes  0x20
    0x00, 0x00, 0x00, 0x00,  // UnionSize36Alignment4.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // UnionSize36Alignment4.env.presence
    0xff, 0xff, 0xff, 0xff,  // UnionSize36Alignment4.env.presence [cont.]

    0x05, 0x06, 0x07, 0x08,  // Sandwich4.after  0x30
    0x00, 0x00, 0x00, 0x00,  // Sandwich4.after (padding)

    0xa0, 0xa1, 0xa2, 0xa3,  // UnionSize36Alignment4.data, i.e. Sandwich4.the_union.data
    0xa4, 0xa5, 0xa6, 0xa7,  // UnionSize36Alignment4.data [cont.]
    0xa8, 0xa9, 0xaa, 0xab,  // UnionSize36Alignment4.data [cont.]  0x40
    0xac, 0xad, 0xae, 0xaf,  // UnionSize36Alignment4.data [cont.]
    0xb0, 0xb1, 0xb2, 0xb3,  // UnionSize36Alignment4.data [cont.]
    0xb4, 0xb5, 0xb6, 0xb7,  // UnionSize36Alignment4.data [cont.]
    0xb8, 0xb9, 0xba, 0xbb,  // UnionSize36Alignment4.data [cont.]  0x50
    0xbc, 0xbd, 0xbe, 0xbf,  // UnionSize36Alignment4.data [cont.]
};

static const uint8_t sandwich4_with_hdr_case1_old[] = {
    0x00, 0x00, 0x00, 0x00,  // Fake transaction header  0x00
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //

    0x01, 0x02, 0x03, 0x04,  // Sandwich4.before  0x10

    0x03, 0x00, 0x00, 0x00,  // UnionSize36Alignment4.tag, i.e. Sandwich4.the_union
    0xa0, 0xa1, 0xa2, 0xa3,  // UnionSize36Alignment4.data
    0xa4, 0xa5, 0xa6, 0xa7,  // UnionSize36Alignment4.data [cont.]
    0xa8, 0xa9, 0xaa, 0xab,  // UnionSize36Alignment4.data [cont.]  0x20
    0xac, 0xad, 0xae, 0xaf,  // UnionSize36Alignment4.data [cont.]
    0xb0, 0xb1, 0xb2, 0xb3,  // UnionSize36Alignment4.data [cont.]
    0xb4, 0xb5, 0xb6, 0xb7,  // UnionSize36Alignment4.data [cont.]
    0xb8, 0xb9, 0xba, 0xbb,  // UnionSize36Alignment4.data [cont.]  0x30
    0xbc, 0xbd, 0xbe, 0xbf,  // UnionSize36Alignment4.data [cont.]

    0x05, 0x06, 0x07, 0x08,  // Sandwich4.after

    0x00, 0x00, 0x00, 0x00,  // padding for top-level struct
};

static const uint8_t sandwich5_case1_with_hdr_v1[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x01, 0x02, 0x03, 0x04,  // Sandwich5.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.before (padding)

    0x02, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.ordinal
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.padding
    0x20, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.env.num_bytes
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // Sandwich5.UnionOfUnion.env.presence
    0xff, 0xff, 0xff, 0xff,  // Sandwich5.UnionOfUnion.env.presence [cont.]

    0x05, 0x06, 0x07, 0x08,  // Sandwich5.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.after (padding)

    0x03, 0x00, 0x00, 0x00,  // UnionOfUnion.UnionSize8Aligned4.ordinal
    0x00, 0x00, 0x00, 0x00,  // UnionOfUnion.UnionSize8Aligned4.padding
    0x08, 0x00, 0x00, 0x00,  // UnionOfUnion.UnionSize8Aligned4.env.num_bytes
    0x00, 0x00, 0x00, 0x00,  // UnionOfUnion.UnionSize8Aligned4.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // UnionOfUnion.UnionSize8Aligned4.env.presence
    0xff, 0xff, 0xff, 0xff,  // UnionOfUnion.UnionSize8Aligned4.env.presence [cont.]

    0x09, 0x0a, 0x0b, 0x0c,  // UnionOfUnion.UnionSize8Aligned4.data
    0x00, 0x00, 0x00, 0x00,  // UnionOfUnion.UnionSize8Aligned4.data (padding)
};

static const uint8_t sandwich5_case1_with_hdr_old[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x01, 0x02, 0x03, 0x04,  // Sandwich5.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.before (padding)

    0x01, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.tag
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.tag (padding)

    0x02, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag, i.e Sandwich5.UnionOfUnion.data
    0x09, 0x0a, 0x0b, 0x0c,  // UnionSize8Aligned4.data
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.data (padding)
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.data (padding)
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.data (padding)
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.UnionSize8Aligned4.data (padding)

    0x05, 0x06, 0x07, 0x08,  // Sandwich5.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.after (padding)
};

static const uint8_t sandwich5_case2_with_hdr_v1[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x01, 0x02, 0x03, 0x04,  // Sandwich5.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.before (padding)

    0x04, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.ordinal
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.padding
    0x28, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.env.num_bytes
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // Sandwich5.UnionOfUnion.env.presence
    0xff, 0xff, 0xff, 0xff,  // Sandwich5.UnionOfUnion.env.presence [cont.]

    0x05, 0x06, 0x07, 0x08,  // Sandwich5.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.after (padding)

    0x04, 0x00, 0x00, 0x00,  // UnionOfUnion.UnionSize24Alignment8.ordinal
    0x00, 0x00, 0x00, 0x00,  // UnionOfUnion.UnionSize24Alignment8.padding
    0x10, 0x00, 0x00, 0x00,  // UnionOfUnion.UnionSize24Alignment8.env.num_bytes
    0x00, 0x00, 0x00, 0x00,  // UnionOfUnion.UnionSize24Alignment8.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // UnionOfUnion.UnionSize24Alignment8.env.presence
    0xff, 0xff, 0xff, 0xff,  // UnionOfUnion.UnionSize24Alignment8.env.presence [cont.]

    0xa0, 0xa1, 0xa2, 0xa3,  // UnionOfUnion.UnionSize24Alignment8.data
    0xa4, 0xa5, 0xa6, 0xa7,  // UnionOfUnion.UnionSize24Alignment8.data [cont.]
    0xa8, 0xa9, 0xaa, 0xab,  // UnionOfUnion.UnionSize24Alignment8.data [cont.]
    0xac, 0xad, 0xae, 0xaf,  // UnionOfUnion.UnionSize24Alignment8.data [cont.]
};

static const uint8_t sandwich5_case2_with_hdr_old[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x01, 0x02, 0x03, 0x04,  // Sandwich5.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.before (padding)

    0x03, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.tag
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.UnionOfUnion.tag (padding)

    0x03, 0x00, 0x00, 0x00,  // UnionSize24Alignment8.tag, i.e Sandwich5.UnionOfUnion.data
    0x00, 0x00, 0x00, 0x00,  // UnionSize24Alignment8.tag (padding)
    0xa0, 0xa1, 0xa2, 0xa3,  // UnionSize24Alignment8.data
    0xa4, 0xa5, 0xa6, 0xa7,  // UnionSize24Alignment8.data [cont.]
    0xa8, 0xa9, 0xaa, 0xab,  // UnionSize24Alignment8.data [cont.]
    0xac, 0xad, 0xae, 0xaf,  // UnionSize24Alignment8.data [cont.]

    0x05, 0x06, 0x07, 0x08,  // Sandwich5.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich5.after (padding)
};

static const uint8_t sandwich6_case5_v1[] = {
    0x01, 0x02, 0x03, 0x04,  // Sandwich6.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich6.before (padding)

    0x06, 0x00, 0x00, 0x00,  // UnionWithVector.ordinal (start of Sandwich6.the_union)
    0x00, 0x00, 0x00, 0x00,  // UnionWithVector.ordinal (padding)
    0x20, 0x00, 0x00, 0x00,  // UnionWithVector.env.num_bytes
    0x03, 0x00, 0x00, 0x00,  // UnionWithVector.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // UnionWithVector.env.presence
    0xff, 0xff, 0xff, 0xff,  // UnionWithVector.env.presence [cont.]

    0x05, 0x06, 0x07, 0x08,  // Sandwich6.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich6.after (padding)

    0x03, 0x00, 0x00, 0x00,  // vector<handle>.size, i.e. Sandwich6.the_union.data
    0x00, 0x00, 0x00, 0x00,  // vector<handle>.size [cont.]
    0xff, 0xff, 0xff, 0xff,  // vector<handle>.presence
    0xff, 0xff, 0xff, 0xff,  // vector<handle>.presence [cont.]

    0xff, 0xff, 0xff, 0xff,  // vector<handle>.data
    0xff, 0xff, 0xff, 0xff,  // vector<handle>.data
    0xff, 0xff, 0xff, 0xff,  // vector<handle>.data
    0x00, 0x00, 0x00, 0x00,  // vector<handle>.data (padding)
};

static const uint8_t sandwich6_case5_old[] = {
    0x01, 0x02, 0x03, 0x04,  // Sandwich6.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich6.before (padding)

    0x05, 0x00, 0x00, 0x00,  // UnionWithVector.tag (start of Sandwich6.the_union)
    0x00, 0x00, 0x00, 0x00,  // UnionWithVector.tag (padding)
    0x03, 0x00, 0x00, 0x00,  // vector<handle>.size, i.e. Sandwich6.the_union.data
    0x00, 0x00, 0x00, 0x00,  // vector<handle>.size [cont.]
    0xff, 0xff, 0xff, 0xff,  // vector<handle>.presence
    0xff, 0xff, 0xff, 0xff,  // vector<handle>.presence [cont.]

    0x05, 0x06, 0x07, 0x08,  // Sandwich6.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich6.after (padding)

    0xff, 0xff, 0xff, 0xff,  // vector<handle>.data
    0xff, 0xff, 0xff, 0xff,  // vector<handle>.data
    0xff, 0xff, 0xff, 0xff,  // vector<handle>.data
    0x00, 0x00, 0x00, 0x00,  // vector<handle>.data (padding)
};

static const uint8_t sandwich7_case1_with_hdr_v1[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x11, 0x12, 0x13, 0x14,  // Sandwich7.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.before (padding)
    0xff, 0xff, 0xff, 0xff,  // Sandwich7.opt_sandwich1.presence
    0xff, 0xff, 0xff, 0xff,  // Sandwich7.opt_sandwich1.presence [cont.]
    0x21, 0x22, 0x23, 0x24,  // Sandwich7.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.after (padding)

    0x01, 0x02, 0x03, 0x04,  // Sandwich1.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich1.before (padding)
    0x03, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag, i.e. Sandwich1.the_union
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.padding
    0x08, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_bytes
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.env.presence
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.presence [cont.]
    0x05, 0x06, 0x07, 0x08,  // Sandwich1.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich1.after (padding)

    0x09, 0x0a, 0x0b, 0x0c,  // UnionSize8Aligned4.data, i.e. Sandwich1.the_union.data
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.data (padding)
};

static const uint8_t sandwich7_case1_with_hdr_old[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x11, 0x12, 0x13, 0x14,  // Sandwich7.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.before (padding)
    0xff, 0xff, 0xff, 0xff,  // Sandwich7.opt_sandwich1.presence
    0xff, 0xff, 0xff, 0xff,  // Sandwich7.opt_sandwich1.presence [cont.]
    0x21, 0x22, 0x23, 0x24,  // Sandwich7.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.after (padding)

    0x01, 0x02, 0x03, 0x04,  // Sandwich1.before
    0x02, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag, i.e. Sandwich1.the_union
    0x09, 0x0a, 0x0b, 0x0c,  // UnionSize8Aligned4.data
    0x05, 0x06, 0x07, 0x08,  // Sandwich1.after
};

static const uint8_t sandwich7_case2_with_hdr_v1[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x11, 0x12, 0x13, 0x14,  // Sandwich7.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.before (padding)
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.opt_sandwich1.preabsentsence
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.opt_sandwich1.absence [cont.]
    0x21, 0x22, 0x23, 0x24,  // Sandwich7.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.after (padding)
};

static const uint8_t sandwich7_case2_with_hdr_old[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x11, 0x12, 0x13, 0x14,  // Sandwich7.before
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.before (padding)
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.opt_sandwich1.absence
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.opt_sandwich1.absence [cont.]
    0x21, 0x22, 0x23, 0x24,  // Sandwich7.after
    0x00, 0x00, 0x00, 0x00,  // Sandwich7.after (padding)
};

static const uint8_t regression5_old_and_v1[] = {
    0x01, 0x00, 0x00, 0x00,  // f1 (uint8) + padding
    0x2F, 0x30, 0x31, 0x32,  // f2 (uint32 enum)
    0x08, 0x00, 0x15, 0x16,  // f3 (uint8 enum) + padding + f4 (uint16)
    0x00, 0x00, 0x00, 0x00,  // padding
    0x5D, 0x5E, 0x5F, 0x60,  // f5 (uint64)
    0x61, 0x62, 0x63, 0x64,  // f5 (uint64) [cont.]
    0x08, 0x00, 0x00, 0x00,  // f6 (uint8) + padding
    0x00, 0x00, 0x00, 0x00,  // padding
};

static const uint8_t regression6_old_and_v1[] = {
    0x01, 0x00, 0x00, 0x00,  // f1 (uint8) + padding
    0x30, 0x00, 0x00, 0x03,  // f2 (uint32 enum)
    0x08, 0x00, 0x15, 0x16,  // f3 (uint8 enum) + padding + f4 (uint16)
    0x00, 0x00, 0x00, 0x00,  // padding
    0x5D, 0x5E, 0x5F, 0x60,  // f5 (uint64)
    0x61, 0x62, 0x63, 0x64,  // f5 (uint64) [cont.]
    0x08, 0x00, 0x00, 0x00,  // f6 (uint8) + padding
    0x00, 0x00, 0x00, 0x00,  // padding
};

static const uint8_t mixed_fields_v1[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x01, 0x02, 0x03, 0x04,  // before
    0x00, 0x00, 0x00, 0x00,  // before (padding)

    0x03, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.padding
    0x08, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_bytes
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.env.presence
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.presence [cont.]

    0x0a, 0x0b, 0x00, 0x00,  // middle_start
    0x00, 0x00, 0x00, 0x00,  // middle_start (padding)

    0x08, 0x07, 0x06, 0x05,  // middle_end
    0x04, 0x03, 0x02, 0x01,  // middle_end

    0x03, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.padding
    0x08, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_bytes
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.env.num_handle
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.env.presence
    0xff, 0xff, 0xff, 0xff,  // UnionSize8Aligned4.presence [cont.]

    0x05, 0x06, 0x07, 0x08,  // after
    0x00, 0x00, 0x00, 0x00,  // after (padding)

    0x09, 0x0a, 0x0b, 0x0c,  // UnionSize8Aligned4.data, i.e. Sandwich1.the_union.data
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.data (padding)

    0x90, 0xa0, 0xb0, 0xc0,  // UnionSize8Aligned4.data, i.e. Sandwich1.the_union.data
    0x00, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.data (padding)
};

static const uint8_t mixed_fields_old[] = {
    0xf0, 0xf1, 0xf2, 0xf3,  // Fake transaction header
    0xf4, 0xf5, 0xf6, 0xf7,  // [cont.]
    0xf8, 0xf9, 0xfa, 0xfb,  // [cont.]
    0xfc, 0xfd, 0xfe, 0xff,  // [cont.]

    0x01, 0x02, 0x03, 0x04,  // before

    0x02, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag
    0x09, 0x0a, 0x0b, 0x0c,  // UnionSize8Aligned4.data, i.e. Sandwich1.the_union.data

    0x0a, 0x0b, 0x00, 0x00,  // middle_start

    0x08, 0x07, 0x06, 0x05,  // middle_end
    0x04, 0x03, 0x02, 0x01,  // middle_end

    0x02, 0x00, 0x00, 0x00,  // UnionSize8Aligned4.tag
    0x90, 0xa0, 0xb0, 0xc0,  // UnionSize8Aligned4.data, i.e. Sandwich1.the_union.data

    0x05, 0x06, 0x07, 0x08,  // after
    0x00, 0x00, 0x00, 0x00,  // after (padding)
};

// This is a non-static global variable since it's also used by message_tests.cc.
const uint8_t simpletablearraystruct_v1_and_old[] = {
    0x01, 0x00, 0x00, 0x00,  // 0x00
    0x00, 0x00, 0x00, 0x00,  //
    0xFF, 0xFF, 0xFF, 0xFF,  //
    0xFF, 0xFF, 0xFF, 0xFF,  //
    0x01, 0x00, 0x00, 0x00,  // 0x10
    0x00, 0x00, 0x00, 0x00,  //
    0xFF, 0xFF, 0xFF, 0xFF,  //
    0xFF, 0xFF, 0xFF, 0xFF,  //
    0x08, 0x00, 0x00, 0x00,  // 0x20
    0x00, 0x00, 0x00, 0x00,  //
    0xFF, 0xFF, 0xFF, 0xFF,  //
    0xFF, 0xFF, 0xFF, 0xFF,  //
    0xA0, 0xA1, 0xa2, 0xa3,  // 0x30
    0x00, 0x00, 0x00, 0x00,  //
    0x08, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0xFF, 0xFF, 0xFF, 0xFF,  // 0x40
    0xFF, 0xFF, 0xFF, 0xFF,  //
    0xB0, 0xB1, 0xB2, 0xB3,  //
    0x00, 0x00, 0x00, 0x00,  //
};

static const uint8_t stringunionstructwrapperresponse_v1[] = {
    0x00, 0x00, 0x00, 0x00,  // 0x00
    0x01, 0x00, 0x00, 0x01,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x25, 0x32, 0xa0, 0x32,  //
    0x01, 0x00, 0x00, 0x00,  // 0x10
    0x00, 0x00, 0x00, 0x00,  //
    0x18, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0xff, 0xff, 0xff, 0xff,  // 0x20
    0xff, 0xff, 0xff, 0xff,  //
    0x02, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x08, 0x00, 0x00, 0x00,  // 0x30
    0x00, 0x00, 0x00, 0x00,  //
    0xff, 0xff, 0xff, 0xff,  //
    0xff, 0xff, 0xff, 0xff,  //
    0x05, 0x00, 0x00, 0x00,  // 0x40
    0x00, 0x00, 0x00, 0x00,  //
    0xff, 0xff, 0xff, 0xff,  //
    0xff, 0xff, 0xff, 0xff,  //
    0x68, 0x65, 0x6c, 0x6c,  // 0x50
    0x6f, 0x00, 0x00, 0x00,  //
    0x01, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
};

static const uint8_t stringunionstructwrapperresponse_old[] = {
    0x00, 0x00, 0x00, 0x00,  // 0x00
    0x01, 0x00, 0x00, 0x01,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x25, 0x32, 0xa0, 0x32,  //
    0x00, 0x00, 0x00, 0x00,  // 0x10
    0x00, 0x00, 0x00, 0x00,  //
    0x05, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0xff, 0xff, 0xff, 0xff,  // 0x20
    0xff, 0xff, 0xff, 0xff,  //
    0xff, 0xff, 0xff, 0xff,  //
    0xff, 0xff, 0xff, 0xff,  //
    0x68, 0x65, 0x6c, 0x6c,  // 0x30
    0x6f, 0x00, 0x00, 0x00,  //
    0x01, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x01, 0x00, 0x00, 0x00,  // 0x40
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
};

static const uint8_t launcher_create_component_request_v1[] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,  // 0x00 tx header
    0x00, 0x00, 0x00, 0x00, 0x65, 0x29, 0x3F, 0x0D,  //
    0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x10 string url
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x20 vector args
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x30 out
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // err
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  // 0x40 dir_request
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // flat_namespace
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x50 additional_services
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  // handle h (request parameter)
    0x66, 0x75, 0x63, 0x68, 0x73, 0x69, 0x61, 0x2D,  // 0x60
    0x70, 0x6B, 0x67, 0x3A, 0x2F, 0x2F, 0x66, 0x75,  //
    0x63, 0x68, 0x73, 0x69, 0x61, 0x2E, 0x63, 0x6F,  // 0x70
    0x6D, 0x2F, 0x66, 0x69, 0x64, 0x6C, 0x5F, 0x63,  //
    0x6F, 0x6D, 0x70, 0x61, 0x74, 0x69, 0x62, 0x69,  // 0x80
    0x6C, 0x69, 0x74, 0x79, 0x5F, 0x74, 0x65, 0x73,  //
    0x74, 0x5F, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72,  // 0x90
    0x5F, 0x72, 0x75, 0x73, 0x74, 0x5F, 0x77, 0x72,  //
    0x69, 0x74, 0x65, 0x5F, 0x78, 0x75, 0x6E, 0x69,  // 0xa0
    0x6F, 0x6E, 0x23, 0x6D, 0x65, 0x74, 0x61, 0x2F,  //
    0x66, 0x69, 0x64, 0x6C, 0x5F, 0x63, 0x6F, 0x6D,  // 0xb0
    0x70, 0x61, 0x74, 0x69, 0x62, 0x69, 0x6C, 0x69,  //
    0x74, 0x79, 0x5F, 0x74, 0x65, 0x73, 0x74, 0x5F,  // 0xc0
    0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x5F, 0x72,  //
    0x75, 0x73, 0x74, 0x5F, 0x77, 0x72, 0x69, 0x74,  // 0xd0
    0x65, 0x5F, 0x78, 0x75, 0x6E, 0x69, 0x6F, 0x6E,  //
    0x2E, 0x63, 0x6D, 0x78, 0x00, 0x00, 0x00, 0x00,  // 0xe0
};

static const uint8_t launcher_create_component_request_old[] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,  // 0x00
    0x00, 0x00, 0x00, 0x00, 0x65, 0x29, 0x3F, 0x0D,  //
    0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x10
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x20
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x30
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  // 0x40
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0x50
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,  //
    0x66, 0x75, 0x63, 0x68, 0x73, 0x69, 0x61, 0x2D,  // 0x60
    0x70, 0x6B, 0x67, 0x3A, 0x2F, 0x2F, 0x66, 0x75,  //
    0x63, 0x68, 0x73, 0x69, 0x61, 0x2E, 0x63, 0x6F,  // 0x70
    0x6D, 0x2F, 0x66, 0x69, 0x64, 0x6C, 0x5F, 0x63,  //
    0x6F, 0x6D, 0x70, 0x61, 0x74, 0x69, 0x62, 0x69,  // 0x80
    0x6C, 0x69, 0x74, 0x79, 0x5F, 0x74, 0x65, 0x73,  //
    0x74, 0x5F, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72,  // 0x90
    0x5F, 0x72, 0x75, 0x73, 0x74, 0x5F, 0x77, 0x72,  //
    0x69, 0x74, 0x65, 0x5F, 0x78, 0x75, 0x6E, 0x69,  // 0xa0
    0x6F, 0x6E, 0x23, 0x6D, 0x65, 0x74, 0x61, 0x2F,  //
    0x66, 0x69, 0x64, 0x6C, 0x5F, 0x63, 0x6F, 0x6D,  // 0xb0
    0x70, 0x61, 0x74, 0x69, 0x62, 0x69, 0x6C, 0x69,  //
    0x74, 0x79, 0x5F, 0x74, 0x65, 0x73, 0x74, 0x5F,  // 0xc0
    0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x5F, 0x72,  //
    0x75, 0x73, 0x74, 0x5F, 0x77, 0x72, 0x69, 0x74,  // 0xd0
    0x65, 0x5F, 0x78, 0x75, 0x6E, 0x69, 0x6F, 0x6E,  //
    0x2E, 0x63, 0x6D, 0x78, 0x00, 0x00, 0x00, 0x00,  // 0xe0
};

static const uint8_t regression9_response_v1[] = {
    0x01, 0x00, 0x00, 0x00,  // txn header
    0x01, 0x00, 0x00, 0x01,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x69, 0xc9, 0xcb, 0x56,  //

    // 16: result union xunion header
    0x01, 0x00, 0x00, 0x00,  // ordinal (success)
    0x00, 0x00, 0x00, 0x00,  //
    0x68, 0x00, 0x00, 0x00,  // num bytes
    0x00, 0x00, 0x00, 0x00,  // num handles
    0xff, 0xff, 0xff, 0xff,  // presence
    0xff, 0xff, 0xff, 0xff,  //

    // 40: Unions.u xunion header
    0x01, 0x00, 0x00, 0x00,  // ordinal
    0x00, 0x00, 0x00, 0x00,  //
    0x30, 0x00, 0x00, 0x00,  // num bytes
    0x00, 0x00, 0x00, 0x00,  // num handles
    0xff, 0xff, 0xff, 0xff,  // presence
    0xff, 0xff, 0xff, 0xff,  //

    // 64: Unions.nullable_u xunion header
    0x02, 0x00, 0x00, 0x00,  // ordinal
    0x00, 0x00, 0x00, 0x00,  //
    0x08, 0x00, 0x00, 0x00,  // num bytes
    0x00, 0x00, 0x00, 0x00,  // num handles
    0xff, 0xff, 0xff, 0xff,  // presence
    0xff, 0xff, 0xff, 0xff,  //

    // 88: Unions.u.s vector header
    0x20, 0x00, 0x00, 0x00,  // num bytes
    0x00, 0x00, 0x00, 0x00,  //
    0xff, 0xff, 0xff, 0xff,  // presence
    0xff, 0xff, 0xff, 0xff,  //

    // 104: Unions.u.s.data
    0xf2, 0x87, 0xa3, 0xb4,  //
    0x4c, 0xf1, 0x9f, 0x9b,  //
    0x83, 0xf3, 0x86, 0xa9,  //
    0xa0, 0xf2, 0x93, 0xa4,  //
    0xab, 0xf0, 0xb4, 0x81,  //
    0xb9, 0xf0, 0xbb, 0x95,  //
    0xb7, 0xf1, 0xa7, 0x93,  //
    0xac, 0xe9, 0x95, 0x85,  //

    // 136: Unions.nullable_u.data
    0x00, 0x00, 0x00, 0x00,  // false
    0x00, 0x00, 0x00, 0x00,  // padding
};

static const uint8_t regression9_response_old[] = {
    0x01, 0x00, 0x00, 0x00,  // header
    0x01, 0x00, 0x00, 0x01,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x69, 0xc9, 0xcb, 0x56,  //

    // 16: result union xunion header
    0x00, 0x00, 0x00, 0x00,  // tag (success)
    0x00, 0x00, 0x00, 0x00,  //

    // 32: Unions.u
    0x00, 0x00, 0x00, 0x00,  // tag (0)
    0x00, 0x00, 0x00, 0x00,  //
    0x20, 0x00, 0x00, 0x00,  // data num bytes
    0x00, 0x00, 0x00, 0x00,  //
    0xff, 0xff, 0xff, 0xff,  // data presence
    0xff, 0xff, 0xff, 0xff,  //

    // 56: Unions.nullable_u
    0xff, 0xff, 0xff, 0xff,  // presence
    0xff, 0xff, 0xff, 0xff,  //

    // 64: Unions.u data
    0xf2, 0x87, 0xa3, 0xb4,  //
    0x4c, 0xf1, 0x9f, 0x9b,  //
    0x83, 0xf3, 0x86, 0xa9,  //
    0xa0, 0xf2, 0x93, 0xa4,  //
    0xab, 0xf0, 0xb4, 0x81,  //
    0xb9, 0xf0, 0xbb, 0x95,  //
    0xb7, 0xf1, 0xa7, 0x93,  //
    0xac, 0xe9, 0x95, 0x85,  //

    // 70 Unions.nullable u data
    0x01, 0x00, 0x00, 0x00,  // tag
    0x00, 0x00, 0x00, 0x00,  // data (false)
    0x00, 0x00, 0x00, 0x00,  // padding
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
    0x00, 0x00, 0x00, 0x00,  //
};

static bool run_fidl_transform(const fidl_type_t* v1_type, const fidl_type_t* old_type,
                               const uint8_t* v1_bytes, uint32_t v1_num_bytes,
                               const uint8_t* old_bytes, uint32_t old_num_bytes) {
  BEGIN_HELPER;

  // TODO(apang): Refactor this function out so that we have individual test
  // cases for V1ToOld and OldToV1.

  {
    uint8_t actual_old_bytes[ZX_CHANNEL_MAX_MSG_BYTES];
    uint32_t actual_old_num_bytes;
    memset(actual_old_bytes, 0xcc /* poison */, ZX_CHANNEL_MAX_MSG_BYTES);

    const char* error = NULL;
    zx_status_t status =
        fidl_transform(FIDL_TRANSFORMATION_V1_TO_OLD, v1_type, v1_bytes, v1_num_bytes,
                       actual_old_bytes, old_num_bytes, &actual_old_num_bytes, &error);
    if (error) {
      printf("ERROR: %s\n", error);
    }

    ASSERT_EQ(status, ZX_OK, "");
    ASSERT_TRUE_NOMSG(
        cmp_payload(actual_old_bytes, actual_old_num_bytes, old_bytes, old_num_bytes));
  }

  {
    uint8_t actual_v1_bytes[ZX_CHANNEL_MAX_MSG_BYTES];
    uint32_t actual_v1_num_bytes;
    memset(actual_v1_bytes, 0xcc /* poison */, ZX_CHANNEL_MAX_MSG_BYTES);

    const char* error = NULL;
    zx_status_t status =
        fidl_transform(FIDL_TRANSFORMATION_OLD_TO_V1, old_type, old_bytes, old_num_bytes,
                       actual_v1_bytes, v1_num_bytes, &actual_v1_num_bytes, &error);
    if (error) {
      printf("ERROR: %s\n", error);
    }

    ASSERT_EQ(status, ZX_OK, "");
    ASSERT_TRUE_NOMSG(cmp_payload(actual_v1_bytes, actual_v1_num_bytes, v1_bytes, v1_num_bytes));
  }

  END_HELPER;
}

static bool sandwich1_with_hdr(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(
      run_fidl_transform(&v1_example_FakeProtocolSendSandwich1RequestTable,
                         &example_FakeProtocolSendSandwich1RequestTable,
                         sandwich1_case1_with_hdr_v1, sizeof(sandwich1_case1_with_hdr_v1),
                         sandwich1_case1_with_hdr_old, sizeof(sandwich1_case1_with_hdr_old)));

  END_TEST;
}

static bool sandwich4_with_hdr(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(
      run_fidl_transform(&v1_example_FakeProtocolWrapSandwich4RequestTable,
                         &example_FakeProtocolWrapSandwich4RequestTable,
                         sandwich4_with_hdr_case1_v1, sizeof(sandwich4_with_hdr_case1_v1),
                         sandwich4_with_hdr_case1_old, sizeof(sandwich4_with_hdr_case1_old)));

  END_TEST;
}

static bool sandwich5_case1_with_hdr(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(
      run_fidl_transform(&v1_example_FakeProtocolSendSandwich5RequestTable,
                         &example_FakeProtocolSendSandwich5RequestTable,
                         sandwich5_case1_with_hdr_v1, sizeof(sandwich5_case1_with_hdr_v1),
                         sandwich5_case1_with_hdr_old, sizeof(sandwich5_case1_with_hdr_old)));

  END_TEST;
}

static bool sandwich5_case2_with_hdr(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(
      run_fidl_transform(&v1_example_FakeProtocolSendSandwich5RequestTable,
                         &example_FakeProtocolSendSandwich5RequestTable,
                         sandwich5_case2_with_hdr_v1, sizeof(sandwich5_case2_with_hdr_v1),
                         sandwich5_case2_with_hdr_old, sizeof(sandwich5_case2_with_hdr_old)));

  END_TEST;
}

static bool sandwich6_case5(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(run_fidl_transform(&v1_example_Sandwich6Table, &example_Sandwich6Table,
                                       sandwich6_case5_v1, sizeof(sandwich6_case5_v1),
                                       sandwich6_case5_old, sizeof(sandwich6_case5_old)));

  END_TEST;
}

static bool sandwich7_case1_with_hdr(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(
      run_fidl_transform(&v1_example_FakeProtocolSendSandwich7RequestTable,
                         &example_FakeProtocolSendSandwich7RequestTable,
                         sandwich7_case1_with_hdr_v1, sizeof(sandwich7_case1_with_hdr_v1),
                         sandwich7_case1_with_hdr_old, sizeof(sandwich7_case1_with_hdr_old)));

  END_TEST;
}

static bool sandwich7_case2_with_hdr(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(
      run_fidl_transform(&v1_example_FakeProtocolSendSandwich7RequestTable,
                         &example_FakeProtocolSendSandwich7RequestTable,
                         sandwich7_case2_with_hdr_v1, sizeof(sandwich7_case2_with_hdr_v1),
                         sandwich7_case2_with_hdr_old, sizeof(sandwich7_case2_with_hdr_old)));

  END_TEST;
}

static bool regression5_enums(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(run_fidl_transform(&v1_example_Regression5Table, &example_Regression5Table,
                                       regression5_old_and_v1, sizeof(regression5_old_and_v1),
                                       regression5_old_and_v1, sizeof(regression5_old_and_v1)));

  END_TEST;
}

static bool regression6_bits(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(run_fidl_transform(&v1_example_Regression6Table, &example_Regression6Table,
                                       regression6_old_and_v1, sizeof(regression6_old_and_v1),
                                       regression6_old_and_v1, sizeof(regression6_old_and_v1)));

  END_TEST;
}

static bool mixed_fields(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(run_fidl_transform(&v1_example_FakeProtocolSendMixedFieldsRequestTable,
                                       &example_FakeProtocolSendMixedFieldsRequestTable,
                                       mixed_fields_v1, sizeof(mixed_fields_v1), mixed_fields_old,
                                       sizeof(mixed_fields_old)));

  END_TEST;
}

static bool stringunionstructwrapperresponse(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(run_fidl_transform(
      &v1_example_StringUnionStructWrapperProtocolTheMethodResponseTable,
      &example_StringUnionStructWrapperProtocolTheMethodResponseTable,
      stringunionstructwrapperresponse_v1, sizeof(stringunionstructwrapperresponse_v1),
      stringunionstructwrapperresponse_old, sizeof(stringunionstructwrapperresponse_old)));

  END_TEST;
}

static bool regression_no_union_launcher_create_component_request(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(run_fidl_transform(
      &v1_example_FakeProtocolSendFakeLauncherCreateComponentRequestRequestTable,
      &example_FakeProtocolSendFakeLauncherCreateComponentRequestRequestTable,
      launcher_create_component_request_v1, sizeof(launcher_create_component_request_v1),
      launcher_create_component_request_old, sizeof(launcher_create_component_request_old)));

  END_TEST;
}

static bool regression9_response(void) {
  BEGIN_TEST;

  ASSERT_TRUE_NOMSG(run_fidl_transform(&v1_example_FakeProtocolRegression9ResponseTable,
                                       &example_FakeProtocolRegression9ResponseTable,
                                       regression9_response_v1, sizeof(regression9_response_v1),
                                       regression9_response_old, sizeof(regression9_response_old)));

  END_TEST;
}

static bool fails_on_bad_transformation(void) {
  BEGIN_TEST;

  uint8_t dst_bytes[ZX_CHANNEL_MAX_MSG_BYTES];
  uint32_t out_dst_num_bytes;
  const char* error = NULL;
  fidl_transformation_t transform = 0x12345678;
  const zx_status_t status = fidl_transform(transform, &example_Sandwich1Table, sandwich1_case1_old,
                                            (uint32_t)(sizeof(sandwich1_case1_old)), dst_bytes,
                                            ZX_CHANNEL_MAX_MSG_BYTES, &out_dst_num_bytes, &error);
  ASSERT_EQ(status, ZX_ERR_INVALID_ARGS, "");
  ASSERT_NONNULL(error, "");
  ASSERT_STR_STR(error, "unsupported transformation", "");

  END_TEST;
}

static bool fails_if_does_not_read_src_num_bytes(void) {
  BEGIN_TEST;

  const uint32_t actual_src_bytes_size = (uint32_t)(sizeof(sandwich1_case1_old));
  uint8_t src_bytes[sizeof(sandwich1_case1_old) + 1];
  memcpy(src_bytes, sandwich1_case1_old, actual_src_bytes_size);
  src_bytes[actual_src_bytes_size] = 0;

  for (uint32_t adjust = 0; adjust <= 1; adjust++) {
    uint8_t dst_bytes[ZX_CHANNEL_MAX_MSG_BYTES];
    uint32_t out_dst_num_bytes;
    const zx_status_t status =
        fidl_transform(FIDL_TRANSFORMATION_OLD_TO_V1, &example_Sandwich1Table, src_bytes,
                       actual_src_bytes_size + adjust, dst_bytes, ZX_CHANNEL_MAX_MSG_BYTES,
                       &out_dst_num_bytes, NULL);
    if (adjust == 1) {
      ASSERT_EQ(status, ZX_ERR_INVALID_ARGS, "");
    } else {
      ASSERT_EQ(status, ZX_OK, "");
    }
  }

  END_TEST;
}

// Most tests in this file have been ported to GIDL:
//
//     tools/fidl/gidl-conformance-suite/transformer.gidl
//     tools/fidl/gidl-conformance-suite/transformer.test.fidl
//
// New transformer tests should go there. Some existing tests cannot (yet) be
// faithfully expressed in GIDL; they are listed below with explanation.

BEGIN_TEST_CASE(transformer)

// These tests verify failure modes of the `fidl_transform` function, not its
// input/output behavior. They are not in GIDL.
RUN_TEST(fails_on_bad_transformation)
RUN_TEST(fails_if_does_not_read_src_num_bytes)

// These tests use request/response types, and include headers. They are
// simulated in GIDL with a struct that matches the transaction header layout,
// but remain here too because this is testing something slightly different.
RUN_TEST(sandwich1_with_hdr)
RUN_TEST(sandwich4_with_hdr)
RUN_TEST(sandwich5_case1_with_hdr)
RUN_TEST(sandwich5_case2_with_hdr)
RUN_TEST(sandwich7_case1_with_hdr)
RUN_TEST(sandwich7_case2_with_hdr)
RUN_TEST(stringunionstructwrapperresponse)
RUN_TEST(mixed_fields)
RUN_TEST(regression_no_union_launcher_create_component_request)
RUN_TEST(regression9_response)

// These tests use enums, bits, and handles. They are simulated in GIDL with
// integer types, but remain here too because their entire purpose is to
// exercise the transformer with those types.
RUN_TEST(regression5_enums)
RUN_TEST(regression6_bits)
RUN_TEST(sandwich6_case5)

END_TEST_CASE(transformer)
