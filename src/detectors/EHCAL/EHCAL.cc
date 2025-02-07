// Copyright 2023, Friederike Bock
// Subject to the terms in the LICENSE file found in the top-level directory.
//
//

#include <Evaluator/DD4hepUnits.h>
#include <JANA/JApplication.h>
#include <string>

#include "algorithms/calorimetry/CalorimeterHitDigiConfig.h"
#include "extensions/jana/JChainMultifactoryGeneratorT.h"
#include "factories/calorimetry/CalorimeterClusterRecoCoG_factoryT.h"
#include "factories/calorimetry/CalorimeterHitDigi_factoryT.h"
#include "factories/calorimetry/CalorimeterHitReco_factoryT.h"
#include "factories/calorimetry/CalorimeterHitsMerger_factoryT.h"
#include "factories/calorimetry/CalorimeterIslandCluster_factoryT.h"
#include "factories/calorimetry/CalorimeterTruthClustering_factoryT.h"

extern "C" {
    void InitPlugin(JApplication *app) {

        using namespace eicrecon;

        InitJANAPlugin(app);
        // Make sure digi and reco use the same value
        decltype(CalorimeterHitDigiConfig::capADC)        HcalEndcapN_capADC = 32768; // assuming 15 bit ADC like FHCal
        decltype(CalorimeterHitDigiConfig::dyRangeADC)    HcalEndcapN_dyRangeADC = 100 * dd4hep::MeV; // to be verified with simulations
        decltype(CalorimeterHitDigiConfig::pedMeanADC)    HcalEndcapN_pedMeanADC = 10;
        decltype(CalorimeterHitDigiConfig::pedSigmaADC)   HcalEndcapN_pedSigmaADC = 2;
        decltype(CalorimeterHitDigiConfig::resolutionTDC) HcalEndcapN_resolutionTDC = 10 * dd4hep::picosecond;

        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitDigi_factoryT>(
          "HcalEndcapNRawHits", {"HcalEndcapNHits"}, {"HcalEndcapNRawHits"},
          {
            .tRes = 0.0 * dd4hep::ns,
            .capADC = HcalEndcapN_capADC,
            .dyRangeADC = HcalEndcapN_dyRangeADC,
            .pedMeanADC = HcalEndcapN_pedMeanADC,
            .pedSigmaADC = HcalEndcapN_pedSigmaADC,
            .resolutionTDC = HcalEndcapN_resolutionTDC,
            .corrMeanScale = 1.0,
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitReco_factoryT>(
          "HcalEndcapNRecHits", {"HcalEndcapNRawHits"}, {"HcalEndcapNRecHits"},
          {
            .capADC = HcalEndcapN_capADC,
            .dyRangeADC = HcalEndcapN_dyRangeADC,
            .pedMeanADC = HcalEndcapN_pedMeanADC,
            .pedSigmaADC = HcalEndcapN_pedSigmaADC,
            .resolutionTDC = HcalEndcapN_resolutionTDC,
            .thresholdFactor = 0.0,
            .thresholdValue = 0.0,
            .sampFrac = 0.0095, // from latest study - implement at level of reco hits rather than clusters
            .readout = "HcalEndcapNHits",
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterHitsMerger_factoryT>(
          "HcalEndcapNMergedHits", {"HcalEndcapNRecHits"}, {"HcalEndcapNMergedHits"},
          {
            .readout = "HcalEndcapNHits",
            .fields = {"layer", "slice"},
            .refs = {1, 0},
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterTruthClustering_factoryT>(
          "HcalEndcapNTruthProtoClusters", {"HcalEndcapNRecHits", "HcalEndcapNHits"}, {"HcalEndcapNTruthProtoClusters"},
          app   // TODO: Remove me once fixed
        ));
        app->Add(new JChainMultifactoryGeneratorT<CalorimeterIslandCluster_factoryT>(
          "HcalEndcapNIslandProtoClusters", {"HcalEndcapNRecHits"}, {"HcalEndcapNIslandProtoClusters"},
          {
            .sectorDist = 5.0 * dd4hep::cm,
            .localDistXY = {15*dd4hep::cm, 15*dd4hep::cm},
            .splitCluster = true,
            .minClusterHitEdep = 0.0 * dd4hep::MeV,
            .minClusterCenterEdep = 30.0 * dd4hep::MeV,
            .transverseEnergyProfileMetric = "globalDistEtaPhi",
            .transverseEnergyProfileScale = 1.,
          },
          app   // TODO: Remove me once fixed
        ));
        app->Add(
          new JChainMultifactoryGeneratorT<CalorimeterClusterRecoCoG_factoryT>(
             "HcalEndcapNTruthClusters",
            {"HcalEndcapNTruthProtoClusters",        // edm4eic::ProtoClusterCollection
             "HcalEndcapNHits"},                     // edm4hep::SimCalorimeterHitCollection
            {"HcalEndcapNTruthClusters",             // edm4eic::Cluster
             "HcalEndcapNTruthClusterAssociations"}, // edm4eic::MCRecoClusterParticleAssociation
            {
              .energyWeight = "log",
              .sampFrac = 1.0,
              .logWeightBase = 6.2,
              .enableEtaBounds = false
            },
            app   // TODO: Remove me once fixed
          )
        );

        app->Add(
          new JChainMultifactoryGeneratorT<CalorimeterClusterRecoCoG_factoryT>(
             "HcalEndcapNClusters",
            {"HcalEndcapNIslandProtoClusters",  // edm4eic::ProtoClusterCollection
             "HcalEndcapNHits"},                // edm4hep::SimCalorimeterHitCollection
            {"HcalEndcapNClusters",             // edm4eic::Cluster
             "HcalEndcapNClusterAssociations"}, // edm4eic::MCRecoClusterParticleAssociation
            {
              .energyWeight = "log",
              .sampFrac = 1.0,
              .logWeightBase = 6.2,
              .enableEtaBounds = false,
            },
            app   // TODO: Remove me once fixed
          )
        );
    }
}
