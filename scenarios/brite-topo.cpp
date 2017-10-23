/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "../extensions/randnetworks/networkgenerator.h"

#include <sstream>

namespace ns3 {



int
main(int argc, char* argv[])
{

  std::string confFile = "brite_configs/brite_medium_bw.conf";
  std::string linkErrorParam = "0";
  std::string queue = "DropTail_Bytes";

  // Read Commandline Parameters
  CommandLine cmd;
  cmd.AddValue("briteConfig", "Brite-config file", confFile);
  cmd.AddValue("linkErrors", "Number of link errors during simulation", linkErrorParam);
  cmd.Parse(argc, argv);

  // 1) Parse Brite-Config and generate network with BRITE
  ns3::ndn::NetworkGenerator gen(confFile, queue, 50);

  uint32_t simTime = 10 * 60* 1000; // Simtime in milliseconds (10 minutes)

  int min_bw_as = -1;
  int max_bw_as = -1;
  int min_bw_leaf = -1;
  int max_bw_leaf = -1;
  int additional_random_connections_as = -1;
  int additional_random_connections_leaf = - 1;

  // Config for Medium connectivity from example.cc
  min_bw_as = 3000;
  max_bw_as = 5000;

  min_bw_leaf = 500;
  max_bw_leaf = 1500;

  //
  additional_random_connections_as = gen.getNumberOfAS ();
  additional_random_connections_leaf = gen.getAllASNodesFromAS (0).size () / 2;

  // Add random connections between nodes
  gen.randomlyAddConnectionsBetweenTwoAS (additional_random_connections_as,min_bw_as,max_bw_as,5,20);
  gen.randomlyAddConnectionsBetweenTwoNodesPerAS(additional_random_connections_leaf,min_bw_leaf,max_bw_leaf,5,20);

  int linkErrors = std::stoi(linkErrorParam);
  int averageErrorTime = 60;
  for(int i = 0; i < linkErrors; i++)
    gen.creatRandomLinkFailure(0, simTime, averageErrorTime * 0.8, averageErrorTime * 1.2);
  gen.exportLinkFailures("link-failures.csv");


  // 2) Create Callees, Callers, and cross-traffic clients/server
  PointToPointHelper *p2p = new PointToPointHelper;
  p2p->SetChannelAttribute ("Delay", StringValue ("2ms"));

  p2p->SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  gen.randomlyPlaceNodes (4, "DataServer",ns3::ndn::NetworkGenerator::LeafNode, p2p);
  p2p->SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  gen.randomlyPlaceNodes (4, "DataClient",ns3::ndn::NetworkGenerator::LeafNode, p2p);


  // 3) Install NDN Stack on all nodes
  ns3::ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore ("ns3::ndn::cs::Stats::Lru","MaxSize", "1"); // Disable caches
  ndnHelper.Install(gen.getAllASNodes ());// install all on network nodes...

  NodeContainer dataServer = gen.getCustomNodes ("DataServer");
  ndnHelper.Install(dataServer);
  NodeContainer dataClient = gen.getCustomNodes ("DataClient");
  ndnHelper.Install(dataClient);

  // 4) Install routing helper on all nodes
  ns3::ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/data/", "/localhost/nfd/strategy/best-route");

  // 7) Configure Cross-Traffic
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  for (uint i = 0; i < dataServer.size(); i++) {
    Ptr<Node> server = dataServer.Get(i);
    std::string prefix = "/data/" + boost::lexical_cast<std::string>(server->GetId());
    producerHelper.SetPrefix(prefix);
    producerHelper.Install(server);
    ndnGlobalRoutingHelper.AddOrigins(prefix, server);
  }

  // Cross-Traffic Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute("Frequency", StringValue("100")); // 100 interests a second
  for (uint i = 0; i < dataClient.size(); i++) {

    Ptr<Node> server = dataServer.Get(i);
    Ptr<Node> cl = dataClient.Get(i);
    std::string prefix = "/data/" + boost::lexical_cast<std::string>(server->GetId());
    consumerHelper.SetPrefix(prefix);

    consumerHelper.Install(cl);
  }

  // Calculate and install FIBs
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ns3::ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes (); 

  std::cout << "Start" << std::endl;

  Simulator::Stop(MilliSeconds(simTime));

  Simulator::Run();
  Simulator::Destroy();

  std::cout << "Simulation completed" << std::endl;

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}