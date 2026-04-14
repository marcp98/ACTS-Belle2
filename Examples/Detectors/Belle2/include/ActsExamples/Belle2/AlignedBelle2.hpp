// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/DetectorCommons/Aligned.hpp"
#include "ActsExamples/Belle2/Belle2.hpp"
#include "ActsExamples/Belle2/Belle2Element.hpp"

namespace ActsExamples {
/// Define the aligned DD4hep detector element and factory type
using AlignedBelle2Element = Aligned<Belle2Element>;

class AlignedBelle2 : public Belle2 {
 public:
  using Config = Belle2::Config;
  explicit AlignedBelle2(const Config& cfg);
};

}  // namespace ActsExamples
