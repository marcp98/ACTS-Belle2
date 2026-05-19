// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "./Belle2Builder.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "ActsExamples/Belle2/ProtoLayerCreator.hpp"
#include "ActsPlugins/Root/TGeoLayerBuilder.hpp" 
#include "ActsPlugins/Root/TGeoDetectorElement.hpp"
#include <memory>

using namespace Acts::UnitLiterals;

namespace ActsExamples::B2 {

namespace {

/// Helper method for positioning
/// @param radius is the cylinder radius
/// @param zStagger is the radial staggering along z
/// @param moduleHalfLength is the module length (longitudinal)
/// @param lOverlap is the overlap of the modules (longitudinal)
/// @binningSchema is the way the bins are laid out rphi x z
std::vector<Acts::Vector3> modulePositionsCylinder(
    double radius, double zStagger, double moduleHalfLength, double lOverlap,
    const std::pair<int, int>& binningSchema, std::size_t layer) {
  int nPhiBins = binningSchema.first;
  int nZbins = binningSchema.second;
  // prepare the return value
  std::vector<Acts::Vector3> mPositions;
  mPositions.reserve(nPhiBins * nZbins);
  // prep work
  double phiStep = 2 * std::numbers::pi / nPhiBins;
  double minPhi = -std::numbers::pi + 0.5 * phiStep;
  double zStart = -0.5 * (nZbins - 1) * (2 * moduleHalfLength - lOverlap);
  double zStep = 2 * std::abs(zStart) / (nZbins - 1);
  // loop over the bins
  for (std::size_t zBin = 0; zBin < static_cast<std::size_t>(nZbins); ++zBin) {
    
    if ((layer == 1 && zBin == 2) || (layer == 2 && zBin == 3)) {
        // prepare z and r
        double moduleZ = zStart + static_cast<double>(zBin) * zStep - moduleHalfLength + moduleHalfLength*std::cos(0.207);
        std::cout<<"moduleZ: " << moduleZ << std::endl;
        double moduleR = radius - moduleHalfLength*std::sin(0.207);
        
        for (std::size_t phiBin = 0; phiBin < static_cast<std::size_t>(nPhiBins); ++phiBin) {
            // calculate the current phi value
            double modulePhi = minPhi + phiBin * phiStep;
            mPositions.push_back(Acts::Vector3(moduleR * std::cos(modulePhi),
                                                moduleR * std::sin(modulePhi),
                                                moduleZ));
        }
    }else{
        // prepare z and r
        double moduleZ = zStart + static_cast<double>(zBin) * zStep;
        double moduleR = radius;
        
        for (std::size_t phiBin = 0; phiBin < static_cast<std::size_t>(nPhiBins); ++phiBin) {
            // calculate the current phi value
            double modulePhi = minPhi + phiBin * phiStep;
            mPositions.push_back(Acts::Vector3(moduleR * std::cos(modulePhi),
                                                moduleR * std::sin(modulePhi),
                                                moduleZ));
        }
    }
}
  return mPositions;
}

/// Helper method for positioning
/// @param z is the z position of the ring
/// @param radius is the ring radius
/// @param phiStagger is the radial staggering along phi
/// @param lOverlap is the overlap of the modules
/// @param nPhiBins is the number of bins in phi
std::vector<Acts::Vector3> modulePositionsRing(double z, double radius,
                                               double phiStagger,
                                               double phiSubStagger,
                                               int nPhiBins) {
  // create and fill the positions
  std::vector<Acts::Vector3> rPositions;
  rPositions.reserve(nPhiBins);
  // prep work
  double phiStep = 2 * std::numbers::pi / nPhiBins;
  double minPhi = -std::numbers::pi + 0.5 * phiStep;
  // phi loop
  for (std::size_t iphi = 0; iphi < static_cast<std::size_t>(nPhiBins);
       ++iphi) {
    // if we have a phi sub stagger presents
    double rzs = 0.;
    // phi stagger affects 0 vs 1, 2 vs 3 ... etc
    // -> only works if it is a %4
    // phi sub stagger affects 2 vs 4, 1 vs 3 etc.
    if (phiSubStagger != 0. && ((nPhiBins % 4) == 0)) {
      // switch sides
      if ((iphi % 4) == 0u) {
        rzs = phiSubStagger;
      } else if (((iphi + 1) % 4) == 0u) {
        rzs = -phiSubStagger;
      }
    }
    // the module phi
    double phi = minPhi + static_cast<double>(iphi) * phiStep;
    // main z position depending on phi bin
    double rz = (iphi % 2) != 0u ? z - 0.5 * phiStagger : z + 0.5 * phiStagger;
    rPositions.push_back(Acts::Vector3(radius * std::cos(phi),
                                       radius * std::sin(phi), rz + rzs));
  }
  return rPositions;
}

/// Helper method for positioning
/// @param z is the nominal z posiiton of the dis
/// @param ringStagger is the staggering of the different rings
/// @param phiStagger is the staggering on a ring in phi : it is even/odd
/// @param phiSubStagger is the sub staggering on a ring in phi : it affects
/// 0/4/8 and 3/6
/// @param innerRadius is the inner Radius for the disc
/// @param outerRadius is the outer Radius for the disc
/// @param discBinning is the binning setup in r, phi
/// @param moduleHalfLength is pair of phibins and module length
std::vector<std::vector<Acts::Vector3>> modulePositionsDisc(
    double z, double ringStagger, std::vector<double> phiStagger,
    std::vector<double> phiSubStagger, double innerRadius, double outerRadius,
    const std::vector<std::size_t>& discBinning,
    const std::vector<double>& moduleHalfLength) {
  // calculate the radii
  std::vector<double> radii;
  // the radial span of the disc
  double deltaR = outerRadius - innerRadius;
  // quick exits
  if (discBinning.size() == 1) {
    radii.push_back(0.5 * (innerRadius + outerRadius));
  } else {
    double totalLength = 0;
    // sum up the total length
    for (auto& mhlength : moduleHalfLength) {
      totalLength += 2 * mhlength;
    }
    // now calculate the overlap (equal pay)
    double rOverlap = (totalLength - deltaR) /
                      static_cast<double>(moduleHalfLength.size() - 1);
    // and now fill the radii and gaps
    double lastR = innerRadius;
    double lastHl = 0.;
    double lastOl = 0.;
    // now calculate
    for (auto& mhlength : moduleHalfLength) {
      // calculate the radius
      radii.push_back(lastR + lastHl - lastOl + mhlength);
      lastR = radii[radii.size() - 1];
      lastOl = rOverlap;
      lastHl = mhlength;
    }
  }
  // now prepare the return method
  std::vector<std::vector<Acts::Vector3>> mPositions;
  for (std::size_t ir = 0; ir < radii.size(); ++ir) {
    // generate the z value
    // convention inner ring is closer to origin : makes sense
    double stagger =
        (ir % 2) != 0u ? z + 0.5 * ringStagger : z - 0.5 * ringStagger;
    double rz = radii.size() == 1 ? z : stagger;
    // fill the ring positions
    double psStagger = phiSubStagger.empty() ? 0. : phiSubStagger[ir];
    mPositions.push_back(modulePositionsRing(rz, radii[ir], phiStagger[ir],
                                             psStagger, discBinning[ir]));
  }
  return mPositions;
}

}  // namespace

Belle2Builder::Belle2Builder(const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
                                              : m_cfg(cfg), m_logger(std::move(logger)) {
    // Prepare the proto material - in case it's designed to do so
    // - cylindrical
    Acts::BinUtility pCylinderUtility(10, -1, 1, Acts::closed,Acts::AxisDirection::AxisPhi);
    pCylinderUtility += Acts::BinUtility(10, -1, 1, Acts::open, Acts::AxisDirection::AxisZ);
    auto pCylinderMaterial = std::make_shared<const Acts::ProtoSurfaceMaterial>(pCylinderUtility);
    // - disc
    Acts::BinUtility pDiscUtility(10, 0, 1, Acts::open, Acts::AxisDirection::AxisR);
    pDiscUtility +=Acts::BinUtility(10, -1, 1, Acts::closed, Acts::AxisDirection::AxisPhi);
    auto pDiscMaterial = std::make_shared<const Acts::ProtoSurfaceMaterial>(pDiscUtility);
    // - plane
    Acts::BinUtility pPlaneUtility(1, -1, 1, Acts::open, Acts::AxisDirection::AxisX);
    auto pPlaneMaterial = std::make_shared<const Acts::ProtoSurfaceMaterial>(pPlaneUtility);

    ///
    /// BeamPipe material
    ///
    const auto beryllium = Acts::Material::fromMassDensity(
        static_cast<float>(352.8_mm), 
        static_cast<float>(407_mm), 
        9.012f, 
        4.0f, 
        static_cast<float>(1.848_g / 1_cm3)
    );
    m_beamPipeMaterial = std::make_shared<const Acts::HomogeneousSurfaceMaterial>(
        Acts::MaterialSlab(beryllium, static_cast<float>(1.0_mm))
    );

    ///
    /// PIXEL MATERIAL
    ///

    Acts::MaterialSlab pcModuleMaterial(kSiliconMaterial, kPixelCentralModuleT);
    Acts::MaterialSlab peModuleMaterial(kSiliconMaterial, kPixelEndcapModuleT);
    // Layer material properties - thickness, X0, L0, A, Z, Rho
    Acts::MaterialSlab pcmbProperties(kSiliconMaterial, static_cast<float>(1.5_mm));
    Acts::MaterialSlab pcmecProperties(kSiliconMaterial, static_cast<float>(1.5_mm));

    // Module, central and disc material
    m_pixelCentralMaterial = std::make_shared<const Acts::HomogeneousSurfaceMaterial>(pcmbProperties);
    m_pixelEndcapMaterial = std::make_shared<const Acts::HomogeneousSurfaceMaterial>(pcmecProperties);
    m_pixelCentralModuleMaterial = std::make_shared<const Acts::HomogeneousSurfaceMaterial>(pcModuleMaterial);
    m_pixelEndcapModuleMaterial = std::make_shared<const Acts::HomogeneousSurfaceMaterial>(peModuleMaterial);
    if (m_cfg.protoMaterial) {
        m_pixelCentralMaterial = pCylinderMaterial;
        m_pixelCentralModuleMaterial = pPlaneMaterial;
        m_pixelEndcapMaterial = pDiscMaterial;
        m_pixelEndcapModuleMaterial = pPlaneMaterial;
    }

    ///
    /// PST MATERIAL
    ///

    m_pstMaterial = std::make_shared<const Acts::HomogeneousSurfaceMaterial>(Acts::MaterialSlab(beryllium, static_cast<float>(1.8_mm)));
    if (m_cfg.protoMaterial) {
      m_pstMaterial = pCylinderMaterial;
    }
}

const Acts::Material Belle2Builder::kSiliconMaterial =
      Acts::Material::fromMassDensity(static_cast<float>(95.7_mm),
                                      static_cast<float>(465.2_mm), 28.03f, 14.f,
                                      static_cast<float>(2.32e-3_g / 1_cm3));

ProtoLayerCreator Belle2Builder::createPixelProtoLayerCreator() {
    // envelope for layers
    std::pair<double, double> pcEnvelope(2., 2.);

    // configure the pixel proto layer builder
    ProtoLayerCreator::Config pplConfig;
    pplConfig.detectorElementFactory = m_cfg.detectorElementFactory;

    // standard, an approach envelope
    pplConfig.approachSurfaceEnvelope = 1.;
    // BARREL :
    // 4 pixel layers
    // configure the central barrel
    pplConfig.centralLayerBinMultipliers = {1, 1};
    pplConfig.centralLayerRadii = {39., 80., 105.};
    pplConfig.centralLayerEnvelopes = {pcEnvelope, pcEnvelope, pcEnvelope};
    pplConfig.centralModuleBinningSchema = {
        {7, 2}, {10, 3}, {12, 4}};
    pplConfig.centralModuleTiltPhi = {0.111, 0.157, 0.111};
    pplConfig.centralModuleHalfX = {19.275, 28.86, 28.86};
    pplConfig.centralModuleHalfY = {61.45, 61.45, 61.45};
    pplConfig.centralModuleThickness = {
        0.32f, 0.32f, 0.32f};
    pplConfig.centralModuleMaterial = {
        m_pixelCentralModuleMaterial, m_pixelCentralModuleMaterial,
        m_pixelCentralModuleMaterial};
    // no frontside/backside
    pplConfig.centralModuleFrontsideStereo = {};
    pplConfig.centralModuleBacksideStereo = {};
    pplConfig.centralModuleBacksideGap = {};
    // mPositions
    std::vector<std::vector<Acts::Vector3>> pplCentralModulePositions;
    for (std::size_t plb = 0; plb < pplConfig.centralLayerRadii.size(); ++plb) {
      // call the helper function
      pplCentralModulePositions.push_back(
          modulePositionsCylinder(pplConfig.centralLayerRadii[plb],
                                  0.0,  // 1 mm stagger
                                  pplConfig.centralModuleHalfY[plb],
                                  0.,  // 4 mm module overlap in z
                                  pplConfig.centralModuleBinningSchema[plb], plb));
    }
    pplConfig.centralModulePositions = pplCentralModulePositions;
    

    /// The ProtoLayer creator
    ProtoLayerCreator pplCreator(pplConfig,
                                logger().clone("PPLCrtr", m_cfg.layerLogLevel));

    return pplCreator;
  }
std::shared_ptr<ActsPlugins::TGeoDetectorElement> Belle2Builder::defaultElementFactory(
    const ActsPlugins::TGeoDetectorElement::Identifier& identifier,
    const TGeoNode& tGeoNode, const TGeoMatrix& tGeoMatrix,
    ActsPlugins::TGeoAxes axes, double scalor,
    std::shared_ptr<const Acts::ISurfaceMaterial> material) {
    
    // Wir leiten den Aufruf einfach an die Standard-Funktion von ACTS weiter
    return ActsPlugins::TGeoLayerBuilder::defaultElementFactory(
        identifier, tGeoNode, tGeoMatrix, axes, scalor, std::move(material));
}

}  // namespace ActsExamples::Generic
