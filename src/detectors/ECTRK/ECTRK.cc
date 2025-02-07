// Copyright 2022, Dmitry Romanov
// Subject to the terms in the LICENSE file found in the top-level directory.
//
//

#include <Evaluator/DD4hepUnits.h>
#include <JANA/JApplication.h>
#include <string>

#include "algorithms/interfaces/WithPodConfig.h"
#include "extensions/jana/JChainMultifactoryGeneratorT.h"
#include "factories/digi/SiliconTrackerDigi_factoryT.h"
#include "factories/tracking/TrackerHitReconstruction_factoryT.h"

extern "C" {
void InitPlugin(JApplication *app) {
    InitJANAPlugin(app);

    using namespace eicrecon;

    // Digitization
    app->Add(new JChainMultifactoryGeneratorT<SiliconTrackerDigi_factoryT>(
        "EndcapTrackerDigiHit",
        {"TrackerEndcapHits"},
        {"EndcapTrackerDigiHit"},
        {
            .threshold = 0.54 * dd4hep::keV,
        },
        app
    ));

    // Convert raw digitized hits into hits with geometry info (ready for tracking)
    app->Add(new JChainMultifactoryGeneratorT<TrackerHitReconstruction_factoryT>(
        "SiEndcapTrackerRecHits",
        {"EndcapTrackerDigiHit"},
        {"SiEndcapTrackerRecHits"},
        {}, // default config
        app
    ));

}
} // extern "C"
