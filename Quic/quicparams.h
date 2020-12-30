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

#include <string>

#include "Support/conf.h"

#include <folly/Optional.h>
#include <folly/SocketAddress.h>
#include <proxygen/httpclient/samples/curl/CurlClient.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <proxygen/lib/http/HTTPHeaders.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <proxygen/lib/http/SynchronizedLruQuicPskCache.h>
#include <proxygen/lib/http/session/HQSession.h>
#include <quic/QuicConstants.h>
#include <quic/fizz/client/handshake/QuicPskCache.h>
#include <quic/server/QuicServerTransport.h>

namespace uiiit {
namespace edge {

struct HTTPVersion {
  std::string version;
  std::string canonical;
  uint16_t    major{1};
  uint16_t    minor{1};

  bool parse(const std::string& verString) {
    // version, major and minor are fields of struct HTTPVersion
    version = verString;
    if (version.length() == 1) {
      major     = folly::to<uint16_t>(version);
      minor     = 0;
      canonical = folly::to<std::string>(major, ".", minor);
      return true;
    }
    std::string delimiter = ".";
    std::size_t pos       = version.find(delimiter);
    if (pos == std::string::npos) {
      LOG(ERROR) << "Invalid http-version string: " << version
                 << ", defaulting to HTTP/1.1";
      major     = 1;
      minor     = 1;
      canonical = folly::to<std::string>(major, ".", minor);
      return false;
    }

    try {
      std::string majorVer = version.substr(0, pos);
      std::string minorVer = version.substr(pos + delimiter.length());
      major                = folly::to<uint16_t>(majorVer);
      minor                = folly::to<uint16_t>(minorVer);
      canonical            = folly::to<std::string>(major, ".", minor);
      return true;
    } catch (const folly::ConversionError&) {
      LOG(ERROR) << "Invalid http-version string: " << version
                 << ", defaulting to HTTP/1.1";
      major     = 1;
      minor     = 1;
      canonical = folly::to<std::string>(major, ".", minor);
      return false;
    }
  }
};

/**
 * Struct to hold both HTTP/3 and HTTP/2 settings for EdgeQuicServer and
 * EdgeQuicClient
 */
struct HQParams {

  // Transport section
  std::string                                  host;
  uint16_t                                     port;
  folly::Optional<folly::SocketAddress>        localAddress;
  folly::Optional<folly::SocketAddress>        remoteAddress;
  std::vector<quic::QuicVersion>               quicVersions;
  std::vector<std::string>                     supportedAlpns;
  quic::TransportSettings                      transportSettings;
  std::string                                  congestionControlName;
  folly::Optional<quic::CongestionControlType> congestionControl;
  bool                                         earlyData;
  folly::Optional<int64_t>                     rateLimitPerThread;
  std::chrono::milliseconds                    connectTimeout;
  std::string                                  ccpConfig;

  // HTTP section
  uint16_t                              h2port;
  folly::Optional<folly::SocketAddress> localH2Address;
  HTTPVersion                           httpVersion;
  proxygen::HTTPHeaders                 httpHeaders;
  std::vector<folly::StringPiece>       httpPaths;

  std::chrono::milliseconds txnTimeout;

  size_t                    httpServerThreads;
  std::chrono::milliseconds httpServerIdleTimeout;
  std::vector<int>          httpServerShutdownOn;
  bool                      httpServerEnableContentCompression;
  bool                      h2cEnabled;

  // Partial reliability section
  folly::Optional<uint64_t> prChunkSize;
  folly::Optional<uint64_t> prChunkDelayMs;

  // Fizz options
  std::shared_ptr<quic::QuicPskCache> pskCache;
  fizz::server::ClientAuthMode clientAuth{fizz::server::ClientAuthMode::None};

