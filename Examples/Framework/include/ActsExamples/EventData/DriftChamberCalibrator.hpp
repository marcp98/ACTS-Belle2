#pragma once

#include "ActsExamples/EventData/MeasurementCalibration.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"

namespace ActsExamples {

class DriftChamberCalibrator : public MeasurementCalibrator {
public:
  void calibrate(
      const MeasurementContainer& measurements,
      const ClusterContainer* clusters, 
      const Acts::GeometryContext& gctx,
      const Acts::CalibrationContext& cctx, 
      const Acts::SourceLink& sourceLink,
      Acts::VectorMultiTrajectory::TrackStateProxy& trackState) const override;
};

} // namespace ActsExamples