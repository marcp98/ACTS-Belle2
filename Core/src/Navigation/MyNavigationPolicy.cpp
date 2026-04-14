#include "Acts/Navigation/MyNavigationPolicy.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Navigation/NavigationStream.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include <iostream>

namespace Acts {
struct LayerInfo {
	double radius;
	double radiusSq;
	double halfZ;
	const Acts::TrackingVolume* volume;
	const Portal* portal = nullptr;
};

MyNavigationPolicy::MyNavigationPolicy(const GeometryContext& gctx, const TrackingVolume& volume,
    const Logger& logger, const Config& config)
    : m_cfg { config }
    , m_volume(&volume) {
	assert(m_volume != nullptr);
	ACTS_VERBOSE("MyNavigationPolicy created for volume "
	    << m_volume->volumeName() << " with config: "
	    << " portals=" << m_cfg.portals << " sensitives=" << m_cfg.sensitives
	    << " passives=" << m_cfg.passives);

	SurfaceArrayCreator::Config sacConfigL5;
	SurfaceArrayCreator::Config sacConfigL6;
	SurfaceArrayCreator::Config sacConfigL4;

	sacConfigL5.doPhiBinningOptimization = false;
	sacConfigL6.doPhiBinningOptimization = false;
	sacConfigL4.doPhiBinningOptimization = false;

	SurfaceArrayCreator sacL5 { sacConfigL5, logger.clone("SrfArrCrtr5") };
	SurfaceArrayCreator sacL6 { sacConfigL6, logger.clone("SrfArrCrtr6") };
	SurfaceArrayCreator sacL4 { sacConfigL4, logger.clone("SrfArrCrtr4") };

	std::vector<std::shared_ptr<const Surface>> surfacesL5;
	std::vector<std::shared_ptr<const Surface>> surfacesL6;
	std::vector<std::shared_ptr<const Surface>> surfacesL4;
	surfacesL4.reserve(10);
	surfacesL5.reserve(12);
	surfacesL6.reserve(16);
	for (const auto& surface : volume.surfaces() | std::views::take(10)) {

		if (surface.surfacePlacement() == nullptr) {
			continue;
		}
		surfacesL4.push_back(surface.getSharedPtr());
	}

	for (const auto& surface : volume.surfaces() | std::views::drop(16)) {

		if (surface.surfacePlacement() == nullptr) {
			continue;
		}
		surfacesL6.push_back(surface.getSharedPtr());
	}
	for (const auto& surface : volume.surfaces() | std::views::drop(10) | std::views::take(10)) {

		if (surface.surfacePlacement() == nullptr) {
			continue;
		}
		surfacesL5.push_back(surface.getSharedPtr());
	}

	ACTS_VERBOSE(
	    "Number of surfaces passed to the surface array creation: " << surfacesL5.size());
	auto [binsPhiL4, binsZL4] = config.bins[0];
	auto [binsPhiL5, binsZL5] = config.bins[1];
	auto [binsPhiL6, binsZL6] = config.bins[2];
	m_surfaceArrayL5
	    = sacL5.surfaceArrayOnCylinder(gctx, std::move(surfacesL5), binsPhiL5, binsZL5);
	m_surfaceArrayL6
	    = sacL6.surfaceArrayOnCylinder(gctx, std::move(surfacesL6), binsPhiL6, binsZL6);
	m_surfaceArrayL4
	    = sacL4.surfaceArrayOnCylinder(gctx, std::move(surfacesL4), binsPhiL4, binsZL4);

	if (!m_surfaceArrayL5) {
		ACTS_ERROR("Failed to create surface array");
		throw std::runtime_error("Failed to create surface array");
	}
}

MyNavigationPolicy::MyNavigationPolicy(
    const GeometryContext& gctx, const TrackingVolume& volume, const Logger& logger)
    : MyNavigationPolicy(gctx, volume, logger, {}) { }

void MyNavigationPolicy::initializeCandidates([[maybe_unused]] const GeometryContext& gctx,
    const NavigationArguments& args, AppendOnlyNavigationStream& stream,
    const Logger& logger) const {

	ACTS_VERBOSE("MyNavigationPolicy");
	assert(m_volume != nullptr);
	if (m_cfg.portals) {
		for (const auto& portal : m_volume->portals()) {
			stream.addPortalCandidate(portal);
		}
	}

	const std::vector<const Surface*>& sensitiveSurfacesL5
	    = m_surfaceArrayL5->neighbors(args.position, args.direction);
	const std::vector<const Surface*>& sensitiveSurfacesL6
	    = m_surfaceArrayL6->neighbors(args.position, args.direction);
	const std::vector<const Surface*>& sensitiveSurfacesL4
	    = m_surfaceArrayL4->neighbors(args.position, args.direction);

	for (const Surface* surface : sensitiveSurfacesL5) {
		stream.addSurfaceCandidate(*surface, args.tolerance);
	};
	for (const Surface* surface : sensitiveSurfacesL4) {
		stream.addSurfaceCandidate(*surface, args.tolerance);
	};
	for (const Surface* surface : sensitiveSurfacesL6) {
		stream.addSurfaceCandidate(*surface, args.tolerance);
	};
}

void MyNavigationPolicy::connect(NavigationDelegate& delegate) const {
	connectDefault<MyNavigationPolicy>(delegate);
}

} // namespace Acts
