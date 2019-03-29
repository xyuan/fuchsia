// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddk/binding.h>
#include <ddk/driver.h>
#include <ddk/platform-defs.h>

extern zx_status_t pdm_audio_bind(void* ctx, zx_device_t* parent);

static zx_driver_ops_t aml_pdm_driver_ops = {
    .version = DRIVER_OPS_VERSION,
    .bind = pdm_audio_bind,
};

// clang-format off
ZIRCON_DRIVER_BEGIN(aml_pdm, aml_pdm_driver_ops, "zircon", "0.1", 4)
    BI_ABORT_IF(NE, BIND_PROTOCOL, ZX_PROTOCOL_PDEV),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_VID, PDEV_VID_AMLOGIC),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_PID, PDEV_PID_AMLOGIC_T931),
    BI_MATCH_IF(EQ, BIND_PLATFORM_DEV_DID, PDEV_DID_SHERLOCK_PDM),
ZIRCON_DRIVER_END(aml_pdm)
// clang-format on
