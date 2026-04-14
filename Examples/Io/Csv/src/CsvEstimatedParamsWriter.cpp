#include "ActsExamples/Io/Csv/CsvTrackParameterWriter.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Io/Csv/CsvInputOutput.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <optional>
#include <stdexcept>

#include "CsvOutputData.hpp"
#include <cmath>     // Für std::log, std::tan
#include <fstream>   // Für std::ofstream
#include <iomanip>   // Für std::setprecision
#include <stdexcept> // Für Exceptions

#include "ActsExamples/Io/Csv/CsvEstimatedParamsWriter.hpp"

ActsExamples::CsvEstimatedParamsWriter::CsvEstimatedParamsWriter(
    const Config& config,
    Acts::Logging::Level level)
    : WriterT(config.inputCollection,"CsvEstimatedParamsWriter", level),
    m_cfg(config) {

    if (m_cfg.inputCollection.empty()){
        throw std::invalid_argument("Input collection missing");

    }
}
ActsExamples::CsvEstimatedParamsWriter::~CsvEstimatedParamsWriter() = default;

ActsExamples::ProcessCode ActsExamples::CsvEstimatedParamsWriter::writeT(
    const AlgorithmContext& ctx,
    const TrackParametersContainer& trackParams){

    std::string path = perEventFilepath(
        m_cfg.outputDir, m_cfg.outputStem+".csv",ctx.eventNumber);

    std::ofstream os(path, std::ofstream::out | std::ofstream::trunc);
    if(!os){
        throw std::ios_base::failure("Could not open '" + path + "' to write");
    }

    os << "loc0,loc1,phi,theta,qop,time,var_d0,var_z0,var_phi,var_theta,var_qop,cov_d0z0,cov_d0phi,cov_d0theta,cov_d0qop,cov_z0d0,cov_z0phi,cov_z0theta,cov_z0qop,cov_phid0,cov_phiz0,cov_phitheta,cov_phiqop,cov_thetad0,cov_thetaz0,cov_thetaphi,cov_thetaqop,cov_qopd0,cov_qopz0,cov_qopphi,cov_qoptheta\n";
    os << std::setprecision(m_cfg.outputPrecision);

    for (const auto& params : trackParams) {
        auto p = params.parameters();

        double pt = params.transverseMomentum();
        
        double eta = -std::log(std::tan(p[Acts::eBoundTheta]/2.0));

        const auto& cov = params.covariance().value();

        os << p[Acts::eBoundLoc0] << "," 
       << p[Acts::eBoundLoc1] << ","
       << p[Acts::eBoundPhi] << ","
       << p[Acts::eBoundTheta] << ","
       << p[Acts::eBoundQOverP] << ","
       << p[Acts::eBoundTime] << ","

       <<cov(Acts::eBoundLoc0, Acts::eBoundLoc0)<< ","
       <<cov(Acts::eBoundLoc1, Acts::eBoundLoc1)<< ","
       <<cov(Acts::eBoundPhi, Acts::eBoundPhi)<< ","
       <<cov(Acts::eBoundTheta, Acts::eBoundTheta)<< ","
       <<cov(Acts::eBoundQOverP, Acts::eBoundQOverP)<< ","

       <<cov(Acts::eBoundLoc0, Acts::eBoundLoc1)<< ","
       <<cov(Acts::eBoundLoc0, Acts::eBoundPhi)<< ","
       <<cov(Acts::eBoundLoc0, Acts::eBoundTheta)<< ","
       <<cov(Acts::eBoundLoc0, Acts::eBoundQOverP)<< ","

       <<cov(Acts::eBoundLoc1, Acts::eBoundLoc0)<< ","
       <<cov(Acts::eBoundLoc1, Acts::eBoundPhi)<< ","
       <<cov(Acts::eBoundLoc1, Acts::eBoundTheta)<< ","
       <<cov(Acts::eBoundLoc1, Acts::eBoundQOverP)<< ","

       <<cov(Acts::eBoundPhi, Acts::eBoundLoc0)<< ","
       <<cov(Acts::eBoundPhi, Acts::eBoundLoc1)<< ","
       <<cov(Acts::eBoundPhi, Acts::eBoundTheta)<< ","
       <<cov(Acts::eBoundPhi, Acts::eBoundQOverP)<< ","

       <<cov(Acts::eBoundTheta, Acts::eBoundLoc0)<< ","
       <<cov(Acts::eBoundTheta, Acts::eBoundLoc1)<< ","
       <<cov(Acts::eBoundTheta, Acts::eBoundPhi)<< ","
       <<cov(Acts::eBoundTheta, Acts::eBoundQOverP)<< ","

       <<cov(Acts::eBoundQOverP, Acts::eBoundLoc0)<< ","
       <<cov(Acts::eBoundQOverP, Acts::eBoundLoc1)<< ","
       <<cov(Acts::eBoundQOverP, Acts::eBoundPhi)<< ","
       <<cov(Acts::eBoundQOverP, Acts::eBoundTheta)<<"\n";
    }
    return ProcessCode::SUCCESS;
    }
