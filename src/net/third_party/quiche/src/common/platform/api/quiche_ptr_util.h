// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_COMMON_PLATFORM_API_QUICHE_PTR_UTIL_H_
#define QUICHE_COMMON_PLATFORM_API_QUICHE_PTR_UTIL_H_

#include <memory>

#include "net/quiche/common/platform/impl/quiche_ptr_util_impl.h"

namespace quiche {

template <typename T, typename... Args>
std::unique_ptr<T> QuicheMakeUnique(Args&&... args) {
  return QuicheMakeUniqueImpl<T>(std::forward<Args>(args)...);
}

}  // namespace quiche

#endif  // QUICHE_COMMON_PLATFORM_API_QUICHE_PTR_UTIL_H_
