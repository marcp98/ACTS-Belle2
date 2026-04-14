#include "ActsExamples/Io/Csv/CsvEstimatedParamsReader.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "CsvOutputData.hpp"

namespace ActsExamples {

struct TrackParameterData2 {
  double loc0;
  double loc1;
  double phi;
  double theta;
  double qop;
  double time;
  double var_d0, var_z0, var_phi, var_theta, var_qop;

  double cov_d0z0, cov_d0phi, cov_d0theta, cov_d0qop;
  double cov_z0d0, cov_z0phi, cov_z0theta, cov_z0qop;
  double cov_phid0, cov_phiz0, cov_phitheta, cov_phiqop;
  double cov_thetad0, cov_thetaz0, cov_thetaphi, cov_thetaqop;
  double cov_qopd0, cov_qopz0, cov_qopphi, cov_qoptheta;
};
  BOOST_DESCRIBE_STRUCT(TrackParameterData2, (), (loc0, loc1, phi, theta, qop, time,var_d0,
                 var_z0, var_phi, var_theta, var_qop, cov_d0z0, cov_d0phi,
                 cov_d0theta, cov_d0qop, cov_z0d0, cov_z0phi, cov_z0theta,
                 cov_z0qop, cov_phid0, cov_phiz0, cov_phitheta, cov_phiqop,
                 cov_thetad0, cov_thetaz0, cov_thetaphi, cov_thetaqop,
                 cov_qopd0, cov_qopz0, cov_qopphi, cov_qoptheta));


ActsExamples::CsvEstimatedParamsReader::CsvEstimatedParamsReader(
    const ActsExamples::CsvEstimatedParamsReader::Config& config,
    Acts::Logging::Level level)
    :   m_cfg(config),
        m_eventsRange(
            determineEventFilesRange(m_cfg.inputDir,m_cfg.inputStem + ".csv")),
        m_logger(Acts::getDefaultLogger("CsvEstimatedParamsReader", level)) {
    
    if (m_cfg.inputStem.empty()){
        throw std::invalid_argument("Missing Input filename stem");
    }
    if (m_cfg.outputTrackParameter.empty()){
        throw std::invalid_argument("Missing output collection");
    }
    m_outputTrackParameters.initialize(m_cfg.outputTrackParameter);
    }
std::string
ActsExamples::CsvEstimatedParamsReader::name() const {
    return "CsvEstimatedParamsReader";
}
std::pair<std::size_t, std::size_t>
ActsExamples::CsvEstimatedParamsReader::availableEvents() const {
    return m_eventsRange;
}

ActsExamples::ProcessCode ActsExamples::CsvEstimatedParamsReader::read(
    const ActsExamples::AlgorithmContext& ctx) {
    TrackParametersContainer trackParameters;
    
    auto surface = Acts::Surface::makeShared<Acts::PerigeeSurface>(
    Acts::Vector3(m_cfg.beamspot[0], m_cfg.beamspot[1], m_cfg.beamspot[2]));

    auto path = perEventFilepath(m_cfg.inputDir, m_cfg.inputStem + ".csv", ctx.eventNumber);

    BoostDescribeCsvReader<TrackParameterData2> reader(path);
    TrackParameterData2 d{};
    
    while (reader.read(d)) {
        Acts::BoundVector params = Acts::BoundVector::Zero();
        params[Acts::eBoundLoc0] = d.loc0;
        params[Acts::eBoundLoc1] = d.loc1;
        params[Acts::eBoundPhi] = d.phi;
        params[Acts::eBoundTheta] = d.theta;
        params[Acts::eBoundQOverP] = d.qop;
        params[Acts::eBoundTime] = d.time;
        
        Acts::BoundSquareMatrix cov = Acts::BoundSquareMatrix::Zero();
        cov(Acts::eBoundLoc0, Acts::eBoundLoc0) = d.var_d0;
        cov(Acts::eBoundLoc1, Acts::eBoundLoc1) = d.var_z0;
        cov(Acts::eBoundPhi, Acts::eBoundPhi) = d.var_phi;
        cov(Acts::eBoundTheta, Acts::eBoundTheta) = d.var_theta;
        cov(Acts::eBoundQOverP, Acts::eBoundQOverP) = d.var_qop;
        cov(Acts::eBoundTime, Acts::eBoundTime) = 1;

        cov(Acts::eBoundLoc0, Acts::eBoundLoc1) = d.cov_d0z0;
        cov(Acts::eBoundLoc0, Acts::eBoundPhi) = d.cov_d0phi;
        cov(Acts::eBoundLoc0, Acts::eBoundTheta) = d.cov_d0theta;
        cov(Acts::eBoundLoc0, Acts::eBoundQOverP) = d.cov_d0qop;

        cov(Acts::eBoundLoc1, Acts::eBoundLoc0) = d.cov_z0d0;
        cov(Acts::eBoundLoc1, Acts::eBoundPhi) = d.cov_z0phi;
        cov(Acts::eBoundLoc1, Acts::eBoundTheta) = d.cov_z0theta;
        cov(Acts::eBoundLoc1, Acts::eBoundQOverP) = d.cov_z0qop;

        cov(Acts::eBoundPhi, Acts::eBoundLoc0) = d.cov_phid0;
        cov(Acts::eBoundPhi, Acts::eBoundLoc1) = d.cov_phiz0;
        cov(Acts::eBoundPhi, Acts::eBoundTheta) = d.cov_phitheta;
        cov(Acts::eBoundPhi, Acts::eBoundQOverP) = d.cov_phiqop;

        cov(Acts::eBoundTheta, Acts::eBoundLoc0) = d.cov_thetad0;
        cov(Acts::eBoundTheta, Acts::eBoundLoc1) = d.cov_thetaz0;
        cov(Acts::eBoundTheta, Acts::eBoundPhi) = d.cov_thetaphi;
        cov(Acts::eBoundTheta, Acts::eBoundQOverP) = d.cov_thetaqop;

        cov(Acts::eBoundQOverP, Acts::eBoundLoc0) = d.cov_qopd0;
        cov(Acts::eBoundQOverP, Acts::eBoundLoc1) = d.cov_qopz0;
        cov(Acts::eBoundQOverP, Acts::eBoundPhi) = d.cov_qopphi;
        cov(Acts::eBoundQOverP, Acts::eBoundTheta) = d.cov_qoptheta;

        trackParameters.emplace_back(surface, params, cov,
                                    Acts::ParticleHypothesis::pion());
  }
    m_outputTrackParameters(ctx, std::move(trackParameters));

    return ProcessCode::SUCCESS;
}
}