#include "ndnbritehelper.h"

#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/net-device.h"
#include "ns3/net-device-container.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/data-rate.h"
#include "ns3/rng-seed-manager.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

#include <iostream>
#include <fstream>

NS_LOG_COMPONENT_DEFINE ("NDNBriteHelper");

using namespace ns3;
using namespace ns3::ndn;

NDNBriteHelper::NDNBriteHelper (std::string confFile,
                                          std::string seedFile,
                                          std::string newseedFile)
  : m_confFile (confFile),
    m_seedFile (seedFile),
    m_newSeedFile (newseedFile),
    m_numAs (0),
    m_topology (NULL),
    m_numNodes (0),
    m_numEdges (0),
    m_queueName (NULL),
    m_queueSize (0)
{
  NS_LOG_FUNCTION (this);

  m_uv = CreateObject<UniformRandomVariable> ();

}

NDNBriteHelper::NDNBriteHelper (std::string confFile, std::string queueName, uint32_t queueSize)
  : m_confFile (confFile),
    m_queueName (queueName),
    m_queueSize (queueSize),
    m_numAs (0),
    m_topology (NULL),
    m_numNodes (0),
    m_numEdges (0)
{
  NS_LOG_FUNCTION (this);

  m_uv = CreateObject<UniformRandomVariable> ();

}

NDNBriteHelper::~NDNBriteHelper ()
{
  NS_LOG_FUNCTION (this);
  delete m_topology;

  while (!m_netDevices.empty ())
    {
      delete m_netDevices.back ();
      m_netDevices.pop_back ();
    }

  while (!m_asLeafNodes.empty ())
    {
      delete m_asLeafNodes.back ();
      m_asLeafNodes.pop_back ();
    }

  while (!m_nodesByAs.empty ())
    {
      delete m_nodesByAs.back ();
      m_nodesByAs.pop_back ();
    }
}

void
NDNBriteHelper::AssignStreams (int64_t streamNumber)
{
  m_uv->SetStream (streamNumber);
}

void
NDNBriteHelper::BuildBriteNodeInfoList (void)
{
  NS_LOG_FUNCTION (this);
  brite::Graph *g = m_topology->GetGraph ();
  for (int i = 0; i < g->GetNumNodes (); ++i)
    {
      BriteNodeInfo nodeInfo;
      nodeInfo.nodeId = g->GetNodePtr (i)->GetId ();
      nodeInfo.xCoordinate = g->GetNodePtr (i)->GetNodeInfo ()->GetCoordX ();
      nodeInfo.yCoordinate = g->GetNodePtr (i)->GetNodeInfo ()->GetCoordY ();
      nodeInfo.inDegree = g->GetNodePtr (i)->GetInDegree ();
      nodeInfo.outDegree = g->GetNodePtr (i)->GetOutDegree ();

      switch (g->GetNodePtr (i)->GetNodeInfo ()->GetNodeType ())
        {
        case brite::NodeConf::RT_NODE:

          if (((brite::RouterNodeConf*)(g->GetNodePtr (i)->GetNodeInfo ()))->GetASId () == -1)
            {
              m_numAs = nodeInfo.asId = 0;
            }
          else
            {
              m_numAs = nodeInfo.asId = ((brite::RouterNodeConf*)(g->GetNodePtr (i)->GetNodeInfo ()))->GetASId ();
            }

          switch (((brite::RouterNodeConf*)(g->GetNodePtr (i)->GetNodeInfo ()))->GetRouterType ())
            {
            case brite::RouterNodeConf::RT_NONE:
              nodeInfo.type = "RT_NONE ";
              break;
            case brite::RouterNodeConf::RT_LEAF:
              nodeInfo.type = "RT_LEAF ";
              break;
            case brite::RouterNodeConf::RT_BORDER:
              nodeInfo.type = "RT_BORDER";
              break;
            case brite::RouterNodeConf::RT_STUB:
              nodeInfo.type = "RT_STUB ";
              break;
            case brite::RouterNodeConf::RT_BACKBONE:
              nodeInfo.type = "RT_BACKBONE ";
              break;
            default:
              NS_FATAL_ERROR ("Topology::Output(): Improperly classfied Router node encountered...");
            }
          break;

        case brite::NodeConf::AS_NODE:
          m_numAs = nodeInfo.asId =
              ((brite::ASNodeConf*)(g->GetNodePtr (i)->GetNodeInfo ()))->GetASId ();

          switch (((brite::ASNodeConf*)(g->GetNodePtr (i)->GetNodeInfo ()))->GetASType ())
            {
            case brite::ASNodeConf::AS_NONE:
              nodeInfo.type = "AS_NONE ";
              break;
            case brite::ASNodeConf::AS_LEAF:
              nodeInfo.type = "AS_LEAF ";
              break;
            case brite::ASNodeConf::AS_STUB:
              nodeInfo.type = "AS_STUB ";
              break;
            case brite::ASNodeConf::AS_BORDER:
              nodeInfo.type = "AS_BORDER ";
              break;
            case brite::ASNodeConf::AS_BACKBONE:
              nodeInfo.type = "AS_BACKBONE ";
              break;
            default:
              NS_FATAL_ERROR ("Topology::Output(): Improperly classfied AS node encountered...");
            }
          break;
        }

      m_briteNodeInfoList.push_back (nodeInfo);
    }

  //Currently m_numAs stores the highest AS number.  We want m_numAs to store the number
  //of AS created in the topology.  Since AS numbering starts at 0 we add one to get
  //the correct count
  m_numAs++;
}

