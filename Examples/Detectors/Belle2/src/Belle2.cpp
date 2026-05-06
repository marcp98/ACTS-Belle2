// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Belle2/Belle2.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/Blueprint.hpp"
#include "Acts/Geometry/BlueprintNode.hpp"
#include "Acts/Geometry/ContainerBlueprintNode.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBuilder.hpp"
#include "Acts/Geometry/CylinderVolumeHelper.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerBlueprintNode.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/MaterialDesignatorBlueprintNode.hpp"
#include "Acts/Geometry/NavigationPolicyFactory.hpp"
#include "Acts/Geometry/PassiveLayerBuilder.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/ProtoLayerHelper.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingGeometryBuilder.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"
#include "Acts/Geometry/VolumeAttachmentStrategy.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Navigation/CylinderNavigationPolicy.hpp"
#include "Acts/Navigation/MultiNavigationPolicy.hpp"
#include "Acts/Navigation/MyNavigationPolicy.hpp"
#include "Acts/Navigation/SurfaceArrayNavigationPolicy.hpp"
#include "Acts/Navigation/TryAllNavigationPolicy.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "ActsExamples/Belle2/Belle2Element.hpp"
#include "ActsExamples/Belle2/CDCWireElement.hpp"
#include "ActsExamples/Belle2/LayerBuilder.hpp"
#include "ActsExamples/Belle2/MyIdentifierHook.hpp"
#include "ActsExamples/Belle2/MyIdentifierProvider.hpp"
#include "ActsPlugins/Root/TGeoDetectorElement.hpp"
#include "ActsPlugins/Root/TGeoParser.hpp"

#include <cstddef>
#include <fstream>
#include <memory>

#include "./Belle2Builder.hpp"
#include "TGeoManager.h"

namespace ActsExamples {

namespace B2 {
namespace {

class Gen3Belle2Builder : public Belle2Builder {
 public:
  using Belle2Builder::Belle2Builder;

  std::unique_ptr<const Acts::TrackingGeometry> buildTrackingGeometry(
      const Acts::GeometryContext& gctx,
      std::optional<std::filesystem::path> graphvizFile);

  void buildSVD(Acts::Experimental::BlueprintNode& parent,
                const Acts::GeometryContext& gctx);

  void buildCDC(Acts::Experimental::BlueprintNode& parent,
                const Acts::GeometryContext& gctx);

  std::vector<ProtoLayerSurfaces> buildLayers(
      const Acts::GeometryContext& gctx);

  std::vector<ProtoLayerSurfaces> buildCDCLayers(
      const Acts::GeometryContext& gctx);

  static std::shared_ptr<const Acts::HomogeneousSurfaceMaterial> asHomogeneous(
      std::string_view debugLabel,
      const std::shared_ptr<const Acts::ISurfaceMaterial>& material) {
    auto hm = std::dynamic_pointer_cast<const Acts::HomogeneousSurfaceMaterial>(
        material);
    if (hm == nullptr) {
      throw std::runtime_error(std::string{debugLabel} +
                               " material is not homogeneous");
    }
    return hm;
  }

  using LayerType = Acts::SurfaceArrayNavigationPolicy::LayerType;
  std::unique_ptr<Acts::NavigationPolicyFactory> createNavigationPolicy(
      LayerType layerType,
      std::pair<std::size_t, std::size_t> bins = {0, 0}) const {
    using SrfArrayNavPol = Acts::SurfaceArrayNavigationPolicy;
    return Acts::NavigationPolicyFactory{}
        .add<Acts::CylinderNavigationPolicy>()
        .add<SrfArrayNavPol>(
            SrfArrayNavPol::Config{.layerType = layerType, .bins = bins})
        .asUniquePtr();
  }
  std::unique_ptr<Acts::NavigationPolicyFactory> createNavigationPolicy2()
      const {
    return Acts::NavigationPolicyFactory{}
        .add<Acts::TryAllNavigationPolicy>()
        .asUniquePtr();
  }
  std::unique_ptr<Acts::NavigationPolicyFactory> createNavigationPolicy3(
      LayerType layerType,
      std::pair<std::size_t, std::size_t> bins = {0, 0}) const {
    using SrfArrayNavPol = Acts::SurfaceArrayNavigationPolicy;
    return Acts::NavigationPolicyFactory{}
        .add<SrfArrayNavPol>(
            SrfArrayNavPol::Config{.layerType = layerType, .bins = bins})
        .add<Acts::TryAllNavigationPolicy>()
        .asUniquePtr();
  }
  std::unique_ptr<Acts::NavigationPolicyFactory> createNavigationPolicy4(
      LayerType layerType,
      std::pair<std::size_t, std::size_t> bins = {0, 0}) const {
    return Acts::NavigationPolicyFactory{}
        .add<Acts::MyNavigationPolicy>()
        .asUniquePtr();
  }
  const std::vector<std::shared_ptr<ActsPlugins::TGeoDetectorElement>>&
  getElementStore() const {
    return m_elementStore;
  }

