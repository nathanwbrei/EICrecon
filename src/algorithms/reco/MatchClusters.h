// Original header license: SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2022 Sylvester Joosten, Dmitry Romanov

// Takes a list of particles (presumed to be from tracking), and all available clusters.
// 1. Match clusters to their tracks using the mcID field
// 2. For unmatched clusters create neutrals and add to the particle list

#pragma once

#include <edm4eic/ClusterCollection.h>
#include <edm4eic/MCRecoClusterParticleAssociationCollection.h>
#include <edm4eic/MCRecoParticleAssociationCollection.h>
#include <edm4eic/ReconstructedParticleCollection.h>
#include <edm4hep/MCParticleCollection.h>
#include <spdlog/logger.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <tuple>
#include <vector>


namespace eicrecon {

    class MatchClusters {
    private:

    public:

        using MatchingResults = std::tuple<edm4eic::ReconstructedParticleCollection*, edm4eic::MCRecoParticleAssociationCollection*>;

        void init(std::shared_ptr<spdlog::logger> logger);

        MatchingResults execute(
            const edm4hep::MCParticleCollection* mcparticles,
            const edm4eic::ReconstructedParticleCollection* inparts,
            const edm4eic::MCRecoParticleAssociationCollection* inpartsassoc,
            const std::vector<const edm4eic::ClusterCollection*> &cluster_collections,
            const std::vector<const edm4eic::MCRecoClusterParticleAssociationCollection*> &cluster_assoc_collections);

    private:

        std::shared_ptr<spdlog::logger> m_log;

        // get a map of mcID --> cluster
        // input: cluster_collections --> list of handles to all cluster collections
        std::map<int, edm4eic::Cluster> indexedClusters(
                const std::vector<const edm4eic::ClusterCollection*> &cluster_collections,
                const std::vector<const edm4eic::MCRecoClusterParticleAssociationCollection*> &associations_collections);

        // reconstruct a neutral cluster
        // (for now assuming the vertex is at (0,0,0))
        edm4eic::ReconstructedParticle
        reconstruct_neutral(const edm4eic::Cluster *cluster, const double mass, const int32_t pdg) const;
    };

} // namespace eicrecon
