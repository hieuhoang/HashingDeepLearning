#include "Layer.h"
#include "../Util.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <unordered_set>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

namespace hieu {
Layer::Layer(size_t layerIdx, size_t numNodes, size_t prevNumNodes,
             size_t maxBatchsize, bool sparsify, size_t K, size_t L,
             size_t RangePow)
    : _layerIdx(layerIdx), _numNodes(numNodes), _prevNumNodes(prevNumNodes) {

  _weights.resize(numNodes * prevNumNodes);
  _bias.resize(numNodes);

  if (sparsify) {
    _hashTables = new LSH(K, L, RangePow);
    _dwtaHasher = new DensifiedWtaHash(K * L, prevNumNodes);
  }

  _nodes.reserve(numNodes);
  for (size_t nodeIdx = 0; nodeIdx < numNodes; ++nodeIdx) {
    SubVectorConst<float> nodeWeights(_weights, nodeIdx * prevNumNodes, prevNumNodes);
    float &nodeBias = _bias.at(nodeIdx);

    _nodes.emplace_back(Node(nodeIdx, nodeWeights, nodeBias, maxBatchsize));
  }

  cerr << "Created Layer"
       << " layerIdx=" << _layerIdx << " numNodes=" << _nodes.size()
       << " prevNumNodes=" << _prevNumNodes << endl;
}

void Layer::Load(const cnpy::npz_t &npzArray) {
  cnpy::NpyArray weightArr = npzArray.at("w_layer_" + to_string(_layerIdx));
  Print("weightArr=", weightArr.shape);
  assert(_weights.size() == weightArr.num_vals);
  memcpy(_weights.data(), weightArr.data<float>(),
         sizeof(float) * weightArr.num_vals);

  cnpy::NpyArray biasArr = npzArray.at("b_layer_" + to_string(_layerIdx));
  Print("biasArr=", biasArr.shape);
  assert(_bias.size() == biasArr.num_vals);
  memcpy(_bias.data(), biasArr.data<float>(), sizeof(float) * biasArr.num_vals);
}

void RandomizeWeights() {
  /*
random_device rd;
default_random_engine dre(rd());
normal_distribution<float> distribution(0.0, 0.01);

generate(_weights.begin(), _weights.end(),
         [&]() { return distribution(dre); });
generate(_bias.begin(), _bias.end(), [&]() { return distribution(dre); });
*/
}

Layer::~Layer() { delete _hashTables; }

size_t Layer::computeActivation(std::vector<float> &dataOut,
                                const std::vector<float> &dataIn) const {
  //cerr << "computeActivation layer=" << _layerIdx << endl;
  assert(dataIn.size() == _prevNumNodes);

  if (_hashTables) {
    std::vector<int> hashes = _dwtaHasher->getHashEasy(dataIn);
    std::vector<int> hashIndices = _hashTables->hashesToIndex(hashes);
    std::vector<const std::vector<int> *> actives =
        _hashTables->retrieveRaw(hashIndices);
    /*
    Print("dataIn", dataIn);
    Print("hashIndices", hashIndices);
    cerr << "hashes2 " << hashes.size() << " " << hashIndices.size() << " " << actives.size() << endl;
    Print("actives", actives);
    for (const std::vector<int> *v : actives) {
      //Print("v", *v);
      cerr << v->size() << " ";
    }
    cerr << endl;
    */
    std::unordered_set<int> nodesIdx;
    for (const std::vector<int> *v : actives) {
      //Print("v", *v);
      //cerr << v->size() << " ";
      std::copy(v->begin(), v->end(), std::inserter(nodesIdx, nodesIdx.end()));
    }
    //cerr << "nodesIdx" << nodesIdx.size() << endl;

    dataOut.resize(_numNodes, 0);
    for (int nodeIdx: nodesIdx) {
      const Node &node = getNode(nodeIdx);
      dataOut.at(nodeIdx) = node.computeActivation(dataIn);
    }

  } else {
    dataOut.resize(_numNodes);
    for (size_t nodeIdx = 0; nodeIdx < _nodes.size(); ++nodeIdx) {
      const Node &node = getNode(nodeIdx);
      dataOut.at(nodeIdx) = node.computeActivation(dataIn);
    }
  }
}

void Layer::HashWeights() {
  if (_hashTables) {
    for (Node &node : _nodes) {
      node.HashWeights(*_hashTables, *_dwtaHasher);
    }
  }
}

} // namespace hieu