  const std::vector<std::shared_ptr<CDCWireElement>>& getCDCElementStore()
      const {
    return m_cdcElementStore;
  }

 private:
  std::vector<std::shared_ptr<ActsPlugins::TGeoDetectorElement>> m_elementStore;

  std::vector<std::shared_ptr<CDCWireElement>> m_cdcElementStore;
};

std::unique_ptr<const Acts::TrackingGeometry>
Gen3Belle2Builder::buildTrackingGeometry(
    const Acts::GeometryContext& gctx,
    std::optional<std::filesystem::path> graphvizFile) {
  using enum Acts::AxisDirection;
  using namespace Acts::UnitLiterals;
  using namespace Acts::Experimental;
  using enum Acts::AxisBoundaryType;
  using enum Acts::CylinderVolumeBounds::Face;
  ACTS_INFO("Belle2 construction in  Gen3 mode");

  TGeoManager::Import("/work/configs/Belle2.root");  // set realpath later
  std::string cdcFile = "wireInfo.csv";

  Blueprint::Config cfg;
  cfg.envelope = Acts::ExtentEnvelope{{
      .z = {20_mm, 20_mm},
      .r = {0_mm, 20_mm},
  }};
  cfg.hook = std::make_shared<MyIdentifierHook>();
  Blueprint root{cfg};

  root.addCylinderContainer("Detector", AxisR, [&](auto& detector) {
    // auto beampipeBounds = std::make_unique<Acts::CylinderVolumeBounds>(0_mm,
    // kBeamPipeRadius, kBeamPipeHalfLengthZ); auto beampipe =
    // std::make_unique<Acts::TrackingVolume>(Acts::Transform3::Identity(),
    // std::move(beampipeBounds), "Beampipe");
    /*
    detector.addMaterial("BeampipeMaterial", [&](auto& bpMat) {
        bpMat.configureFace(OuterCylinder, asHomogeneous("Beam pipe",
    m_beamPipeMaterial)); bpMat.addStaticVolume(std::move(beampipe));
    });
    */

    buildSVD(detector, gctx);
    buildCDC(detector, gctx);
  });

  if (graphvizFile) {
    std::ofstream file(*graphvizFile);
    root.graphviz(file);
  }

  BlueprintOptions options;
  auto trackingGeometry =
      root.construct(options, gctx, *logger().clone("Blprnt"));
  ACTS_INFO("Lade material_map.json direkt in C++...");

  return trackingGeometry;
}

void Gen3Belle2Builder::buildCDC(Acts::Experimental::BlueprintNode& parent,
                                 const Acts::GeometryContext& gctx) {
  using namespace Acts::Experimental;
  using namespace Acts::UnitLiterals;
  using enum Acts::CylinderVolumeBounds::Face;

  ACTS_DEBUG("Building CDC");
  auto cdcLayers = buildCDCLayers(gctx);

  auto cdcBounds =
      std::make_shared<Acts::CylinderVolumeBounds>(150_mm, 3000_mm, 3000_mm);

  struct TrackingVolumeAccessor
      : public Acts::Experimental::StaticBlueprintNode {
    static Acts::TrackingVolume* getVolume(
        Acts::Experimental::StaticBlueprintNode& node) {
      return static_cast<TrackingVolumeAccessor&>(node).m_volume.get();
    }
  };
  parent.addMaterial("CDC_PassiveMaterial", [&](auto& matNode){
    using Face = Acts::CylinderVolumeBounds::Face;
    using enum Acts::AxisDirection;
    using enum Acts::AxisBoundaryType;

    matNode.configureFace(Face::OuterCylinder,
        Acts::DirectedProtoAxis(AxisRPhi, Closed, -M_PI, M_PI, 120),
        Acts::DirectedProtoAxis(AxisZ, Open, -3000_mm, 3000_mm, 100));
    /*
    matNode.configureFace(Face::NegativeDisc,
        Acts::DirectedProtoAxis(AxisR, Open, 150_mm, 3000_mm, 8),
        Acts::DirectedProtoAxis(AxisPhi, Closed, -M_PI, M_PI, 8));

    matNode.configureFace(Face::PositiveDisc,
        Acts::DirectedProtoAxis(AxisR, Open, 150_mm, 3000_mm, 8),
        Acts::DirectedProtoAxis(AxisPhi, Closed, -M_PI, M_PI, 8));
    */
    matNode.addStaticVolume(Acts::Transform3::Identity(), cdcBounds, "CDC_Container",
      [&](auto& cdcVol) {
        for (size_t i = 0; i < cdcLayers.size(); ++i) {
          std::string name = "CDC_Layer" + std::to_string(i);

          cdcVol.addLayer(name, [&](auto& layer) {

            layer.setProtoLayer(cdcLayers[i].protoLayer);
            layer.setEnvelope(
                Acts::ExtentEnvelope{{.z = {5_mm, 5_mm}, .r = {2_mm, 2_mm}}});
            layer.setNavigationPolicyFactory(createNavigationPolicy2());
          });
          std::string suppName = name + "_Support";
            cdcVol.addLayer(suppName, [&](auto& suppLayer) {
              double layerRadius =
                  cdcLayers[i].protoLayer.medium(Acts::AxisDirection::AxisR) -
                  2_mm;

              auto cylBounds = std::make_shared<Acts::CylinderBounds>(
                  layerRadius, 1500_mm);
              auto cylSurf = Acts::Surface::makeShared<Acts::CylinderSurface>(
                  Acts::Transform3::Identity(), cylBounds);

              Acts::BinUtility bu(120, -M_PI, M_PI, Acts::open,
                                  Acts::AxisDirection::AxisPhi);
              bu += Acts::BinUtility(100, -1500_mm, 1500_mm, Acts::open,
                                     Acts::AxisDirection::AxisZ);
              cylSurf->assignSurfaceMaterial(
                  std::make_shared<Acts::ProtoSurfaceMaterial>(bu));

              std::vector<std::shared_ptr<Acts::Surface>> passiveSurfaces = {
                  cylSurf};
              Acts::MutableProtoLayer pl(gctx, passiveSurfaces);

              suppLayer.setProtoLayer(pl);
              suppLayer.setEnvelope(
                  Acts::ExtentEnvelope{{.z = {2_mm, 2_mm}, .r = {2_mm, 2_mm}}});

              suppLayer.setNavigationPolicyFactory(createNavigationPolicy2());
            });
        }
        cdcVol.setNavigationPolicyFactory(createNavigationPolicy2());
      });
  });
}


std::vector<ProtoLayerSurfaces> Gen3Belle2Builder::buildCDCLayers(
    const Acts::GeometryContext& gctx) {
  std::vector<ProtoLayerSurfaces> cpLayers;

  using LayerSurfaceVector = std::vector<std::shared_ptr<const Acts::Surface>>;
  LayerSurfaceVector layerSurfaces;
  const auto& layerConfigs = m_cfg.layerConfigs;

  ACTS_DEBUG(" layers : found " << layerConfigs.size() << " configuration(s)");

  // Helper function to fill the layer
  auto fillLayer =
      [&](const std::vector<std::shared_ptr<Acts::Surface>>& sVector) {
        std::size_t nb0 = 1;
        std::size_t nb1 = 1;

        Acts::MutableProtoLayer pl(gctx, sVector);

        ACTS_DEBUG("- creating CylinderLayer with "
                   << sVector.size() << " surfaces at r = "
                   << pl.medium(Acts::AxisDirection::AxisR));

        pl.envelope[Acts::AxisDirection::AxisR] = {1, 1};
        pl.envelope[Acts::AxisDirection::AxisZ] = {5, 5};
        ProtoLayerSurfaces pls{std::move(pl), sVector, nb0, nb1};
        cpLayers.push_back(std::move(pls));
      };

  std::vector<int> superLayers = {0, 1, 2, 3, 4, 5, 6, 7};

  std::map<unsigned int, std::vector<std::shared_ptr<Acts::Surface>>> surfacesByLayer;

  std::ifstream file("/work/configs/wireInfo.csv");
  if (!file.is_open()) {
    ACTS_ERROR("Konnte wireInfo.csv nicht oeffnen!");
    return cpLayers;
  }

  std::string line;
  bool isHeader = true;

  while (std::getline(file, line)) {
    if (isHeader) {
      isHeader = false;
      continue;
    }

    std::stringstream ss(line);
    std::string cell;
    std::vector<std::string> tokens;

    while (std::getline(ss, cell, ',')) {
      tokens.push_back(cell);
    }

    if (tokens.size() < 17) continue;

    unsigned int layerIndex = std::stoul(tokens[1]); 
    unsigned int wireIndex = std::stoul(tokens[2]);

    double halfLength = std::stod(tokens[4]);
    double cx = std::stod(tokens[5]);
    double cy = std::stod(tokens[6]);
    double cz = std::stod(tokens[7]);

    double r00 = std::stod(tokens[8]),  r10 = std::stod(tokens[9]),  r20 = std::stod(tokens[10]);
    double r01 = std::stod(tokens[11]), r11 = std::stod(tokens[12]), r21 = std::stod(tokens[13]);
    double r02 = std::stod(tokens[14]), r12 = std::stod(tokens[15]), r22 = std::stod(tokens[16]);

    Acts::GeometryIdentifier geoId = Acts::GeometryIdentifier()
                                         .withVolume(14)
                                         .withLayer(layerIndex + 1)
                                         .withSensitive(wireIndex + 1);

    auto bounds = std::make_shared<const Acts::LineBounds>(20.0, halfLength); //TODO: Insert actual radius of the driftcells
    Acts::Transform3 transform = Acts::Transform3::Identity();

    Acts::RotationMatrix3 rot;
    rot << r00, r01, r02, 
           r10, r11, r12, 
           r20, r21, r22;
    transform.linear() = rot;
    transform.translation() = Acts::Vector3(cx, cy, cz);

    auto wireElement = std::make_shared<CDCWireElement>(geoId, transform, bounds, 1.0);
    wireElement->surface().assignGeometryId(geoId);
    //wireElement->surface().assignSurfaceMaterial(std::make_shared<Acts::ProtoSurfaceMaterial>());

    surfacesByLayer[layerIndex].push_back(wireElement->surface().getSharedPtr());
    m_cdcElementStore.push_back(wireElement);
  }

  for (auto const& [layerIdx, surfaces] : surfacesByLayer) {
      ACTS_DEBUG("- Custom CSV Drift Chamber loaded. Found " << surfaces.size() << " wires for CDC layer " << layerIdx);
      fillLayer(surfaces);
  }

  return cpLayers;
}
void Gen3Belle2Builder::buildSVD(Acts::Experimental::BlueprintNode& parent,
                                 const Acts::GeometryContext& gctx) {
  using namespace Acts::Experimental;
  using namespace Acts::UnitLiterals;
  using enum Acts::CylinderVolumeBounds::Face;

  ACTS_DEBUG("Building SVD with Nested Volumes");

  // ProtoLayerCreator svdCreator = createPixelProtoLayerCreator();
  // Hier muss das root plugin angebunden werden
  auto svdLayers = buildLayers(gctx);

  double z_positionL6 = 55_mm;
  double z_positionL5 = 47.5_mm;
  double z_positionL4 = 24.5_mm;
  double z_positionL3 = 29.5_mm;

  std::vector<double> z_ranges = {140.0_mm, 300.0_mm, 400.0_mm, 500.0_mm};

  auto svdBounds =
      std::make_shared<Acts::CylinderVolumeBounds>(0_mm, 150_mm, 380_mm);

  Acts::Transform3 transformationZL6 = Acts::Transform3::Identity();
  transformationZL6.translation() = Acts::Vector3(0_mm, 0_mm, z_positionL6);
  Acts::Transform3 transformationZL5 = Acts::Transform3::Identity();
  transformationZL5.translation() = Acts::Vector3(0_mm, 0_mm, z_positionL5);
  Acts::Transform3 transformationZL4 = Acts::Transform3::Identity();
  transformationZL4.translation() = Acts::Vector3(0_mm, 0_mm, z_positionL4);
  Acts::Transform3 transformationZL3 = Acts::Transform3::Identity();
  transformationZL3.translation() = Acts::Vector3(0_mm, 0_mm, z_positionL3);

  std::vector<Acts::Transform3> z_transformations = {
      transformationZL3, transformationZL4, transformationZL5,
      transformationZL6};
  struct TrackingVolumeAccessor
      : public Acts::Experimental::StaticBlueprintNode {
    static Acts::TrackingVolume* getVolume(
        Acts::Experimental::StaticBlueprintNode& node) {
      return static_cast<TrackingVolumeAccessor&>(node).m_volume.get();
    }
  };

  parent.addMaterial("SVD_PassiveMaterial", [&](auto& matNode) {
    using Face = Acts::CylinderVolumeBounds::Face;
    using enum Acts::AxisDirection;
    using enum Acts::AxisBoundaryType;
    
    matNode.configureFace(Face::OuterCylinder,
        Acts::DirectedProtoAxis(AxisRPhi, Closed, -M_PI, M_PI, 120),
        Acts::DirectedProtoAxis(AxisZ, Open, -420_mm, 420_mm, 100));
    /*
    matNode.configureFace(Face::NegativeDisc,
        Acts::DirectedProtoAxis(AxisR, Open, 0_mm, 180_mm, 8),
        Acts::DirectedProtoAxis(AxisPhi, Closed, -M_PI, M_PI, 8));

    matNode.configureFace(Face::PositiveDisc,
        Acts::DirectedProtoAxis(AxisR, Open, 0_mm, 180_mm, 8),
        Acts::DirectedProtoAxis(AxisPhi, Closed, -M_PI, M_PI, 8));
    */
    matNode.addStaticVolume(
        Acts::Transform3::Identity(), svdBounds, "SVD_Container",
        [&](auto& svdVol) {
          for (int i = 0; i < 4; ++i) {
            std::string name = "SVD_Layer" + std::to_string(i + 3);

            svdVol.addLayer(name, [&](auto& layer) {
              layer.setProtoLayer(svdLayers[i].protoLayer);
              layer.setEnvelope(
                  Acts::ExtentEnvelope{{.z = {5_mm, 5_mm}, .r = {5_mm, 5_mm}}});

              layer.setNavigationPolicyFactory(createNavigationPolicy3(
                  LayerType::Cylinder,
                  {svdLayers[i].bins0, svdLayers[i].bins1}));
            });

            std::string suppName = name + "_Support";
            svdVol.addLayer(suppName, [&](auto& suppLayer) {
              double layerRadius =
                  svdLayers[i].protoLayer.medium(Acts::AxisDirection::AxisR) -
                  2_mm;

              auto cylBounds = std::make_shared<Acts::CylinderBounds>(
                  layerRadius, z_ranges[i]);
              auto cylSurf = Acts::Surface::makeShared<Acts::CylinderSurface>(
                  z_transformations[i], cylBounds);

              Acts::BinUtility bu(120, -M_PI, M_PI, Acts::open,
                                  Acts::AxisDirection::AxisPhi);
              bu += Acts::BinUtility(100, -z_ranges[i], z_ranges[i], Acts::open,
                                     Acts::AxisDirection::AxisZ);
              cylSurf->assignSurfaceMaterial(
                  std::make_shared<Acts::ProtoSurfaceMaterial>(bu));

              std::vector<std::shared_ptr<Acts::Surface>> passiveSurfaces = {
                  cylSurf};
              Acts::MutableProtoLayer pl(gctx, passiveSurfaces);

              suppLayer.setProtoLayer(pl);
              suppLayer.setEnvelope(
                  Acts::ExtentEnvelope{{.z = {2_mm, 2_mm}, .r = {2_mm, 2_mm}}});

              suppLayer.setNavigationPolicyFactory(createNavigationPolicy2());
            });
          }
          /*
          auto* volPtr = TrackingVolumeAccessor::getVolume(svdVol);
          for (auto& slantedLayer :
               {svdLayers[4], svdLayers[5], svdLayers[6]}) {
            for (auto s : slantedLayer.protoLayer.surfaces()) {
              volPtr->addSurface(s->getSharedPtr());
            }
          }
          */
          svdVol.setNavigationPolicyFactory(createNavigationPolicy2());
        });
  });
}

std::vector<ProtoLayerSurfaces> Gen3Belle2Builder::buildLayers(
    const Acts::GeometryContext& gctx) {
  std::vector<ProtoLayerSurfaces> cpLayers;

  // Bail out if you have no gGeoManager
  if (gGeoManager == nullptr) {
    ACTS_WARNING("No gGeoManager found - bailing out.");
    return {};
  }

  using LayerSurfaceVector = std::vector<std::shared_ptr<const Acts::Surface>>;
  LayerSurfaceVector layerSurfaces;
  const auto& layerConfigs = m_cfg.layerConfigs;
  // Screen output of the configuration
  ACTS_DEBUG(" layers : found " << layerConfigs.size() << " configuration(s)");

  // Helper function to fill the layer
  auto fillLayer =
      [&](const std::vector<std::shared_ptr<Acts::Surface>>& sVector,
          const LayerConfig& lCfg) {
        std::size_t nb0 = std::get<int>(lCfg.binning0.at(0));
        std::size_t nb1 = std::get<int>(lCfg.binning1.at(0));
        ;

        Acts::MutableProtoLayer pl(gctx, sVector);

        ACTS_DEBUG("- creating CylinderLayer with "
                   << sVector.size() << " surfaces at r = "
                   << pl.medium(Acts::AxisDirection::AxisR));

        pl.envelope[Acts::AxisDirection::AxisR] = {lCfg.envelope.first,
                                                   lCfg.envelope.second};
        pl.envelope[Acts::AxisDirection::AxisZ] = {lCfg.envelope.second,
                                                   lCfg.envelope.second};
        ProtoLayerSurfaces pls{std::move(pl), sVector, nb0, nb1};
        cpLayers.push_back(std::move(pls));
      };

  // Loop over Layer configs to loop over layers
  for (const auto& layerCfg : layerConfigs) {
    ACTS_DEBUG("- layer configuration found for layer " << layerCfg.volumeName
                                                        << " with sensors ");
    std::vector<std::shared_ptr<Acts::Surface>> sVector;

    for (auto& sensor : layerCfg.sensorNames) {
      ACTS_DEBUG("  - sensor: " << sensor);
    }
    if (!layerCfg.parseRanges.empty()) {
      for (const auto& [axisDir, pRange] : layerCfg.parseRanges) {
        ACTS_DEBUG("- layer parsing restricted in "
                   << axisDirectionName(axisDir) << " to [" << pRange.first
                   << "/" << pRange.second << "].");
      }
    }
    if (!layerCfg.splitConfigs.empty()) {
      for (const auto& [axisDir, tConfig] : layerCfg.splitConfigs) {
        ACTS_DEBUG("- layer splitting attempt in " << axisDirectionName(axisDir)
                                                   << " with tolerance "
                                                   << tConfig << ".");
      }
    }

    // Either pick the configured volume or take the top level volume
    // and retrieve its global transformation.
    TGeoVolume* tVolume =
        gGeoManager->FindVolumeFast(layerCfg.volumeName.c_str());
    TGeoHMatrix gmatrix = TGeoIdentity((layerCfg.volumeName + "ID").c_str());

    if (tVolume == nullptr) {
      tVolume = gGeoManager->GetTopVolume();
      ACTS_DEBUG("- search volume is TGeo top volume");
    } else {
      ACTS_DEBUG("- setting search volume to " << tVolume->GetName());

      auto node = ActsPlugins::TGeoParser::findNodeRecursive(
          gGeoManager->GetTopNode(), tVolume->GetName());

      if (node == nullptr) {
        std::string volname(tVolume->GetName());
        throw std::invalid_argument("Could not locate node for " + volname);
      }

      gmatrix = *(node->GetMatrix());
    }

    if (tVolume != nullptr) {
      ActsPlugins::TGeoParser::Options tgpOptions;
      tgpOptions.volumeNames = {layerCfg.volumeName};
      tgpOptions.targetNames = layerCfg.sensorNames;
      // tgpOptions.parseRanges = layerCfg.parseRanges;
      tgpOptions.unit = 1 * Acts::UnitConstants::cm;
      ActsPlugins::TGeoParser::State tgpState;
      tgpState.volume = tVolume;

      ACTS_DEBUG("- applying  " << layerCfg.parseRanges.size()
                                << " search restrictions.");
      for (const auto& [axisDir, pRange] : layerCfg.parseRanges) {
        ACTS_VERBOSE(" - range " << axisDirectionName(axisDir) << " within [ "
                                 << pRange.first << ", " << pRange.second
                                 << "]");
      }

      ActsPlugins::TGeoParser::select(tgpState, tgpOptions, gmatrix);

      ACTS_DEBUG("- number of selected nodes found : "
                 << tgpState.selectedNodes.size());

      for (auto& snode : tgpState.selectedNodes) {
        auto identifier =
            m_cfg.identifierProvider != nullptr
                ? m_cfg.identifierProvider->identify(gctx, *snode.node)
                : ActsPlugins::TGeoDetectorElement::Identifier();

        auto tgElementFront = m_cfg.detectorElementFactory(
            identifier, *snode.node, *snode.transform, layerCfg.localAxes,
            1 * Acts::UnitConstants::cm, layerCfg.moduleMaterial);

        m_elementStore.push_back(tgElementFront);
        sVector.push_back(tgElementFront->surface().getSharedPtr());

        if (layerCfg.isDSSD) {
          double gapCm = layerCfg.backsideGap / (1 * Acts::UnitConstants::cm);
          TGeoTranslation localShift(0, 0, gapCm);

          TGeoHMatrix backTransform(*snode.transform);
          backTransform.Multiply(&localShift);

          auto backIdentifier = identifier | 1;

          auto tgElementBack = m_cfg.detectorElementFactory(
              backIdentifier, *snode.node, backTransform,
              ActsPlugins::TGeoAxes("YxZ"), 1 * Acts::UnitConstants::cm,
              layerCfg.moduleMaterial);

          m_elementStore.push_back(tgElementBack);
          sVector.push_back(tgElementBack->surface().getSharedPtr());
        }
      }

      ACTS_DEBUG("- created TGeoDetectorElements : " << sVector.size());

      fillLayer(sVector, layerCfg);
    }
  }
  return cpLayers;
}
}  // namespace
}  // namespace B2

Belle2::Belle2(const Config& cfg)
    : Detector(Acts::getDefaultLogger("Belle2", cfg.logLevel)), m_cfg(cfg) {
  m_nominalGeometryContext =
      Acts::GeometryContext::dangerouslyDefaultConstruct();

  ACTS_INFO("Building tracking geometry");
  if (m_trackingGeometry != nullptr) {
    throw std::runtime_error("Tracking geometry already built");
  }

  B2::Belle2Builder::Config b_cfg;
  // Auch hier wurde Schabernack betrieben, diesmal mit der detectorelement
  // factory
  b_cfg.protoMaterial = m_cfg.buildProto;
  b_cfg.layerLogLevel = m_cfg.layerLogLevel;
  b_cfg.buildLevel = m_cfg.buildLevel;
  b_cfg.identifierProvider = std::make_shared<MyIdentifierProvider>();
  B2::Belle2Builder::LayerConfig cl3{
      .volumeName = "SVD.Envelope",
      .sensorNames = {"SVD.3.*.*.Active"},
      .parseRanges = {{Acts::AxisDirection::AxisR, {41.4, 45.9}},
                      {Acts::AxisDirection::AxisZ, {-94.1, 153.8}}},
      .splitConfigs = {},
      .binning0 = {std::variant<int, Acts::BinningType>(7)},
      .binning1 = {std::variant<int, Acts::BinningType>(2)},
      .envelope = {30, 30},
      .localAxes = ActsPlugins::TGeoAxes("XYZ")};
  B2::Belle2Builder::LayerConfig cl4{
      .volumeName = "SVD.Envelope",
      .sensorNames = {"SVD.4.*.2.Active", "SVD.4.*.3.Active"},
      .parseRanges = {{Acts::AxisDirection::AxisR, {56.9, 89.4}},
                      {Acts::AxisDirection::AxisZ, {-161.7, 210.5}}},
      .splitConfigs = {},
      .binning0 = {std::variant<int, Acts::BinningType>(10)},
      .binning1 = {std::variant<int, Acts::BinningType>(2)},
      .envelope = {5, 5},
      .localAxes = ActsPlugins::TGeoAxes("XYZ")};
  B2::Belle2Builder::LayerConfig cl5{
      .volumeName = "SVD.Envelope",
      .sensorNames = {"SVD.5.*.2.Active", "SVD.5.*.3.Active",
                      "SVD.5.*.4.Active"},
      .parseRanges = {{Acts::AxisDirection::AxisR, {82.0, 111.6}},
                      {Acts::AxisDirection::AxisZ, {-200.9, 185.2}}},
      .splitConfigs = {},
      .binning0 = {std::variant<int, Acts::BinningType>(12)},
      .binning1 = {std::variant<int, Acts::BinningType>(3)},
      .envelope = {5, 5},
      .localAxes = ActsPlugins::TGeoAxes("XYZ")};
  B2::Belle2Builder::LayerConfig cl6{
      .volumeName = "SVD.Envelope",
      .sensorNames = {"SVD.6.*.2.Active", "SVD.6.*.3.Active",
                      "SVD.6.*.4.Active", "SVD.6.*.5.Active"},
      .parseRanges = {{Acts::AxisDirection::AxisR, {108, 143.6}},
                      {Acts::AxisDirection::AxisZ, {-254.4, 255.0}}},
      .splitConfigs = {},
      .binning0 = {std::variant<int, Acts::BinningType>(16)},
      .binning1 = {std::variant<int, Acts::BinningType>(4)},
      .envelope = {5, 5},
      .localAxes = ActsPlugins::TGeoAxes("XYZ")};
  B2::Belle2Builder::LayerConfig cl4s{
      .volumeName = "SVD.Envelope",
      .sensorNames = {"SVD.4.*.1.Active"},
      .parseRanges = {{Acts::AxisDirection::AxisR, {72.0, 110.0}},
                      {Acts::AxisDirection::AxisZ, {180.0, 300.0}}},
      .splitConfigs = {},
      .binning0 = {std::variant<int, Acts::BinningType>(10)},
      .binning1 = {std::variant<int, Acts::BinningType>(1)},
      .envelope = {5, 5},
      .localAxes = ActsPlugins::TGeoAxes("XYZ")};
  B2::Belle2Builder::LayerConfig cl5s{
      .volumeName = "SVD.Envelope",
      .sensorNames = {"SVD.5.*.1.Active"},
      .parseRanges = {{Acts::AxisDirection::AxisR, {72.0, 110.0}},
                      {Acts::AxisDirection::AxisZ, {180.0, 300.0}}},
      .splitConfigs = {},
      .binning0 = {std::variant<int, Acts::BinningType>(12)},
      .binning1 = {std::variant<int, Acts::BinningType>(1)},
      .envelope = {5, 5},
      .localAxes = ActsPlugins::TGeoAxes("XYZ")};
  B2::Belle2Builder::LayerConfig cl6s{
      .volumeName = "SVD.Envelope",
      .sensorNames = {"SVD.6.*.1.Active"},
      .parseRanges = {{Acts::AxisDirection::AxisR, {90.0, 143.0}},
                      {Acts::AxisDirection::AxisZ, {240.0, 360.0}}},
      .splitConfigs = {},
      .binning0 = {std::variant<int, Acts::BinningType>(16)},
      .binning1 = {std::variant<int, Acts::BinningType>(1)},
      .envelope = {5, 5},
      .localAxes = ActsPlugins::TGeoAxes("XYZ")};
  Acts::MaterialSlab svdSlab(B2::Belle2Builder::kSiliconMaterial,
                             0.32f); 
  auto svdModuleMaterial =
      m_cfg.buildProto
          ? std::static_pointer_cast<const Acts::ISurfaceMaterial>(
                std::make_shared<Acts::ProtoSurfaceMaterial>())
          : std::static_pointer_cast<const Acts::ISurfaceMaterial>(
                std::make_shared<Acts::HomogeneousSurfaceMaterial>(svdSlab));

  cl3.moduleMaterial = svdModuleMaterial;
  cl4.moduleMaterial = svdModuleMaterial;
  cl5.moduleMaterial = svdModuleMaterial;
  cl6.moduleMaterial = svdModuleMaterial;
  cl4s.moduleMaterial = svdModuleMaterial;
  cl5s.moduleMaterial = svdModuleMaterial;
  cl6s.moduleMaterial = svdModuleMaterial;
  cl3.isDSSD = true;
  cl3.backsideGap = 0.32 * Acts::UnitConstants::mm;
  cl3.backsideStereo = M_PI / 2.0;  

  cl4.isDSSD = true;
  cl4.backsideGap = 0.32 * Acts::UnitConstants::mm;
  cl4.backsideStereo = M_PI / 2.0;
  cl5.isDSSD = true;
  cl5.backsideGap = 0.32 * Acts::UnitConstants::mm;
  cl5.backsideStereo = M_PI / 2.0;
  cl6.isDSSD = true;
  cl6.backsideGap = 0.32 * Acts::UnitConstants::mm;
  cl6.backsideStereo = M_PI / 2.0;
  cl6s.isDSSD = true;
  cl6s.backsideGap = 0.32 * Acts::UnitConstants::mm;
  cl6s.backsideStereo = M_PI / 2.0;
  cl4s.isDSSD = true;
  cl4s.backsideGap = 0.32 * Acts::UnitConstants::mm;
  cl4s.backsideStereo = M_PI / 2.0;
  cl5s.isDSSD = true;
  cl5s.backsideGap = 0.32 * Acts::UnitConstants::mm;
  cl5s.backsideStereo = M_PI / 2.0;
  b_cfg.layerConfigs.push_back(cl3);
  b_cfg.layerConfigs.push_back(cl4);
  b_cfg.layerConfigs.push_back(cl5);
  b_cfg.layerConfigs.push_back(cl6);
  b_cfg.layerConfigs.push_back(cl4s);
  b_cfg.layerConfigs.push_back(cl5s);
  b_cfg.layerConfigs.push_back(cl6s);
  B2::Gen3Belle2Builder builder(b_cfg, logger().clone());

  m_trackingGeometry = builder.buildTrackingGeometry(m_nominalGeometryContext,
                                                     m_cfg.graphvizFile);

  if (m_trackingGeometry == nullptr) {
    throw std::runtime_error("Error building tracking geometry");
  }
  if (m_cfg.materialDecorator) {
    ACTS_INFO("Wende MaterialDecorator auf die gebaute Geometrie an...");
    m_trackingGeometry->visitSurfaces([&](const Acts::Surface* surface) {
      if (surface) {
        m_cfg.materialDecorator->decorate(const_cast<Acts::Surface&>(*surface));
      }
    });
    m_trackingGeometry->visitVolumes([&](const Acts::TrackingVolume* volume) {
      if (volume) {
        m_cfg.materialDecorator->decorate(
            const_cast<Acts::TrackingVolume&>(*volume));

        for (const auto& boundary : volume->boundarySurfaces()) {
          if (boundary) {
            const Acts::Surface& portalSurface =
                boundary->surfaceRepresentation();
            m_cfg.materialDecorator->decorate(
                const_cast<Acts::Surface&>(portalSurface));
          }
        }
      }
    });
  }

  auto elements = builder.getElementStore();
  m_detectorStore.insert(m_detectorStore.end(), elements.begin(),
                         elements.end());
  auto cdcElements = builder.getCDCElementStore();
  m_detectorStore.insert(m_detectorStore.end(), cdcElements.begin(),
                         cdcElements.end());

  ACTS_INFO("Tracking geometry built");
}

}  // namespace ActsExamples
