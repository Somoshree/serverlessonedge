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

#include "Quic/hqsessioncontroller.h"

#include <proxygen/lib/http/session/HQDownstreamSession.h>

namespace uiiit {
namespace edge {

HQSessionController::HQSessionController(
    const HQParams&                       params,
    const HTTPTransactionHandlerProvider& httpTransactionHandlerProvider)
    : params_(params)
    , httpTransactionHandlerProvider_(httpTransactionHandlerProvider) {
}

proxygen::HQSession* HQSessionController::createSession() {
  wangle::TransportInfo tinfo;
  session_ =
      new proxygen::HQDownstreamSession(params_.txnTimeout, this, tinfo, this);
  return session_;
}

void HQSessionController::startSession(std::shared_ptr<quic::QuicSocket> sock) {
  CHECK(session_);
  session_->setSocket(std::move(sock));
  session_->startNow();
  VLOG(10) << "HQSessionController::startSession\n";
}

void HQSessionController::onTransportReady(
    proxygen::HTTPSessionBase* /*session*/) {
  VLOG(10) << "HQSessionController::onTransportReady\n";
}

void HQSessionController::onDestroy(const proxygen::HTTPSessionBase&) {
  LOG(INFO) << "HQSessionController::onDestroy\n";
}

proxygen::HTTPTransactionHandler*
HQSessionController::getRequestHandler(proxygen::HTTPTransaction& /*txn*/,
                                       proxygen::HTTPMessage* msg) {
  VLOG(10) << "HQSessionController::getRequestHandler\n";
  return httpTransactionHandlerProvider_(msg, params_);
}

proxygen::HTTPTransactionHandler* FOLLY_NULLABLE
HQSessionController::getParseErrorHandler(
    proxygen::HTTPTransaction* /*txn*/,
    const proxygen::HTTPException& /*error*/,
    const folly::SocketAddress& /*localAddress*/) {
  VLOG(10) << "HQSessionController::getParseErrorHandler\n";
  return nullptr;
}

proxygen::HTTPTransactionHandler* FOLLY_NULLABLE
HQSessionController::getTransactionTimeoutHandler(
    proxygen::HTTPTransaction* /*txn*/,
    const folly::SocketAddress& /*localAddress*/) {
  VLOG(10) << "HQSessionController::getTransactionTimeoutHandler\n";
  return nullptr;
}

void HQSessionController::attachSession(
    proxygen::HTTPSessionBase* /*session*/) {
  VLOG(10) << "HQSessionController::attachSession\n";
}

void HQSessionController::detachSession(
    const proxygen::HTTPSessionBase* /*session*/) {
  VLOG(10) << "HQSessionController::detachSession\n";
  delete this;
}

} // namespace edge
} // namespace uiiit