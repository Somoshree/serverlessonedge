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

#include "Edge/edgecomputerclient.h"
#include "Support/glograii.h"
#include "Support/saver.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <cstdlib>

namespace po = boost::program_options;
namespace ec = uiiit::edge;

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii myGlogRaii(argv[0]);

  std::string myServerEndpoint;
  std::string myOutputFile;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
    ("help,h", "produce help message")
    ("server-endpoint",
     po::value<std::string>(&myServerEndpoint)->default_value("localhost:6476"),
     "Server end-point.")
    ("output-file",
     po::value<std::string>(&myOutputFile)->default_value("/dev/stdout"),
     "Output file name.")
    ;
  // clang-format on

  try {
    po::variables_map myVarMap;
    po::store(po::parse_command_line(argc, argv, myDesc), myVarMap);
    po::notify(myVarMap);

    if (myVarMap.count("help")) {
      std::cout << myDesc << std::endl;
      return EXIT_FAILURE;
    }

    if (myServerEndpoint.empty()) {
      throw std::runtime_error("Empty end-point: " + myServerEndpoint);
    }

    if (myOutputFile.empty()) {
      throw std::runtime_error("Empty output filename: " + myOutputFile);
    }

    uiiit::support::Saver  mySaver(myOutputFile, true, true, false);
    ec::EdgeComputerClient myClient(myServerEndpoint);
    myClient.StreamUtil(
        [&mySaver](const std::string& aProcessor, const float aUtil) {
          mySaver(aProcessor + " " + std::to_string(aUtil));
        });

    return EXIT_SUCCESS;

  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();
  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
