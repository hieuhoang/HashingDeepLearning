#pragma once
#include "Layer.h"
#include <unordered_map>
#include <vector>

namespace hieu {
class Network {
protected:
  std::vector<Layer *> _layers;

  const Layer &getLayer(size_t idx) const { return *_layers[idx]; }
  Layer &getLayer(size_t idx) { return *_layers[idx]; }

  size_t computeActivation(const std::unordered_map<int, float> &data1,
                           const std::vector<int> &labels) const;

public:
  Network();
  virtual ~Network();

  size_t predictClass(const std::vector<std::unordered_map<int, float>> &data,
                      const Vec2d<int> &labels) const;
};

} // namespace hieu
