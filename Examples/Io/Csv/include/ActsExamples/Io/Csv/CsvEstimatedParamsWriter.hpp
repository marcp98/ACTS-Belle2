
#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IWriter.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace ActsExamples {
struct AlgorithmContext;

class CsvEstimatedParamsWriter final : public WriterT<TrackParametersContainer> {
    public:
        struct Config {
            std::string inputCollection;

            std::string outputDir;
            
            std::string outputStem;

            std::size_t outputPrecision = std::numeric_limits<float>::max_digits10;
        };

        CsvEstimatedParamsWriter(const Config& config, Acts::Logging::Level level);

        ~CsvEstimatedParamsWriter() override;

        ProcessCode writeT(const AlgorithmContext& ctx, const TrackParametersContainer& trackParams) override;

        const Config& config() const { return m_cfg; }
        
    private:
        Config m_cfg;
};

}