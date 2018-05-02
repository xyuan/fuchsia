// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "garnet/bin/appmgr/application_runner_holder.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>

#include "lib/fsl/vmo/file.h"

namespace component {

ApplicationRunnerHolder::ApplicationRunnerHolder(
    Services services,
    ApplicationControllerPtr controller)
    : services_(std::move(services)), controller_(std::move(controller)) {
  services_.ConnectToService(runner_.NewRequest());
}

ApplicationRunnerHolder::~ApplicationRunnerHolder() = default;

void ApplicationRunnerHolder::StartApplication(
    ApplicationPackage package, ApplicationStartupInfo startup_info,
    std::unique_ptr<archive::FileSystem> file_system, fxl::RefPtr<Namespace> ns,
    fidl::InterfaceRequest<ApplicationController> controller) {
  file_systems_.push_back(std::move(file_system));
  namespaces_.push_back(std::move(ns));
  runner_->StartApplication(std::move(package), std::move(startup_info),
                            std::move(controller));
}

}  // namespace component
