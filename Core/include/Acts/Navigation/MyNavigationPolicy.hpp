#pragma once

#include "Acts/Navigation/INavigationPolicy.hpp"
#include "Acts/Navigation/NavigationStream.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include <memory>

namespace Acts {

class TrackingVolume;
class GeometryContext;
class Logger;

class MyNavigationPolicy final : public INavigationPolicy {
  public:
	enum class LayerType { Cylinder, Disc, Plane };
	struct Config {
		bool portals = true;
		bool sensitives = true;
		bool passives = true;
		std::vector<std::pair<std::size_t, std::size_t>> bins
		    = { { 10, 1 }, { 12, 1 }, { 16, 1 } };
	};
	const SurfaceArray& surfaceArray() const;

	/// Constructor from a volume
	/// @param gctx is the geometry context
	/// @param volume is the volume to navigate
	/// @param logger is the logger
	/// @param config The configuration for the policy
	MyNavigationPolicy(const GeometryContext& gctx, const TrackingVolume& volume,
	    const Logger& logger, const Config& config);

	/// Constructor from a volume
	/// @param gctx is the geometry context
	/// @param volume is the volume to navigate
	/// @param logger is the logger
	MyNavigationPolicy(
	    const GeometryContext& gctx, const TrackingVolume& volume, const Logger& logger);

	/// Add all candidates to the stream
	/// @param gctx is the geometry context
	/// @param args are the navigation arguments
	/// @param stream is the navigation stream to update
	/// @param logger is the logger
	void initializeCandidates(const GeometryContext& gctx, const NavigationArguments& args,
	    AppendOnlyNavigationStream& stream, const Logger& logger) const;

	/// Connect the policy to a navigation delegate
	/// @param delegate is the navigation delegate
	void connect(NavigationDelegate& delegate) const override;

  private:
	struct LayerInfo {
		double radius;
		double radiusSq;
		double halfZ;
		const Acts::TrackingVolume* volume;
		const Portal* portal = nullptr;
	};
	std::vector<LayerInfo> m_layerCache;
	Config m_cfg;
	const TrackingVolume* m_volume;
	std::optional<Transform3> m_itransform;

	double m_halfLengthZ;
	double m_rMin2;
	double m_rMax2;

	std::array<const Portal*, 4> m_portals {};
	std::unique_ptr<SurfaceArray> m_surfaceArrayL4 {};
	std::unique_ptr<SurfaceArray> m_surfaceArrayL5 {};
	std::unique_ptr<SurfaceArray> m_surfaceArrayL6 {};
};

static_assert(NavigationPolicyConcept<MyNavigationPolicy>);

} // namespace Acts
