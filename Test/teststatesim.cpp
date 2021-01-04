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

#include "StateSim/counter.h"
#include "StateSim/job.h"
#include "StateSim/network.h"
#include "StateSim/scenario.h"

#include "gtest/gtest.h"

#include <boost/filesystem.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>

#include <glog/logging.h>

namespace uiiit {
namespace statesim {

#include "Test/Data/datastatesim.h"
#include "Test/Data/taskstatesim.h"

struct TestStateSim : public ::testing::Test {
  TestStateSim()
      : theTestDir("TO_REMOVE_DIR") {
  }

  void SetUp() {
    boost::filesystem::remove_all(theTestDir);
    boost::filesystem::create_directories(theTestDir);
  }

  void TearDown() {
    boost::filesystem::remove_all(theTestDir);
  }

  bool prepareNetworkFiles() const {
    std::ofstream myEdges((theTestDir / "edges").string());
    myEdges << theEdges;

    std::ofstream myLinks((theTestDir / "links").string());
    myLinks << theLinks;

    std::ofstream myNodes((theTestDir / "nodes").string());
    myNodes << theNodes;

    std::ofstream myGraph((theTestDir / "graph").string());
    myGraph << theGraph;

    return static_cast<bool>(myEdges) and static_cast<bool>(myLinks) and
           static_cast<bool>(myNodes) and static_cast<bool>(myGraph);
  }

  bool prepareTaskFiles() const {
    std::ofstream myTasks((theTestDir / "tasks").string());
    myTasks << theTasks;
    return static_cast<bool>(myTasks);
  }

  struct MapSave {
    template <class T>
    void operator()(const std::string& aPath, const T& aMap) const {
      std::ofstream myOutfile(aPath);
      if (not myOutfile) {
        throw std::runtime_error("Could not open file for writing: " + aPath);
      }
      for (const auto& elem : aMap) {
        myOutfile << elem.first << ' ' << elem.second << '\n';
      }
    }
  };

  void analyze_tasks(const bool aStatefulOnly) {
    const auto myJobs =
        loadJobs("batch_task.csv", 1000, 100, {{"", 1.0}}, 42, aStatefulOnly);
    LOG(INFO) << "#jobs " << myJobs.size();
    std::map<size_t, size_t> myChainSizeHisto;
    std::map<size_t, size_t> myNumStatesHisto;
    std::map<size_t, size_t> myStateSizeHisto;
    std::map<size_t, size_t> myArgSizeHisto;
    std::map<size_t, size_t> myNumOpsHisto;
    std::map<size_t, size_t> myNumDepsHisto;
    for (const auto& myJob : myJobs) {
      myChainSizeHisto[myJob.tasks().size()]++;
      myNumStatesHisto[myJob.stateSizes().size()]++;
      for (const auto myStateSize : myJob.stateSizes()) {
        myStateSizeHisto[myStateSize]++;
      }
      for (const auto& myTask : myJob.tasks()) {
        myArgSizeHisto[myTask.size()]++;
        myNumOpsHisto[myTask.ops()]++;
        myNumDepsHisto[myTask.deps().size()]++;
      }
    }

    // save everything to files
    MapSave()("chain-size.dat", myChainSizeHisto);
    MapSave()("num-states.dat", myNumStatesHisto);
    MapSave()("state-size.dat", myStateSizeHisto);
    MapSave()("arg-size.dat", myArgSizeHisto);
    MapSave()("num-ops.dat", myNumOpsHisto);
    MapSave()("num-deps.dat", myNumDepsHisto);
  }

  static void printNetwork(const Network& aNetwork) {
    VLOG(2) << "network nodes";
    for (const auto& myNode : aNetwork.nodes()) {
      VLOG(2) << myNode.second.toString();
    }
    VLOG(2) << "network links";
    for (const auto& myLink : aNetwork.links()) {
      VLOG(2) << myLink.second.toString();
    }
  }