  /**
   * Struct Constructor (set default values according to the boolean parameter
   * isServer)
   */
  HQParams(bool isServer) {
    // *** Common Settings Section ***
    host = "127.0.0.1";
    port = 6473;
    if (isServer) {
      localAddress = folly::SocketAddress(host, port, true);
    } else {
      remoteAddress = folly::SocketAddress(host, port, true);
      // local_address empty by default, local_address not set
    }

    // *** TransportSettings ***
    quicVersions = {quic::QuicVersion::MVFST,
                    quic::QuicVersion::MVFST_D24,
                    quic::QuicVersion::MVFST_EXPERIMENTAL,
                    quic::QuicVersion::QUIC_DRAFT,
                    quic::QuicVersion::QUIC_DRAFT_LEGACY};
    // draft_version = 0 by default -> no if branch
    // protocol = "" by default -> else branch
    supportedAlpns                                          = {"h1q-fb",
                      "h1q-fb-v2",
                      proxygen::kH3FBCurrentDraft,
                      proxygen::kH3CurrentDraft,
                      proxygen::kH3LegacyDraft,
                      proxygen::kHQCurrentDraft};
    transportSettings.advertisedInitialConnectionWindowSize = 1024 * 1024 * 10;
    transportSettings.advertisedInitialBidiLocalStreamWindowSize  = 256 * 1024;
    transportSettings.advertisedInitialBidiRemoteStreamWindowSize = 256 * 1024;
    transportSettings.advertisedInitialUniStreamWindowSize        = 256 * 1024;
    congestionControlName                                         = "cubic";
    congestionControl = quic::congestionControlStrToType(congestionControlName);
    // since congestion control cannot be null since we are using default
    // values
    if (congestionControl) {
      transportSettings.defaultCongestionController = congestionControl.value();
    }
    transportSettings.maxRecvPacketSize = quic::kDefaultUDPReadBufferSize;
    transportSettings.numGROBuffers_    = quic::kDefaultNumGROBuffers;
    transportSettings.pacingEnabled     = false;
    // pacingEnabled false by default, no if branch
    transportSettings.batchingMode = quic::getQuicBatchingMode(
        static_cast<uint32_t>(quic::QuicBatchingMode::BATCHING_MODE_NONE));
    transportSettings.useThreadLocalBatching = false;
    transportSettings.threadLocalDelay       = std::chrono::microseconds(1000);
    transportSettings.maxBatchSize           = quic::kDefaultQuicMaxBatchSize;
    transportSettings.turnoffPMTUD           = true;
    transportSettings.partialReliabilityEnabled = false;
    if (!isServer) {
      transportSettings.shouldDrain = false;
      transportSettings.attemptEarlyData =
          false; // CHECK!!! (WHETHER TO USE 0-RTT)
    }
    transportSettings.connectUDP       = false;
    transportSettings.maxCwndInMss     = quic::kLargeMaxCwndInMss;
    transportSettings.disableMigration = false;
    // FLAGS_use_inplace_write = false by default, no if branch
    // FLAGS_rate_limit = -1 by default, no if branch
    connectTimeout                      = std::chrono::milliseconds(2000);
    ccpConfig                           = "";
    transportSettings.d6dConfig.enabled = false;
    transportSettings.d6dConfig.probeRaiserConstantStepSize = 10;
    // d6d_probe_raiser_type = 0 default so we can use the following
    transportSettings.d6dConfig.raiserType =
        quic::ProbeSizeRaiserType::ConstantStep;
    transportSettings.d6dConfig.blackholeDetectionWindow =
        std::chrono::seconds(5);
    transportSettings.d6dConfig.blackholeDetectionThreshold = 5;
    transportSettings.d6dConfig.enabled                     = false;
    transportSettings.d6dConfig.advertisedBasePMTU          = 1252;
    transportSettings.d6dConfig.advertisedRaiseTimeout =
        std::chrono::seconds(600);
    transportSettings.d6dConfig.advertisedProbeTimeout =
        std::chrono::seconds(600);
    transportSettings.maxRecvBatchSize                = 32;
    transportSettings.shouldUseRecvmmsgForBatchRecv   = true;
    transportSettings.advertisedInitialMaxStreamsBidi = 100;
    transportSettings.advertisedInitialMaxStreamsUni  = 100;
    transportSettings.tokenlessPacer                  = true;

    // *** HTTP Settings ***
    h2port         = 6667; // "HTTP/2 server port"
    localH2Address = folly::SocketAddress(host, h2port, true);
    // std::thread::hardware_concurrency() << can be quite a lot...
    httpServerThreads                  = 5;
    httpServerIdleTimeout              = std::chrono::milliseconds(60000);
    httpServerShutdownOn               = {SIGINT, SIGTERM}; //<<<<<<<<<<<<<
    httpServerEnableContentCompression = false;
    h2cEnabled                         = false;
    httpVersion.parse("1.1");
    txnTimeout = std::chrono::milliseconds(120000);
    folly::split(',', "/lambda", httpPaths);
    // parse HTTP headers
    httpHeaders = CurlService::CurlClient::parseHeaders("");
    // Set the host header
    if (!httpHeaders.exists(proxygen::HTTP_HEADER_HOST)) {
      httpHeaders.set(proxygen::HTTP_HEADER_HOST, host);
    }

    // *** Partial Reliability Settings ***
    prChunkSize    = folly::to<uint64_t>(16);
    prChunkDelayMs = folly::to<uint64_t>(0);

    // *** FizzSettings***
    earlyData = false; // CHECK the function of this
    // psk_file is empty by default, else branch
    pskCache = std::make_shared<proxygen::SynchronizedLruQuicPskCache>(1000);
  }
};

/**
 * Basically a wrapper for HQParams, provides ctor methods for both
 * EdgeServerQuic and EdgeClientQuic
 */
class QuicParamsBuilder
{
 public:
  /**
   * Ctor for EdgeClientQuic & for EdgeServerQuic if server-conf is empty
   *
   * TODO: check if it is needed to use boost options from CLI to initialize the
   * client Params or if it is sufficient to initialize with default values
   * (not important now)
   */
  QuicParamsBuilder(uiiit::support::Conf quicConf); // for EdgeServerQuic
  // need to see for the client

  /**
   * need to keep the distinction between the first two members if we do not
   * want to allow user to have the possibility to configure all the
   * QuicParameters
   */
  static std::list<std::string> theConfigurableQuicParamsList;
  // list of all the parameters in HQParams struct, this is needed in order to
  // check if a key provided in the Support::Conf exists or not and to retrieve
  // the element in which we need to insert the value
  std::list<std::string>             theQuicParamsList;
  HQParams                           theQuicParams;
  std::map<std::string, std::string> theInvalidParams;
};

} // namespace edge
} // namespace uiiit