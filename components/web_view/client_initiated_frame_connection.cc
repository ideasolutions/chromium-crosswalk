// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_view/client_initiated_frame_connection.h"

#include <utility>

namespace web_view {

ClientInitiatedFrameConnection::ClientInitiatedFrameConnection(
    mojom::FrameClientPtr frame_client)
    : frame_client_(std::move(frame_client)) {}

ClientInitiatedFrameConnection::~ClientInitiatedFrameConnection() {}

}  // namespace web_view
