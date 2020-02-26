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

#include "Support/macros.h"

#include <cassert>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>

#include "edgeserver.grpc.pb.h"

namespace uiiit {
namespace edge {

struct LambdaResponse;

/**
 * Generic edge server providing a multi-threaded gRPC server interface for the
 * processing of lambda functions.
 */
class EdgeServer
{
  // Class encompassing the state and logic needed to serve a request.
  class CallData
  {
    enum CallStatus { CREATE, PROCESS, FINISH };

   public:
    explicit CallData(rpc::EdgeServer::AsyncService* aService,
                      grpc::ServerCompletionQueue*   aCq,
                      EdgeServer&                    aEdgeServer);

    void Proceed();

   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    rpc::EdgeServer::AsyncService* theService;
    // The producer-consumer queue where for asynchronous server notifications.
    grpc::ServerCompletionQueue* theCq;

    EdgeServer& theEdgeServer;

    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    grpc::ServerContext theContext;

    // What we get from the client.
    rpc::LambdaRequest theRequest;
    // What we send back to the client.
    rpc::LambdaResponse theResponse;

    // The means to get back to the client.
    grpc::ServerAsyncResponseWriter<rpc::LambdaResponse> theResponder;

    // The current serving state.
    CallStatus theStatus;
  };

 public:
  NONCOPYABLE_NONMOVABLE(EdgeServer);

  //! Create an edge server with a given number of threads.
  explicit EdgeServer(const std::string& aServerEndpoint,
                      const size_t       aNumThreads);

  virtual ~EdgeServer();

  //! Start the server. No more configuration allowed after this call.
  void run();

  //! Wait until termination of the server.
  void wait();

 protected:
  /**
   * \return the set of the identifiers of the threads that have been
   * spawned during the call to run(). The cardinality of this set
   * if equal to the number of threads specified in the ctor. If
   * run() has not (yet) been called, then an empty set is returned.
   */
  std::set<std::thread::id> threadIds() const;

 private:
  //! Thread execution body.
  void handle();

  //! Execute initialization logic immediately after run().
  virtual void init() {
  }

  //! Perform actual processing of a lambda request.
  virtual rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) = 0;

 protected:
  mutable std::mutex theMutex;
  const std::string  theServerEndpoint;
  const size_t       theNumThreads;

 private:
  std::unique_ptr<grpc::ServerCompletionQueue> theCq;
  rpc::EdgeServer::AsyncService                theService;
  std::unique_ptr<grpc_impl::Server>           theServer;
  std::list<std::thread>                       theHandlers;
}; // end class EdgeServer

} // end namespace edge
} // end namespace uiiit