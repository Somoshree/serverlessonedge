/*
 ___ ___ __     __ ____________
|   |   |  |   |__|__|__   ___/   Ubiquitout Internet @ IIT-CNR
|   |   |  |  /__/  /  /  /    C++ edge computing libraries and tools
|   |   |  |/__/  /   /  /  https://bitbucket.org/ccicconetti/edge_computing/
|_______|__|__/__/   /__/

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2018 Claudio Cicconetti <https://about.me/ccicconetti>

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "Support/chrono.h"
#include "localoptimizer.h"

#include <map>
#include <mutex>

namespace uiiit {
namespace edge {

class LocalOptimizerFactory;

/**
 * Stateless optimizer: take action to change the weight towards a
 * destination immediately based on the last latency reported.
 *
 * An exponentially weighted smoothing is applied:
 *
 * Weight(t) = alpha * Latency(t)  + (1 - alpha) * Weight(t=1)
 */
class LocalOptimizerAsync final : public LocalOptimizer
{
  friend class LocalOptimizerFactory;

 private:
  struct Elem {
    double theWeight;
    double theTimestamp;
  };

  void operator()(const rpc::LambdaRequest& aReq,
                  const std::string&        aDestination,
                  const double              aTime) override;

  explicit LocalOptimizerAsync(ForwardingTable& aForwardingTable,
                               const double     aAlpha);

 private:
  // ctor arguments
  const double theAlpha;

  // internal state
  std::mutex                                         theMutex;
  std::map<std::string, std::map<std::string, Elem>> theWeights;
  support::Chrono                                    theChrono;

  // static configuration
  static constexpr double stalePeriod() {
    return 5; // seconds
  }
};

} // namespace edge
} // namespace uiiit
