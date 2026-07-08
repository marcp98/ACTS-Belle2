#!/usr/bin/env python3

from pathlib import Path
from typing import Optional

import acts
from acts.examples import GenericDetector

u = acts.UnitConstants


def runTruthTrackingKalman(
    trackingGeometry: acts.TrackingGeometry,
    field: acts.MagneticFieldProvider,
    seeding_config: Path,
    outputDir: Path,
    inputDir: Optional[Path] = None,
    decorators=[],
    reverseFilteringMomThreshold=0 * u.GeV,
    reverseFilteringCovarianceScaling=1,
    s: acts.examples.Sequencer = None,
):
    from acts.examples.simulation import (
        addParticleGun,
        ParticleConfig,
        EtaConfig,
        PhiConfig,
        MomentumConfig,
        addFatras,
        addDigitization,
        ParticleSelectorConfig,
        addDigiParticleSelection,
    )
    from acts.examples.reconstruction import (
        addSeeding,
        SeedingAlgorithm,
        addKalmanTracks,
        addSpacePointsMaking,
        addMyFitter
    )

    s = s or acts.examples.Sequencer(
        events=1000, numThreads=1, logLevel=acts.logging.INFO
    )

    for d in decorators:
        s.addContextDecorator(d)

    outputDir = Path(outputDir)

    logger = acts.getDefaultLogger("full_chain_test", acts.logging.Level(acts.logging.FATAL))

    addParticleGun(
        s,
        MomentumConfig(
           0 * u.GeV,
            1 * u.GeV,
            transverse=True,
        ),
        EtaConfig(-4, 4),
        PhiConfig(0.0, 360.0 * u.degree),
        ParticleConfig(
            4, acts.PdgParticle.eMuon, randomizeCharge=True
        ),
        vtxGen=acts.examples.GaussianVertexGenerator(
            mean=acts.Vector4(0, 0, 0, 0),
            stddev=acts.Vector4(
                0.0125 * u.mm, 0.0125 * u.mm, 55.5 * u.mm, 1.0 * u.ns
            ),
        ),
        multiplicity=10,
        rnd=acts.examples.RandomNumbers(seed=42),
    )


   
    logger.info("Reading prototracks from %s", inputDir)
    

    
   
    return s


if "__main__" == __name__:
    srcdir = Path(__file__).resolve().parent.parent.parent.parent

    from acts.examples import Belle2
    detector = Belle2()
    trackingGeometry = detector.trackingGeometry()
    decorators = detector.contextDecorators()

    field = acts.ConstantBField(acts.Vector3(0, 0, 1.5 * u.T))
    input_dir = "cluster_tests/"
    runTruthTrackingKalman(
        inputDir = input_dir,
        seeding_config= "configs/seeding_config.json",
        trackingGeometry=trackingGeometry,
        field=field,
        outputDir=input_dir,
    ).run()
