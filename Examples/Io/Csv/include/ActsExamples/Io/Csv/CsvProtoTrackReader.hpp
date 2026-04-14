#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace ActsExamples {

struct AlgorithmContext;

class CsvProtoTrackReader final : public IReader {
    public:
        struct Config {
            std::string inputDir;

            std::string inputStem;

            std::string outputProtoTracks;
        };

CsvProtoTrackReader(const Config& config, Acts::Logging::Level level);

std::string name() const final;

std::pair<std::size_t, std::size_t> availableEvents() const final;

ProcessCode read(const ActsExamples::AlgorithmContext& ctx) final;

const Config& config() const {return m_cfg;}

private:
    Config m_cfg;
    std::pair<std::size_t, std::size_t> m_eventsRange;
    std::unique_ptr<const Acts::Logger> m_logger;
    
    WriteDataHandle<ProtoTrackContainer> m_outputProtoTracks{
        this, "OutputProtoTracks"};
    const Acts::Logger& logger() const {return *m_logger;}
    
};

}