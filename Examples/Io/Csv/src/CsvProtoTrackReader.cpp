#include "ActsExamples/Io/Csv/CsvProtoTrackReader.hpp"

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


ActsExamples::CsvProtoTrackReader::CsvProtoTrackReader(
    const ActsExamples::CsvProtoTrackReader::Config& config,
    Acts::Logging::Level level)
    :   m_cfg(config),
        m_eventsRange(
            determineEventFilesRange(m_cfg.inputDir,m_cfg.inputStem + ".csv")),
        m_logger(Acts::getDefaultLogger("CsvProtoTrackReader", level)) {
    
    if (m_cfg.inputStem.empty()){
        throw std::invalid_argument("Missing Input filename stem");
    }
    if (m_cfg.outputProtoTracks.empty()){
        throw std::invalid_argument("Missing output collection");
    }
    m_outputProtoTracks.initialize(m_cfg.outputProtoTracks);
    }
std::string
ActsExamples::CsvProtoTrackReader::name() const {
    return "CsvProtoTrackReader";
}
std::pair<std::size_t, std::size_t>
ActsExamples::CsvProtoTrackReader::availableEvents() const {
    return m_eventsRange;
}

ActsExamples::ProcessCode ActsExamples::CsvProtoTrackReader::read(
    const ActsExamples::AlgorithmContext& ctx) {
    ProtoTrackContainer protoTracks;
    
    auto path = perEventFilepath(m_cfg.inputDir, m_cfg.inputStem + ".csv", ctx.eventNumber);

    BoostDescribeCsvReader<ProtoTrackData> reader(path);
    ProtoTrackData d{};
    
    while (reader.read(d)) {
        if (protoTracks.size() <= d.trackId) {
            protoTracks.resize(d.trackId + 1);
        }

        // Wir fügen nur die measurementId zum entsprechenden Track hinzu.
        // Koordinaten (x, y, z) werden im ProtoTrack-Objekt selbst meistens 
        // nicht gespeichert, da diese über die SourceLinks/Measurements geholt werden.
        protoTracks[d.trackId].push_back(d.measurementId);
  }
    m_outputProtoTracks(ctx, std::move(protoTracks));

    return ProcessCode::SUCCESS;
}
}