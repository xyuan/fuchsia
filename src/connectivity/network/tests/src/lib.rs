// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#![cfg(test)]

mod dhcp;
mod environments;
mod fidl;
mod ipv6;

type Result<T = ()> = std::result::Result<T, anyhow::Error>;