void
NDNBriteHelper::BuildBriteEdgeInfoList (void)
{
  NS_LOG_FUNCTION (this);
  brite::Graph *g = m_topology->GetGraph ();
  std::list<brite::Edge*>::iterator el;
  std::list<brite::Edge*> edgeList = g->GetEdges ();

  for (el = edgeList.begin (); el != edgeList.end (); el++)
    {
      BriteEdgeInfo edgeInfo;
      edgeInfo.edgeId = (*el)->GetId ();
      edgeInfo.srcId = (*el)->GetSrc ()->GetId ();
      edgeInfo.destId = (*el)->GetDst ()->GetId ();
      edgeInfo.length = (*el)->Length ();

      switch ((*el)->GetConf ()->GetEdgeType ())
        {
        case brite::EdgeConf::RT_EDGE:
          edgeInfo.delay = ((brite::RouterEdgeConf*)((*el)->GetConf ()))->GetDelay ();
          edgeInfo.bandwidth = (*el)->GetConf ()->GetBW ();
          //If there is only one AS, BRITE will use -1 as AS Number.  We want it to be 0 instead.
          edgeInfo.asFrom = (((brite::RouterNodeConf*)((*el)->GetSrc ()->GetNodeInfo ()))->GetASId () == -1) ? 0 : ((brite::RouterNodeConf*)((*el)->GetSrc ()->GetNodeInfo ()))->GetASId ();
          edgeInfo.asTo = (((brite::RouterNodeConf*)((*el)->GetDst ()->GetNodeInfo ()))->GetASId () == -1) ? 0 : ((brite::RouterNodeConf*)((*el)->GetDst ()->GetNodeInfo ()))->GetASId ();
          break;

        case brite::EdgeConf::AS_EDGE:
          edgeInfo.delay =  -1;     /* No delay for AS Edges */
          edgeInfo.bandwidth = (*el)->GetConf ()->GetBW ();
          edgeInfo.asFrom = ((brite::ASNodeConf*)((*el)->GetSrc ()->GetNodeInfo ()))->GetASId ();
          edgeInfo.asTo = ((brite::ASNodeConf*)((*el)->GetDst ()->GetNodeInfo ()))->GetASId ();
          break;

        default:
          NS_FATAL_ERROR ("Topology::Output(): Invalid Edge type encountered...");
        }

      switch ((*el)->GetConf ()->GetEdgeType ())
        {
        case brite::EdgeConf::RT_EDGE:
          switch (((brite::RouterEdgeConf*)(*el)->GetConf ())->GetRouterEdgeType ())
            {
            case brite::RouterEdgeConf::RT_NONE:
              edgeInfo.type = "E_RT_NONE ";
              break;
            case brite::RouterEdgeConf::RT_STUB:
              edgeInfo.type = "E_RT_STUB ";
              break;
            case brite::RouterEdgeConf::RT_BORDER:
              edgeInfo.type = "E_RT_BORDER ";
              break;
            case brite::RouterEdgeConf::RT_BACKBONE:
              edgeInfo.type = "E_RT_BACKBONE ";
              break;
            default:
              NS_FATAL_ERROR ("Output(): Invalid router edge type...");
            }
          break;

        case brite::EdgeConf::AS_EDGE:
          switch (((brite::ASEdgeConf*)((*el)->GetConf ()))->GetASEdgeType ())
            {
            case brite::ASEdgeConf::AS_NONE:
              edgeInfo.type = "E_AS_NONE ";
              break;
            case brite::ASEdgeConf::AS_STUB:
              edgeInfo.type = "E_AS_STUB ";
              break;
            case brite::ASEdgeConf::AS_BORDER:
              edgeInfo.type = "E_AS_BORDER ";
              break;
            case brite::ASEdgeConf::AS_BACKBONE:
              edgeInfo.type = "E_AS_BACKBONE ";
              break;
            default:
              NS_FATAL_ERROR ("BriteOutput(): Invalid AS edge type...");
            }
          break;

        default:
          NS_FATAL_ERROR ("BriteOutput(): Invalid edge type...");

        }

      m_briteEdgeInfoList.push_back (edgeInfo);
    }
}

