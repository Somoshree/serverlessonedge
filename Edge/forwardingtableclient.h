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

#include "RpcSupport/simpleclient.h"

#include "edgerouter.grpc.pb.h"

namespace uiiit {
namespace edge {

class ForwardingTableClient final : public rpc::SimpleClient<rpc::EdgeRouter>
{
 public:
  explicit ForwardingTableClient(const std::string& aServerEndpoint);

  //! \return the number of forwarding tables.
  size_t numTables();

  /**
   * \return the forwarding table with given identifier, on an empty map if it
   *         does not exist
   */
  std::map<std::string, std::map<std::string, std::pair<float, bool>>>
  table(const size_t aId);

  //! \return a string representing in ASCII the forwarding tables.
  std::string dump();

  //! Remove all forwarding entries.
  void flush();

  //! Change/add a forwarding entry.
  void change(const std::string& aLambda,
              const std::string& aDestination,
              const float        aWeight,
              const bool         aFinal);

  //! Remove a forwarding entry.
  void remove(const std::string& aLambda, const std::string& aDestination);

 private:
  void send(const rpc::EdgeRouterConf& aReq);
};

} // end namespace edge
} // end namespace uiiit