  const boost::filesystem::path theTestDir;
};

TEST_F(TestStateSim, test_boost_graph) {
  typedef boost::adjacency_list<boost::listS,
                                boost::vecS,
                                boost::directedS,
                                boost::no_property,
                                boost::property<boost::edge_weight_t, float>,
                                boost::no_property,
                                boost::listS>
                                                        Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor VertexDescriptor;
  typedef std::pair<int, int>                           Edge;

  std::vector<Edge>  myEdges({
      {0, 1},
      {1, 0},
      {1, 2},
      {2, 1},
      {2, 3},
      {3, 2},
      {3, 4},
      {4, 3},
      {0, 4},
      {4, 0},
  });
  std::vector<float> myWeights({1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5, 0.5});

  Graph myGraph(
      myEdges.data(), myEdges.data() + myEdges.size(), myWeights.data(), 5);

  // vector for storing distance property
  std::vector<float> myDistances(boost::num_vertices(myGraph));

  // vector for storing the predecessor map
  std::vector<VertexDescriptor> myPredecessors(boost::num_vertices(myGraph));

  // get the first vertex
  const auto mySource = *(vertices(myGraph).first);
  // invoke variant 2 of Dijkstra's algorithm
  boost::dijkstra_shortest_paths(
      myGraph,
      mySource,
      boost::predecessor_map(
          boost::make_iterator_property_map(myPredecessors.begin(),
                                            get(boost::vertex_index, myGraph)))
          .distance_map(myDistances.data()));

  std::vector<float>            myExpectedDistances({0, 1, 2, 1, 0.5});
  std::vector<VertexDescriptor> myExpectedPredecessors({0, 0, 1, 4, 0});
  for (auto vi = boost::vertices(myGraph).first; vi != vertices(myGraph).second;
       ++vi) {
    EXPECT_FLOAT_EQ(myExpectedDistances[*vi], myDistances[*vi])
        << "node " << *vi;
    EXPECT_EQ(myExpectedPredecessors[*vi], myPredecessors[*vi])
        << "node " << *vi;
    VLOG(1) << *vi << ' ' << myDistances[*vi] << ' ' << myPredecessors[*vi];
  }
}

TEST_F(TestStateSim, test_network_files) {
  Counter<int> myCounter;
  ASSERT_THROW(loadNodes((theTestDir / "nodes").string(), myCounter),
               std::runtime_error);
  ASSERT_THROW(loadLinks((theTestDir / "links").string(), myCounter),
               std::runtime_error);

  ASSERT_TRUE(prepareNetworkFiles());

  const auto myNodes = loadNodes((theTestDir / "nodes").string(), myCounter);
  ASSERT_EQ(123, myNodes.size());
  const auto myLinks = loadLinks((theTestDir / "links").string(), myCounter);
  ASSERT_EQ(140, myLinks.size());
  ASSERT_EQ(myNodes.size() + myLinks.size() - 1, myLinks.back().id());
}

TEST_F(TestStateSim, test_network_from_files) {
  ASSERT_TRUE(prepareNetworkFiles());
  Network myNetwork((theTestDir / "nodes").string(),
                    (theTestDir / "links").string(),
                    (theTestDir / "edges").string());
  printNetwork(myNetwork);
  ASSERT_EQ(140, myNetwork.links().size());
  ASSERT_EQ(159, myNetwork.nodes().size());
  ASSERT_EQ(123, myNetwork.processing().size());

  const auto myDesc = myNetwork.nextHop("rpi3_0", "rpi3_53");
  ASSERT_FLOAT_EQ(0.216f, myDesc.first);
  ASSERT_EQ("link_rpi3_0", myDesc.second);

  for (const auto& mySrc : myNetwork.nodes()) {
    for (const auto& myDst : myNetwork.nodes()) {
      const auto myDesc = myNetwork.nextHop(mySrc.first, myDst.first);
      ASSERT_GE(myDesc.first, 0.0f);
      VLOG(1) << mySrc.first << " -> " << myDst.first << ": " << myDesc.first
              << " (" << myDesc.second << ")";
    }
  }

  ASSERT_EQ(113, myNetwork.clients().size());
  for (const auto& myClient : myNetwork.clients()) {
    ASSERT_TRUE(myClient != nullptr);
    ASSERT_EQ(std::string::npos, myClient->name().find("server"));
  }
}

TEST_F(TestStateSim, test_network) {
  /*
       +-----------+
   +---+6  sw2     +--+
   |   +-----------+  |
   |12                |13
   |                  |
+--+---+         +----+-+
|      |         |      |
|  D   +--+      |   E  |
|3     |  |      |4     |
+------+  |      +------+
          |11
          |
     +----+------+
  +--+5   sw1    +------+
  |  +--------+--+      |
  |           |         |
  |7          |8        |10
  |           |         |
+-+----+  +---+--+  +---+--+
|      |  |      |  |      |
|  A   |  |  B   |  |  C   |
|0     |  |1     |  |2     |
+------+  +---+--+  +---+--+
              |    9    |
              +---------+

  A-E are processing nodes
  sw1-sw2 are networking devices
  A, B, and C are clients

  All links have 10 Mb/s capacity except:
  - BC (100 Mb/s)
  - Dsw2 and Esw2 (1000 Mb/s)
  */
  const std::set<Node> myNodes({
      Node("A", 0, 1e9, 1ull << 30, Affinity::Gpu),
      Node("B", 1, 1e9, 1ull << 30, Affinity::Gpu),
      Node("C", 2, 1e9, 1ull << 30, Affinity::Gpu),
      Node("D", 3, 20 * 1e9, 1ull << 36, Affinity::Cpu),
      Node("E", 4, 20 * 1e9, 1ull << 36, Affinity::Cpu),
      Node("sw1", 5),
      Node("sw2", 6),
  });

  const std::set<Link> myLinks({
      Link(Link::Type::Node, "link_0_5", 7, 10e6f),
      Link(Link::Type::Node, "link_1_5", 8, 10e6f),
      Link(Link::Type::Node, "link_1_2", 9, 100e6f),
      Link(Link::Type::Node, "link_2_5", 10, 10e6f),
      Link(Link::Type::Node, "link_5_3", 11, 10e6f),
      Link(Link::Type::Node, "link_3_6", 12, 1000e6f),
      Link(Link::Type::Node, "link_4_6", 13, 1000e6f),
  });

  const std::map<std::string, std::set<std::string>> myEdges({
      {"A", {"link_0_5"}},
      {"B", {"link_1_5", "link_1_2"}},
      {"C", {"link_2_5", "link_1_2"}},
      {"D", {"link_5_3", "link_3_6"}},
      {"E", {"link_4_6"}},
      {"sw1", {"link_0_5", "link_1_5", "link_2_5", "link_5_3"}},
      {"sw2", {"link_3_6", "link_4_6"}},
      {"link_0_5", {"A", "sw1"}},
      {"link_1_5", {"B", "sw1"}},
      {"link_1_2", {"B", "C"}},
      {"link_2_5", {"C", "sw1"}},
      {"link_5_3", {"sw1", "D"}},
      {"link_3_6", {"D", "sw2"}},
      {"link_4_6", {"E", "sw2"}},
  });

  const std::set<std::string> myClients({
      "A",
      "B",
      "C",
  });

  Network myNetwork(myNodes, myLinks, myEdges, myClients);
  printNetwork(myNetwork);

  ASSERT_EQ(myNodes.size(), myNetwork.nodes().size());
  ASSERT_EQ(myLinks.size(), myNetwork.links().size());
  ASSERT_EQ(myClients.size(), myNetwork.clients().size());
  ASSERT_EQ(5, myNetwork.processing().size());

  // check distance matrix
  for (const auto& mySrc : myNetwork.nodes()) {
    for (const auto& myDst : myNetwork.nodes()) {
      const auto myDesc = myNetwork.nextHop(mySrc.first, myDst.first);
      ASSERT_GE(myDesc.first, 0.0f);
      VLOG(1) << mySrc.first << " -> " << myDst.first << ": " << myDesc.first
              << " (" << myDesc.second << ")";
    }
  }
  ASSERT_EQ("link_1_5", myNetwork.nextHop("B", "A").second);
  ASSERT_EQ("B", myNetwork.nextHop("B", "B").second);
  ASSERT_EQ("link_1_2", myNetwork.nextHop("B", "C").second);
  ASSERT_EQ("link_1_5", myNetwork.nextHop("B", "D").second);
  ASSERT_EQ("link_1_5", myNetwork.nextHop("B", "E").second);

  // check tx time
  const size_t N = 1000000;
  ASSERT_FLOAT_EQ(2 * N * 8 / 10e6, myNetwork.txTime("A", "B", N));
  ASSERT_FLOAT_EQ(2 * N * 8 / 10e6 + 2 * N * 8 / 1000e6,
                  myNetwork.txTime("A", "E", N));
  ASSERT_FLOAT_EQ(N * 8 / 100e6, myNetwork.txTime("C", "B", N));
}

TEST_F(TestStateSim, test_all_tasks) {
  ASSERT_TRUE(prepareTaskFiles());
  const std::map<std::string, double> myWeights({
      {"less-often", 1},
      {"more-often", 10},
  });

  const auto myJobs = loadJobs(
      (theTestDir / "tasks").string(), 1000, 100, myWeights, 42, false);
  ASSERT_EQ(22, myJobs.size());

  const auto& myJob = myJobs[10];
  VLOG(1) << myJob.toString();
  ASSERT_EQ(10, myJob.id());
  ASSERT_FLOAT_EQ(3.0875, myJob.start());
  ASSERT_EQ(30, myJob.retSize());
  ASSERT_EQ(7, myJob.tasks().size());
  ASSERT_EQ(40000, myJob.tasks()[3].ops());
  ASSERT_EQ(59, myJob.tasks()[3].size());
  ASSERT_EQ(std::vector<size_t>({0, 1, 2}), myJob.tasks()[3].deps());
  ASSERT_EQ(std::vector<size_t>({39, 59, 49}), myJob.stateSizes());

  std::map<std::string, size_t> myFunctions;
  for (const auto& myJob : myJobs) {
    for (const auto& myTask : myJob.tasks()) {
      myFunctions[myTask.func()]++;
    }
  }

  ASSERT_EQ(2, myFunctions.size());
  ASSERT_EQ(51, myFunctions["more-often"]);
  ASSERT_EQ(5, myFunctions["less-often"]);
}

TEST_F(TestStateSim, test_stateful_tasks) {
  ASSERT_TRUE(prepareTaskFiles());
  const auto myJobs = loadJobs(
      (theTestDir / "tasks").string(), 1000, 100, {{"", 1.0}}, 42, true);
  ASSERT_EQ(6, myJobs.size());
  for (const auto& myJob : myJobs) {
    auto myAllStateless = true;
    for (const auto& myTask : myJob.tasks()) {
      if (not myTask.deps().empty()) {
        myAllStateless = false;
      }
    }
    ASSERT_FALSE(myAllStateless) << myJob.toString();
  }
}

TEST_F(TestStateSim, test_scenario) {
  ASSERT_TRUE(prepareNetworkFiles());
  ASSERT_TRUE(prepareTaskFiles());

  const std::map<std::string, double> myFuncWeights({
      {"lambda1", 1},
      {"lambda2", 1},
      {"lambda3", 1},
      {"lambda4", 1},
      {"lambda5", 10},
  });

  const std::map<Affinity, double> myAffinityWeights({
      {Affinity::Cpu, 10},
      {Affinity::Gpu, 1},
  });

  Scenario myScenario(Scenario::Conf{(theTestDir / "nodes").string(),
                                     (theTestDir / "links").string(),
                                     (theTestDir / "edges").string(),
                                     (theTestDir / "tasks").string(),
                                     1000,
                                     100,
                                     myFuncWeights,
                                     42,
                                     true,
                                     myAffinityWeights});
}

TEST_F(TestStateSim, DISABLED_analyze_tasks_stateful) {
  analyze_tasks(true);
}

TEST_F(TestStateSim, DISABLED_analyze_tasks_all) {
  analyze_tasks(false);
}

} // namespace statesim
} // namespace uiiit