ns3::Ptr<ns3::Node>
NDNBriteHelper::GetLeafNodeForAs (uint32_t asNum, uint32_t leafNum)
{
  return m_asLeafNodes[asNum]->Get (leafNum);
}

ns3::Ptr<ns3::Node>
NDNBriteHelper::GetNodeForAs (uint32_t asNum, uint32_t nodeNum)
{
  return m_nodesByAs[asNum]->Get (nodeNum);
}

uint32_t
NDNBriteHelper::GetNNodesForAs (uint32_t asNum)
{
  return m_nodesByAs[asNum]->GetN ();
}

uint32_t
NDNBriteHelper::GetNLeafNodesForAs (uint32_t asNum)
{
  return m_asLeafNodes[asNum]->GetN ();
}

uint32_t
NDNBriteHelper::GetNNodesTopology () const
{
  return m_numNodes;
}

uint32_t
NDNBriteHelper::GetNEdgesTopology () const
{
  return m_numEdges;
}

uint32_t
NDNBriteHelper::GetNAs (void) const
{
  return m_numAs;
}

uint32_t
NDNBriteHelper::GetSystemNumberForAs (uint32_t asNum) const
{
  return m_systemForAs[asNum];
}

void NDNBriteHelper::GenerateBriteTopology (void)
{
  NS_ASSERT_MSG (m_topology == NULL, "Brite Topology Already Created");

  //check to see if need to generate seed file
  bool generateSeedFile = m_seedFile.empty ();

  if (generateSeedFile)
    {
      NS_LOG_LOGIC ("Generating BRITE Seed file");

      std::ofstream seedFile;

      //overwrite file if already there
      seedFile.open ("briteSeedFile.txt", std::ios_base::out | std::ios_base::trunc);

      //verify open
      NS_ASSERT (!seedFile.fail ());

      //Generate seed file expected by BRITE
      //need unsigned shorts 0-65535
      seedFile << "PLACES " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << std::endl;
      seedFile << "CONNECT " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << std::endl;
      seedFile << "EDGE_CONN " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << std::endl;
      seedFile << "GROUPING " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << std::endl;
      seedFile << "ASSIGNMENT " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << std::endl;
      seedFile << "BANDWIDTH " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << " " << m_uv->GetInteger (0, 65535) << std::endl;
      seedFile.close ();

      //if we're using NS3 generated seed files don't want brite to create a new seed file.
      m_seedFile = m_newSeedFile = "briteSeedFile.txt";
    }

  brite::Brite br (m_confFile, m_seedFile, m_newSeedFile);
  m_topology = br.GetTopology ();
  BuildBriteNodeInfoList ();
  BuildBriteEdgeInfoList ();

  //brite automatically spits out the seed values used to a seperate file so no need to keep this anymore
  if (generateSeedFile)
    {
      remove ("briteSeedFile.txt");
    }

}

void
NDNBriteHelper::BuildBriteTopology ()
{
  NS_LOG_FUNCTION (this);

  GenerateBriteTopology ();

  //not using MPI so each AS is on system number 0
  for (uint32_t i = 0; i < m_numAs; ++i)
    {
      m_systemForAs.push_back (0);
    }

  //create all nodes with system number 0
  m_nodes.Create (m_briteNodeInfoList.size ());

  m_numNodes = m_briteNodeInfoList.size ();

  NS_LOG_DEBUG (m_numNodes << " nodes created in BRITE topology");

  //stack.Install (m_nodes);

  ConstructTopology ();
}

void
NDNBriteHelper::BuildBriteTopology (const uint32_t systemCount)
{
  NS_LOG_FUNCTION (this);

  GenerateBriteTopology ();

  //determine as system number for each AS
  NS_LOG_LOGIC ("Assigning << " << m_numAs << " AS to " << systemCount << " MPI instances");
  for (uint32_t i = 0; i < m_numAs; ++i)
    {
      int val = i % systemCount;
      m_systemForAs.push_back (val);
      NS_LOG_INFO ("AS: " << i << " System: " << val);
    }

  //create nodes
  for (NDNBriteHelper::BriteNodeInfoList::iterator it = m_briteNodeInfoList.begin (); it != m_briteNodeInfoList.end (); ++it)
    {
      m_nodes.Add (CreateObject<Node> (GetSystemNumberForAs ((*it).asId)));
      m_numNodes++;
    }

  NS_LOG_INFO (m_numNodes << " nodes created in BRITE topology");

  //stack.Install (m_nodes);

  ConstructTopology ();
}

