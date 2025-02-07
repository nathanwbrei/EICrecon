// Created by Joe Osborn
// Subject to the terms in the LICENSE file found in the top-level directory.
//

#pragma once

#include <JANA/JEvent.h>
#include <JANA/JException.h>
#include <edm4eic/VertexCollection.h>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <typeindex>
#include <utility>
#include <vector>

#include "IterativeVertexFinderConfig.h"
#include "algorithms/tracking/IterativeVertexFinder.h"
#include "extensions/jana/JChainFactoryT.h"
#include "extensions/spdlog/SpdlogMixin.h"

namespace eicrecon {

class IterativeVertexFinder_factory
    : public JChainFactoryT<edm4eic::Vertex, IterativeVertexFinderConfig>,
      public SpdlogMixin {

public:
  explicit IterativeVertexFinder_factory(std::vector<std::string> default_input_tags,
                                         IterativeVertexFinderConfig cfg)
      : JChainFactoryT<edm4eic::Vertex, IterativeVertexFinderConfig>(std::move(default_input_tags),
                                                                     cfg) {}

  /** One time initialization **/
  void Init() override;

  /** On run change preparations **/
  void ChangeRun(const std::shared_ptr<const JEvent>& event) override;

  /** Event by event processing **/
  void Process(const std::shared_ptr<const JEvent>& event) override;

private:
  eicrecon::IterativeVertexFinder m_vertexing_algo; /// Proxy vertexing algorithm
};

} // namespace eicrecon