void
NDNBriteHelper::ConstructTopology ()
{
  NS_LOG_FUNCTION (this);
  //create one node container to hold leaf nodes for attaching
  for (uint32_t i = 0; i < m_numAs; ++i)
    {
      m_asLeafNodes.push_back (new NodeContainer ());
      m_nodesByAs.push_back (new NodeContainer ());
    }

  for (NDNBriteHelper::BriteEdgeInfoList::iterator it = m_briteEdgeInfoList.begin (); it != m_briteEdgeInfoList.end (); ++it)
    {
      // Set the link delay
      // The brite value for delay is given in milliseconds
      m_britePointToPointHelper.SetChannelAttribute ("Delay",
                                                     TimeValue (Seconds ((*it).delay/1000.0)));

      // The brite value for data rate is given in Mbps
      m_britePointToPointHelper.SetDeviceAttribute ("DataRate",
                                                    DataRateValue (DataRate ((*it).bandwidth * kbpsToBps)));

      if (!m_queueName.empty()) {
        if (m_queueName.compare("DropTail_Packets") == 0) {
          m_britePointToPointHelper.SetQueue("ns3::DropTailQueue", 
            "Mode", StringValue("QUEUE_MODE_PACKETS"),
            "MaxPackets", StringValue(boost::lexical_cast<std::string>(m_queueSize)));

        } else if (m_queueName.compare("DropTail_Bytes") == 0) {
          m_britePointToPointHelper.SetQueue("ns3::DropTailQueue", 
            "Mode", StringValue("QUEUE_MODE_BYTES"),
            "MaxBytes", StringValue(boost::lexical_cast<std::string>(m_queueSize * 1000)));

        } else if (m_queueName.compare("Fair_Packets") == 0) {
          m_britePointToPointHelper.SetQueue("ns3::FairQueue", 
            "Mode", StringValue("QUEUE_MODE_PACKETS"),
            "MaxPackets", StringValue(boost::lexical_cast<std::string>(m_queueSize)));

        } else if (m_queueName.compare("Fair_Bytes") == 0) {
          m_britePointToPointHelper.SetQueue("ns3::FairQueue", 
            "Mode", StringValue("QUEUE_MODE_BYTES"),
            "MaxBytes", StringValue(boost::lexical_cast<std::string>(m_queueSize * 1000)));

        } else if (m_queueName.compare("PriorityQueue_Bytes") == 0) {
          m_britePointToPointHelper.SetQueue("ns3::PriorityQueue", 
            "Mode", StringValue("QUEUE_MODE_BYTES"),
            "MaxBytes", StringValue(boost::lexical_cast<std::string>(m_queueSize * 1000)));

        } else if (m_queueName.compare("PriorityQueue_Packets") == 0) {
          m_britePointToPointHelper.SetQueue("ns3::PriorityQueue", 
            "Mode", StringValue("QUEUE_MODE_PACKETS"),
            "MaxBytes", StringValue(boost::lexical_cast<std::string>(m_queueSize)));

        } else if (m_queueName.compare("WFQ") == 0) {
          m_britePointToPointHelper.SetQueue("ns3::WFQ", 
            "Mode", StringValue("QUEUE_MODE_BYTES"),
            "MaxBytes", StringValue(boost::lexical_cast<std::string>(m_queueSize * 1000)));

        } else if (m_queueName.compare("REDQueue_Bytes") == 0) {
          m_britePointToPointHelper.SetQueue("ns3::RedQueue", 
            "Mode", StringValue("QUEUE_MODE_BYTES"),
            "QueueLimit", StringValue(boost::lexical_cast<std::string>(m_queueSize * 1000)));

        } else if (m_queueName.compare("REDQueue_Packets") == 0) {
          m_britePointToPointHelper.SetQueue("ns3::RedQueue", 
            "Mode", StringValue("QUEUE_MODE_PACKETS"),
            "QueueLimit", StringValue(boost::lexical_cast<std::string>(m_queueSize)));
        } else {
          std::cout << "Invalid Queue Name selected (NDNBriteHelper)" << std::endl;
          exit(-1);
        }
      }

      m_netDevices.push_back ( new NetDeviceContainer ( m_britePointToPointHelper.Install (m_nodes.Get ((*it).srcId), m_nodes.Get ((*it).destId))));

      m_numEdges++;

    }

  NS_LOG_INFO ("Created " << m_numEdges << " edges in BRITE topology");

  //iterate through all nodes and add leaf nodes for each AS
  for (NDNBriteHelper::BriteNodeInfoList::iterator it = m_briteNodeInfoList.begin (); it != m_briteNodeInfoList.end (); ++it)
    {
      m_nodesByAs[(*it).asId]->Add (m_nodes.Get ((*it).nodeId));

      if ((*it).type == "RT_LEAF ")
        {
          m_asLeafNodes[(*it).asId]->Add (m_nodes.Get ((*it).nodeId));
        }
    }
}
