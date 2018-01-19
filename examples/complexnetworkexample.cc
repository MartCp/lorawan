/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */

#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"
#include "ns3/lora-mac.h"
#include "ns3/lora-phy.h"
#include "ns3/lora-channel.h"
#include "ns3/lora-tag.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/hex-grid-position-allocator.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/building-penetration-loss.h"
#include <algorithm>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LoraThroughputTest");

std::vector<int> spreadingFactorCounters(6, 0);
std::vector<int> centralGwSpreadingFactorCounters(6, 0);
std::vector<int> interferedCounters(6, 0);
std::vector<int> nodesWithSpreadingFactor(6, 0);
std::vector<int> nodesWithSpreadingFactorCentralGw(6, 0);
std::vector<int> sentSpreadingFactorCounters(6, 0);
std::vector<int> totalSentSpreadingFactorCounters(6, 0);
std::vector<int> spreadingFactorLossCounters(6, 0);
std::vector<int> underSensitivityCounters(6, 0);

std::list<Ptr<Packet const> > packetTracker;
std::list<Ptr<Packet const> > centralGwPacketTracker;
std::list<Ptr<Packet const> > centralGwSuccessful;
std::list<Ptr<Packet const> > centralGwInterfered;
std::list<Ptr<Packet const> > centralGwNoMoreReceivers;
std::list<Ptr<Packet const> > centralGwUnderSensitivity;

// Global metrics (including all gateways)
int totalPackets = 0;
int interferedPackets = 0;
int noMoreReceiversPackets = 0;
int rxBeginEvents = 0;
int rxEndEvents = 0;
int bytes = 0;

// Central gateway metrics
int centralGwDevices = 0;
int centralGwTotalPackets = 0;
int centralGwSuccessfulPackets = 0;
int centralGwInterferedPackets = 0;
int centralGwNoMoreReceiversPackets = 0;
int centralGwUnderSensitivityPackets = 0;
int centralGwRxBeginEvents = 0;
int centralGwRxEndEvents = 0;

int outOfRangeDevices = 0;
int packetsSentOnChannel = 0;
int dutyCycleLimitations = 0;

// Network settings
int nDevices = 10;
int gatewayRings = 1;
int nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
double radius = 7500;
double gatewayRadius = 7500;
bool buildingsEnabled = true;
double simulationTime = 60*60*24;
int appPeriodSeconds = 60*60*24;
// Output settings
bool printSpreadingFactors = true;
bool printPower = true;
// Options
bool prune = 0;
bool nosf12 = 0;
bool powerBackoffEnabled = false;   // Whether to enable power backoff or not
double powerBackoff = 7;            // The reduction in power experienced by backed-off devices
double powerBackoffThreshold = 10;  // The margin after which to enable power backoff

/**************************
*  Central Gw callbacks  *
**************************/

// This callback is called when a packet intended for the central gw is sent
void
CentralGwTransmissionCallback (Ptr<Packet> packet)
{
  // NS_LOG_INFO ("Transmitted a packet" );
  centralGwTotalPackets++;
  centralGwPacketTracker.push_back (packet);
  LoraTag tag;
  packet -> RemovePacketTag (tag);
  packet->AddPacketTag(tag);
  uint8_t sf = tag.GetSpreadingFactor ();
  // NS_LOG_DEBUG ("Sent spreading factor " << unsigned(sf));
  sentSpreadingFactorCounters.at(int(sf)-7)++;
}

void
CentralGwPacketReceptionCallback (Ptr<Packet> packet)
{
  // Remove the successfully received packet from the list of sent ones
  // NS_LOG_INFO ("Received a packet");
  // NS_LOG_INFO ("Packet to remove " << packet);
  if (find(centralGwPacketTracker.begin (), centralGwPacketTracker.end (), packet) != centralGwPacketTracker.end ())
  {
    centralGwSuccessfulPackets++;
    centralGwPacketTracker.remove (packet);
    centralGwSuccessful.push_back (packet);
    LoraTag tag;
    packet -> RemovePacketTag (tag);
    packet->AddPacketTag(tag);
    uint8_t sf = tag.GetSpreadingFactor ();
    // NS_LOG_DEBUG ("Received spreading factor " << unsigned(sf));
    centralGwSpreadingFactorCounters.at(int(sf)-7)++;
  }
}

void
CentralGwInterferenceCallback (Ptr<Packet> packet)
{
  // NS_LOG_INFO ("A packet was lost because of interference" );
  if (find(centralGwPacketTracker.begin (), centralGwPacketTracker.end (), packet) != centralGwPacketTracker.end ())
  {
    centralGwInterferedPackets++;
    centralGwPacketTracker.remove (packet);
    centralGwInterfered.push_back (packet);
    LoraTag tag;
    packet->RemovePacketTag (tag);
    packet->AddPacketTag(tag);
    uint8_t culpritSf = tag.GetDestroyedBy ();
    uint8_t sf = tag.GetSpreadingFactor ();
    // NS_LOG_DEBUG ("Destroyed packet with spreading factor " << unsigned(sf) << " by SF " << unsigned(culpritSf));
    spreadingFactorLossCounters.at(int(culpritSf)-7)++;
    interferedCounters.at(int(sf)-7)++;
  }
}

void
CentralGwNoMoreReceiversCallback (Ptr<Packet const> packet)
{
  // NS_LOG_INFO ("A packet was lost because there were no more receivers" );
  if (find(centralGwPacketTracker.begin (), centralGwPacketTracker.end (), packet) != centralGwPacketTracker.end ())
  {
    centralGwNoMoreReceiversPackets++;
    centralGwPacketTracker.remove (packet);
    centralGwNoMoreReceivers.push_back (packet);
  }
}

void
CentralGwUnderSensitivityCallback (Ptr<Packet> packet)
{
  // NS_LOG_INFO ("A packet was lost because power at receiver was under sensitivity" );
  if (find(centralGwPacketTracker.begin (), centralGwPacketTracker.end (), packet) != centralGwPacketTracker.end ())
  {
    centralGwUnderSensitivityPackets++;
    centralGwPacketTracker.remove (packet);
    centralGwUnderSensitivity.push_back (packet);

    LoraTag tag;
    packet -> RemovePacketTag (tag);
    packet->AddPacketTag(tag);
    uint8_t sf = tag.GetSpreadingFactor ();
    // NS_LOG_DEBUG ("Packet with SF " << unsigned(sf) << " arrived under sensitivity");
    underSensitivityCounters.at(int(sf)-7)++;
  }
}

void
CentralGwPhyRxBeginCallback (Ptr<Packet const> packet)
{
  centralGwRxBeginEvents++;
}

void
CentralGwPhyRxEndCallback (Ptr<Packet const> packet)
{
  centralGwRxEndEvents++;
}

/**********************
*  Global Callbacks  *
**********************/

void
TransmissionCallback (Ptr<Packet> packet)
{
  // NS_LOG_INFO ("Transmitted a packet" );
  totalPackets++;
  packetTracker.push_back (packet);
  LoraTag tag;
  packet -> RemovePacketTag (tag);
  packet->AddPacketTag(tag);
  uint8_t sf = tag.GetSpreadingFactor ();
  // NS_LOG_DEBUG ("Sent spreading factor " << unsigned(sf));
  totalSentSpreadingFactorCounters.at(int(sf)-7)++;
}

void
PacketReceptionCallback (Ptr<Packet> packet)
{
  // Remove the successfully received packet from the list of sent ones
  // NS_LOG_INFO ("Received a packet");
  // NS_LOG_INFO ("Packet to remove " << packet);
  packetTracker.remove (packet);
}

void
InterferenceCallback (Ptr<Packet> packet)
{
  // NS_LOG_INFO ("A packet was lost because of interference" );
  LoraTag tag;
  packet -> RemovePacketTag (tag);
  packet->AddPacketTag(tag);
  uint8_t sf = tag.GetDestroyedBy ();
  spreadingFactorLossCounters.at(int(sf)-7)++;
  interferedPackets++;
}

void
NoMoreReceiversCallback (Ptr<Packet const> packet)
{
  // NS_LOG_INFO ("A packet was lost because there were no more receivers" );
  noMoreReceiversPackets++;
}

void
PhyRxBeginCallback (Ptr<Packet const> packet)
{
  rxBeginEvents++;
}

void
PhyRxEndCallback (Ptr<Packet const> packet)
{
  rxEndEvents++;
}

/*********************
*  Other callbacks  *
*********************/

void
PacketBelowChannelThreshold (Ptr<Packet const> packet)
{
  // NS_LOG_DEBUG ("Packet not sent because under channel threshold");
}

void
PacketSentOnChannelCallback (Ptr<Packet const> packet)
{
  // NS_LOG_INFO ("A packet was sent in the channel");
  packetsSentOnChannel++;
}

void
CannotSendBecauseDutyCycleCallback (Ptr<Packet const> packet)
{
  // NS_LOG_DEBUG (Cannot send a packet because of duty cycle limitations);
  dutyCycleLimitations++;
}

// void
// OccupiedPaths (int oldValue, int newValue)
// {
  // std::cout << "SF " << Simulator::Now ().GetSeconds () << " " << newValue << std::endl;
// }


void
PrintSpreadingFactors (NodeContainer endDevices, NodeContainer gateways, std::string filename )
{
  const char * c = filename.c_str();
  std::ofstream spreadingFactorFile;
  spreadingFactorFile.open (c);
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
  {
    Ptr<Node> object = *j;
    Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
    NS_ASSERT (position != 0);
    Ptr<NetDevice> netDevice = object->GetDevice (0);
    Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
    NS_ASSERT (loraNetDevice != 0);
    Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac()->GetObject<EndDeviceLoraMac> ();
    int sf = int(mac->GetSpreadingFactor ());
    Vector pos = position->GetPosition ();
    spreadingFactorFile << pos.x << " " << pos.y << " " << sf << std::endl;
  }
  // Also print the gateways
  // for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j)
  // {
    // Ptr<Node> object = *j;
    // Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
    // Vector pos = position->GetPosition ();
    // spreadingFactorFile << pos.x << " " << pos.y << " GW" << std::endl;
  // }
  spreadingFactorFile.close();
}

// Iterate our nodes and set up their Spreading Factor based on distance from
// the closest gateway
void
SetSpreadingFactorsUp (NodeContainer endDevices, NodeContainer gateways, Ptr<LoraChannel> channel)
{
  nodesWithSpreadingFactor.at(0) = 0;
  nodesWithSpreadingFactor.at(1) = 0;
  nodesWithSpreadingFactor.at(2) = 0;
  nodesWithSpreadingFactor.at(3) = 0;
  nodesWithSpreadingFactor.at(4) = 0;
  nodesWithSpreadingFactor.at(5) = 0;

  std::ofstream spreadingFactorFile;
  std::ofstream powerFile;
  if (printSpreadingFactors)
  {
    spreadingFactorFile.open ("spreadingFactors.txt");
  }
  if (printPower)
  {
    powerFile.open ("power.txt");
  }
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
  {
    Ptr<Node> object = *j;
    Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
    NS_ASSERT (position != 0);
    Ptr<NetDevice> netDevice = object->GetDevice (0);
    Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
    NS_ASSERT (loraNetDevice != 0);
    Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac()->GetObject<EndDeviceLoraMac> ();
    Ptr<EndDeviceLoraPhy> phy = loraNetDevice->GetPhy () -> GetObject<EndDeviceLoraPhy> ();
    NS_ASSERT (mac != 0);

    // Try computing the distance from each gateway and find the closest one
    Ptr<Node> bestGateway = gateways.Get(0);
    Ptr<MobilityModel> bestGatewayPosition = bestGateway->GetObject<MobilityModel> ();
    double highestRxPower = channel->GetRxPower (phy->GetTxPowerdBm (), position, bestGatewayPosition);  // dBm

    for (NodeContainer::Iterator currentGw = gateways.Begin () + 1;
         currentGw != gateways.End (); ++currentGw)
    {
      // Compute the power received from the current gateway
      Ptr<Node> curr = *currentGw;
      Ptr<MobilityModel> currPosition = curr->GetObject<MobilityModel> ();
      double currentRxPower = channel->GetRxPower (phy->GetTxPowerdBm (), position, currPosition);  // dBm

      if (currentRxPower > highestRxPower)
      {
        bestGateway = curr;
        bestGatewayPosition = curr->GetObject<MobilityModel> ();
        highestRxPower = currentRxPower;
      }
    }

    Vector pos = position->GetPosition ();
    Vector gwpos = bestGatewayPosition->GetPosition ();
    // NS_LOG_DEBUG ("Rx Power: " << highestRxPower);
    double rxPower = highestRxPower;

    // Get the Gw sensitivity
    Ptr<NetDevice> gatewayNetDevice = bestGateway->GetDevice(0);
    Ptr<LoraNetDevice> gatewayLoraNetDevice = gatewayNetDevice->GetObject<LoraNetDevice> ();
    Ptr<GatewayLoraPhy> gatewayPhy = gatewayLoraNetDevice->GetPhy ()->GetObject<GatewayLoraPhy> ();
    const double *gwSensitivity = gatewayPhy->sensitivity;

    if (printPower) powerFile << pos.x << " " << pos.y << " " << rxPower << std::endl;
    if (printSpreadingFactors) spreadingFactorFile << gwpos.x << " " << gwpos.y << " " << 0 << std::endl;

    // if(rxPower > *gwSensitivity+powerBackoffThreshold && powerBackoffEnabled) // Power Backoff
    // {
      // mac->SetSpreadingFactor (7);
      // phy->SetTxPowerdBm (phy->GetTxPowerdBm () - powerBackoff);
      // nodesWithSpreadingFactor.at(0)++;
      // if (printSpreadingFactors) spreadingFactorFile << pos.x << " " << pos.y << " 6" << std::endl;
    // }
    if(rxPower > *gwSensitivity)
    {
      mac->SetSpreadingFactor (7);
      nodesWithSpreadingFactor.at(0)++;
      if (printSpreadingFactors) spreadingFactorFile << pos.x << " " << pos.y << " 7" << std::endl;
    }
    else if (rxPower > *(gwSensitivity+1))
    {
      // NS_LOG_DEBUG ("Rx Power " << rxPower);
        mac->SetSpreadingFactor (8);
        nodesWithSpreadingFactor.at(1)++;
        if (printSpreadingFactors) spreadingFactorFile << pos.x << " " << pos.y << " 8" << std::endl;
    }
    else if (rxPower > *(gwSensitivity+2))
    {
        mac->SetSpreadingFactor (9);
        nodesWithSpreadingFactor.at(2)++;
        if (printSpreadingFactors) spreadingFactorFile << pos.x << " " << pos.y << " 9" << std::endl;
    }
    else if (rxPower > *(gwSensitivity+3))
    {
        mac->SetSpreadingFactor (10);
        nodesWithSpreadingFactor.at(3)++;
        if (printSpreadingFactors) spreadingFactorFile << pos.x << " " << pos.y << " 10" << std::endl;
    }
    else if (rxPower > *(gwSensitivity+4))
    {
        mac->SetSpreadingFactor (11);
        nodesWithSpreadingFactor.at(4)++;
        if (printSpreadingFactors) spreadingFactorFile << pos.x << " " << pos.y << " 11" << std::endl;
    }
    else if (rxPower > *(gwSensitivity+5))
    {
        mac->SetSpreadingFactor (12);
        nodesWithSpreadingFactor.at(5)++;
        if (nosf12) phy->SetTxPowerdBm (-100000);
        if (printSpreadingFactors) spreadingFactorFile << pos.x << " " << pos.y << " 12" << std::endl;
    }
    else
    {
        mac->SetSpreadingFactor (12);
        nodesWithSpreadingFactor.at(5)++;
        if (nosf12) phy->SetTxPowerdBm (-100000);
        if (printSpreadingFactors) spreadingFactorFile << pos.x << " " << pos.y << " 5" << std::endl;
        outOfRangeDevices++;
    }
  }
  if (printSpreadingFactors)
  {
    // Also print the gateways
    for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      Vector pos = position->GetPosition ();
      spreadingFactorFile << pos.x << " " << pos.y << " 0" << std::endl;
    }
    spreadingFactorFile.close();
  }
}

time_t oldtime = std::time(0);

// Periodically print simulation time
void PrintSimulationTime (void)
{
  // NS_LOG_INFO ("Time: " << Simulator::Now().GetHours());
  std::cout << "Simulated time: " << Simulator::Now().GetHours() << " hours" << std::endl;
  std::cout << "Real time from last call: " << std::time(0) - oldtime << " seconds" << std::endl;
  oldtime = std::time(0);
  // Print number of packets sent so far
  int totalSent = 0;
  for (int i = 0; i < 6; i++)
    {
      totalSent += sentSpreadingFactorCounters.at(i);
    }
  std::cout << "Packets sent so far: " << totalSent << std::endl;
  Simulator::Schedule (Minutes (30), &PrintSimulationTime);
}

int main(int argc, char *argv[])
{

  CommandLine cmd;
  cmd.AddValue("printSpreadingFactors", "Whether or not to print the spreading factors of the end devices", printSpreadingFactors);
  cmd.AddValue("printPower", "Whether or not to print the power received from an end device", printPower);
  cmd.AddValue("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue("nGateways", "Number of gateways to include in the simulation", nGateways);
  cmd.AddValue("gatewayRings", "Number of gateway rings to include", gatewayRings);
  cmd.AddValue("radius", "The radius of the area to simulate", radius);
  cmd.AddValue("powerBackoffEnabled", "Whether or not to include power backoff for close end devices", powerBackoffEnabled);
  cmd.AddValue("powerBackoff", "The amount of reduction in tx power", powerBackoff);
  cmd.AddValue("powerBackoffThreshold", "The margin after which power backoff is enabled", powerBackoffThreshold);
  cmd.AddValue("gatewayRadius", "The distance between two gateways", gatewayRadius);
  cmd.AddValue("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue("buildingsEnabled", "Whether to use buildings in the simulation or not", buildingsEnabled);
  cmd.AddValue("appPeriod", "The period in seconds to be used by periodically transmitting applications", appPeriodSeconds);
  cmd.AddValue("prune", "Whether or not to prune end devices", prune);
  cmd.AddValue("nosf12", "Whether or not to allow SF12 devices to transmit", nosf12);

  cmd.Parse (argc, argv);

  // Set up logging
  // LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
  // LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraMac", LOG_LEVEL_ALL);
  // LogComponentEnable("EndDeviceLoraMac", LOG_LEVEL_ALL);
  // LogComponentEnable("GatewayLoraMac", LOG_LEVEL_ALL);
  // LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraMacHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("PeriodicSenderHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("PeriodicSender", LOG_LEVEL_ALL);
  // LogComponentEnable("HexGridPositionAllocator", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraMacHeader", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);
  // LogComponentEnable("CorrelatedShadowingPropagationLossModel", LOG_LEVEL_INFO);
  // LogComponentEnable("BuildingPenetrationLoss", LOG_LEVEL_INFO);

  /***********
  *  Setup  *
  ***********/

  // Compute the number of gateways
  nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;

  // Create the time value from the period
  Time appPeriod = Seconds(appPeriodSeconds);

  // Mobility
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                 "rho", DoubleValue (radius),
                                 "X", DoubleValue (0.0),
                                 "Y", DoubleValue (0.0));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  spreadingFactorCounters.at(0) = 0;
  spreadingFactorCounters.at(1) = 0;
  spreadingFactorCounters.at(2) = 0;
  spreadingFactorCounters.at(3) = 0;
  spreadingFactorCounters.at(4) = 0;
  spreadingFactorCounters.at(5) = 0;

  spreadingFactorLossCounters.at(0) = 0;
  spreadingFactorLossCounters.at(1) = 0;
  spreadingFactorLossCounters.at(2) = 0;
  spreadingFactorLossCounters.at(3) = 0;
  spreadingFactorLossCounters.at(4) = 0;
  spreadingFactorLossCounters.at(5) = 0;


  /************************
  *  Create the channel  *
  ************************/

  // Create the lora channel object
  // Ptr<RandomPropagationLossModel> loss = CreateObject<RandomPropagationLossModel> ();
  // Ptr<ConstantRandomVariable> rv = CreateObject<ConstantRandomVariable> ();
  // rv->SetAttribute ("Constant", DoubleValue (0));

  // loss->SetAttribute ("Variable", PointerValue (rv));

  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
  loss->SetPathLossExponent(3.76);
  loss->SetReference(1, 8.1);

  // Create the correlated shadowing component
  Ptr<CorrelatedShadowingPropagationLossModel> shadowing = CreateObject<CorrelatedShadowingPropagationLossModel> ();

  // Aggregate shadowing to the logdistance loss
  loss->SetNext(shadowing);

  // Add the effect to the channel propagation loss
  // Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss> ();

  // shadowing->SetNext(buildingLoss);

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

  // Channel trace sources
  channel->TraceConnectWithoutContext("PacketSent",
                                  MakeCallback (&PacketSentOnChannelCallback));
  channel->TraceConnectWithoutContext("BelowChannelThreshold",
                                  MakeCallback (&PacketBelowChannelThreshold));
  /************************
  *  Create the helpers  *
  ************************/

  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LoraMacHelper
  LoraMacHelper macHelper = LoraMacHelper ();

  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();

  /************************
  *  Create End Devices  *
  ************************/

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (nDevices);

  // Assign a mobility model to each node
  mobility.Install (endDevices);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = endDevices.Begin ();
      j != endDevices.End (); ++j)
  {
    Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
    Vector position = mobility->GetPosition ();
    position.z = 1.2;
    mobility->SetPosition (position);
  }

  // Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  helper.Install (phyHelper, macHelper, endDevices);

  // Now end devices are connected to the channel

  // Connect the trace sources of the endDevices belonging to the inner circle to the callback
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j)
  {
    Ptr<Node> node = *j;
    Ptr<LoraNetDevice> loraNetDevice = node->GetDevice(0)->GetObject<LoraNetDevice> ();
    Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
    Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
    Ptr<MobilityModel> mobility = phy->GetMobility ();
    Ptr<MobilityModel> center = CreateObject<ConstantPositionMobilityModel> ();
    center->SetAttribute ("Position", Vector3DValue (Vector (0,0,0)));
    if (mobility->GetDistanceFrom (center) < gatewayRadius)
    {
      centralGwDevices++;
      nodesWithSpreadingFactorCentralGw.at(int(mac->GetSpreadingFactor())-7);
      phy->TraceConnectWithoutContext("StartSending",
                                    MakeCallback (&CentralGwTransmissionCallback));
      phy->TraceConnectWithoutContext("StartSending",
                                    MakeCallback (&TransmissionCallback));
      Ptr<LoraMac> mac = loraNetDevice->GetMac ();
      phy->TraceConnectWithoutContext("CannotSendBecauseDutyCycle",
                                    MakeCallback (&CannotSendBecauseDutyCycleCallback));
    }
  }

  /*********************
  *  Create Gateways  *
  *********************/

  // Create the gateway nodes (allocate them uniformely on the disc)
  NodeContainer gateways;
  gateways.Create (nGateways);

  Ptr<HexGridPositionAllocator> allocator = CreateObject<HexGridPositionAllocator> (gatewayRadius);
  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = gateways.Begin ();
      j != gateways.End (); ++j)
  {
    Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
    Vector position = mobility->GetPosition ();
    position.z = 15;
    mobility->SetPosition (position);
  }

  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  /************************
  *  Configure Gateways  *
  ************************/

  // Install reception paths on gateways
  int maxReceptionPaths = 8;
  for (NodeContainer::Iterator j = gateways.Begin();
      j != gateways.End (); j++)
  {

    Ptr<Node> object = *j;
    // Get the device
    Ptr<NetDevice> netDevice = object->GetDevice (0);
    Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
    NS_ASSERT (loraNetDevice != 0);
    Ptr<GatewayLoraPhy> gwPhy = loraNetDevice->GetPhy()->GetObject<GatewayLoraPhy> ();

    // TODO: Move this process inside the helper
    Ptr<SubBand> subBand = CreateObject<SubBand> (868, 869, 0.01);
    std::vector<Ptr<LogicalLoraChannel> > logicalChannels;
    logicalChannels.push_back (CreateObject<LogicalLoraChannel> (868.1, 0.125, subBand));
    logicalChannels.push_back (CreateObject<LogicalLoraChannel> (868.3, 0.125, subBand));
    logicalChannels.push_back (CreateObject<LogicalLoraChannel> (868.5, 0.125, subBand));

    std::vector<Ptr<LogicalLoraChannel> >::iterator it = logicalChannels.begin ();

    int receptionPaths = 0;
    while (receptionPaths < maxReceptionPaths)
    {
      if (it == logicalChannels.end())
        it = logicalChannels.begin();
      gwPhy->AddReceptionPath(*it);
      ++it;
      receptionPaths++;
    }

    // Set up height of the gateway
    Ptr<MobilityModel> gwMob = (*j)->GetObject<MobilityModel> ();
    Vector position = gwMob->GetPosition ();
    position.z = 15;
    gwMob->SetPosition (position);

    // Set up the callbacks on the central gateway
    if (j == gateways.Begin ())
    {
      gwPhy->TraceConnectWithoutContext("ReceivedPacket",
                                      MakeCallback (&CentralGwPacketReceptionCallback));
      gwPhy->TraceConnectWithoutContext("LostPacketBecauseInterference",
                                      MakeCallback (&CentralGwInterferenceCallback));
      gwPhy->TraceConnectWithoutContext("LostPacketBecauseNoMoreReceivers",
                                      MakeCallback (&CentralGwNoMoreReceiversCallback));
      gwPhy->TraceConnectWithoutContext("LostPacketBecauseUnderSensitivity",
                                      MakeCallback (&CentralGwUnderSensitivityCallback));
      gwPhy->TraceConnectWithoutContext("PhyRxBegin",
                                      MakeCallback (&CentralGwPhyRxBeginCallback));
      gwPhy->TraceConnectWithoutContext("PhyRxEnd",
                                      MakeCallback (&CentralGwPhyRxEndCallback));
    }
    // Global callbacks (every gateway)
    gwPhy->TraceConnectWithoutContext("ReceivedPacket",
                                    MakeCallback (&PacketReceptionCallback));
    gwPhy->TraceConnectWithoutContext("LostPacketBecauseInterference",
                                    MakeCallback (&InterferenceCallback));
    gwPhy->TraceConnectWithoutContext("LostPacketBecauseNoMoreReceivers",
                                    MakeCallback (&NoMoreReceiversCallback));
    gwPhy->TraceConnectWithoutContext("PhyRxBegin",
                                    MakeCallback (&PhyRxBeginCallback));
    gwPhy->TraceConnectWithoutContext("PhyRxEnd",
                                    MakeCallback (&PhyRxEndCallback));
  }

  /**********************
  *  Handle buildings  *
  **********************/

  double xLength = 130;
  double deltaX = 32;
  double yLength = 64;
  double deltaY = 17;
  int gridWidth = 2*radius/(xLength+deltaX);
  int gridHeight = 2*radius/(yLength+deltaY);
  if (buildingsEnabled == false)
  {
    gridWidth = 0;
    gridHeight = 0;
  }
  Ptr<GridBuildingAllocator> gridBuildingAllocator;
  gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
  gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (gridWidth));
  gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (xLength));
  gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (yLength));
  gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (deltaX));
  gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (deltaY));
  gridBuildingAllocator->SetAttribute ("Height", DoubleValue (6));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (2));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (4));
  gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (2));
  gridBuildingAllocator->SetAttribute ("MinX", DoubleValue (-gridWidth*(xLength+deltaX)/2+deltaX/2));
  gridBuildingAllocator->SetAttribute ("MinY", DoubleValue (-gridHeight*(yLength+deltaY)/2+deltaY/2));
  BuildingContainer bContainer = gridBuildingAllocator->Create (gridWidth * gridHeight);

  BuildingsHelper::Install (endDevices);
  BuildingsHelper::Install (gateways);
  BuildingsHelper::MakeMobilityModelConsistent ();

  // Print the buildings
  std::ofstream myfile;
  myfile.open ("buildings.txt");
  std::vector<Ptr<Building> >::const_iterator it;
  int j = 1;
  for (it = bContainer.Begin (); it != bContainer.End (); ++it, ++j)
  {
    Box boundaries = (*it)->GetBoundaries ();
    myfile << "set object " << j << " rect from " << boundaries.xMin << "," << boundaries.yMin << " to " << boundaries.xMax << "," << boundaries.yMax << std::endl;
  }
  myfile.close();

  /**********************************************
  *  Set up the end device's spreading factor  *
  **********************************************/

  SetSpreadingFactorsUp (endDevices, gateways, channel);

  NS_LOG_DEBUG ("Completed configuration");

  if (printSpreadingFactors)
  {
    PrintSpreadingFactors (endDevices, gateways, "endDevices.txt");
  }

  /***********************
  *  Prune end devices  *
  ***********************/

  // Not terribly efficient, could be better (with some added work we could do
  // only one pass of the end devices)

  // The container that will end up containing the subset of devices
  NodeContainer fewerDevices;

  // Position of the central gateway
  Ptr<MobilityModel> center = (*gateways.Begin())->GetObject<MobilityModel> ();

  // Durations of the different spreading factors
  // TODO: Compute this via the channel
  // double durations[6] = {0.07808, 0.139776, 0.246784, 0.493568, 0.856064, 1.712128};
  double durations[6] = {0.051456, 0.102912, 0.185344, 0.329728, 0.659456, 1.31891};

  if (prune)
  {
    // Cycle over spreading factors
    for (uint8_t sf = 7; sf <= 12; ++sf)
    {
      NS_LOG_DEBUG ("Spreading factor " << unsigned(sf));
      // Is exit condition met?
      bool exit = false;
      // Cycle over various radius sizes
      int r = 0;
      for (r = 10; r < radius && exit==false; r+=10)
      {
        // Initialize energy variables
        double insideEnergy = 0;
        double outsideEnergy = 0;

        // Cycle on devices
        for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
        {
          Ptr<Node> object = *j;
          Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
          NS_ASSERT (position != 0);
          Ptr<NetDevice> netDevice = object->GetDevice (0);
          Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
          NS_ASSERT (loraNetDevice != 0);
          Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac () -> GetObject<EndDeviceLoraMac> ();
          Ptr<EndDeviceLoraPhy> phy = loraNetDevice->GetPhy () -> GetObject<EndDeviceLoraPhy> ();
          NS_ASSERT (mac != 0);
          NS_ASSERT (phy != 0);
          // Make sure the current device is using the current spreading factor
          // NS_LOG_DEBUG ("This device's sf: " << unsigned(mac->GetSpreadingFactor ()));
          if (mac->GetSpreadingFactor () == sf)
          {
            // Get the device's energy at the center (rcv_power*duration)
            double txPower = phy -> GetTxPowerdBm ();
            // NS_LOG_DEBUG ("Tx Power: " << txPower);
            double rcvPwr = channel -> GetRxPower (txPower, position, center);
            // NS_LOG_DEBUG ("Rx Power: " << rcvPwr);
            rcvPwr = pow(10,rcvPwr/10);
            double distanceFromCenter = position->GetDistanceFrom (center);
            // NS_LOG_DEBUG ("Distance from center: " << distanceFromCenter);
            // If it's inside the radius, sum it to the internal energy total
            if (distanceFromCenter < r)
            {
              insideEnergy += rcvPwr * durations[int(sf)-7];
            }
            // If it's outside the radius, sum its energy to the external energy total
            else
            {
              outsideEnergy += rcvPwr * durations[int(sf)-7];
            }
          }
        }
        // Print
        // NS_LOG_DEBUG ("Energy inside/outside: " << insideEnergy << "/" << outsideEnergy);
        // Is the requirement met? If so, exit.
        if (outsideEnergy < insideEnergy/10)
        {
          NS_LOG_DEBUG ("Requirement met at r = " << r);
          exit = true;
        }
      }
      // Create a new container that only contains devices inside the circle
      for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
      {
        Ptr<Node> object = *j;
        Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
        NS_ASSERT (position != 0);
        Ptr<NetDevice> netDevice = object->GetDevice (0);
        Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
        NS_ASSERT (loraNetDevice != 0);
        Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac () -> GetObject<EndDeviceLoraMac> ();
        Ptr<EndDeviceLoraPhy> phy = loraNetDevice->GetPhy () -> GetObject<EndDeviceLoraPhy> ();
        NS_ASSERT (mac != 0);
        NS_ASSERT (phy != 0);
        double txPower = phy -> GetTxPowerdBm ();
        double rcvPwr = channel -> GetRxPower (txPower, position, center);
        // NS_LOG_DEBUG ("Rx Power: " << rcvPwr);
        rcvPwr = pow(10,rcvPwr/10);
        // Make sure the current device is using the current spreading factor
        if (mac->GetSpreadingFactor () == sf)
        {
          double distanceFromCenter = position->GetDistanceFrom (center);
          // If it's inside the radius, add it to the container
          if (distanceFromCenter < std::max(double(r),gatewayRadius))
          {
            fewerDevices.Add (*j);
          }
          // Detach pruned devices from the channel
          // else // Actually, detach every device. Only leave gateways attached.
          {
            // channel -> Remove (phy);
          }
        }
      }
    }
  }
  else
  {
    fewerDevices = endDevices;
  }

  // SetSpreadingFactorsUp (fewerDevices, gateways, channel);

  /*********************
  *  Print positions  *
  *********************/
  if (printSpreadingFactors)
  {
    PrintSpreadingFactors (fewerDevices, gateways, "fewerDevices.txt");
  }

  /*********************************************
  *  Install applications on the end devices  *
  *********************************************/

  Time appStopTime = Seconds (simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds(appPeriodSeconds));
  ApplicationContainer appContainer = appHelper.Install (fewerDevices);

  appContainer.Start(Seconds(0));
  appContainer.Stop(appStopTime);

  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (appStopTime + Hours (2));

  // Print the current time
  PrintSimulationTime();

  Simulator::Run ();

  Simulator::Destroy ();

  /*************
  *  Results  *
  *************/

  // int packetCounter = 0;
  // for (int i = 0; i < 6; ++i)
  // {
    // // NS_LOG_DEBUG (i+7 << ":" << spreadingFactorCounters.at (i) << "/" << sentSpreadingFactorCounters.at (i));
    // std::cout << double(centralGwSpreadingFactorCounters.at (i))/double(sentSpreadingFactorCounters.at (i)) << " ";
    // packetCounter += sentSpreadingFactorCounters.at (i);
  // }

  // double lostPacketsBecauseInterference = 0;
  // for (int i = 0; i < 6; ++i)
  // {
    // lostPacketsBecauseInterference += spreadingFactorLossCounters.at (i);
  // }

  // for (int i = 0; i < 6; ++i)
  // {
    // // NS_LOG_DEBUG ("Lost " << spreadingFactorLossCounters.at (i) << " packets because of SF = " << i+7);
    // std::cout << double(spreadingFactorLossCounters.at (i))/lostPacketsBecauseInterference<< " ";
  // }

  // Arrived packets = ((packets that were not heard or for which there was no
  // room) + packets that were scheduled for reception)
  // NS_ASSERT (rxBeginEvents == ((underSensitivityPackets + noMoreReceiversPackets) + rxEndEvents));

  // NS_LOG_DEBUG ("Collectively received " << packetCounter << " packets out of a total of " << totalPackets << " sent packets");
  // std::cout << "Collectively received " << packetCounter << " packets out of a total of " << totalPackets << " sent packets" << std::endl;

  // NS_LOG_DEBUG (interferedPackets << " packets were marked as lost because of interference");
  // std::cout << interferedPackets << " packets were marked as lost because of interference" << std::endl;

  // NS_LOG_DEBUG (noMoreReceiversPackets << " packets were marked as lost because no more receive paths were available");
  // std::cout << noMoreReceiversPackets << " packets were marked as lost because no more receive paths were available" << std::endl;

  // NS_LOG_DEBUG (centralGwUnderSensitivityPackets << " packets were marked as lost because they arrived with a power under the receiver sensitivity");
  // std::cout << underSensitivityPackets << " packets were marked as lost because they arrived with a power under the receiver sensitivity" << std::endl;

  // NS_LOG_DEBUG (dutyCycleLimitations << " packets could not be sent because of duty cycle limitations");
  // std::cout << dutyCycleLimitations << " packets could not be sent because of duty cycle limitations" << std::endl;

  // NS_LOG_DEBUG (packetTracker.size () << " packets were completely lost");
  // std::cout << packetTracker.size () << " packets were completely lost" << std::endl;

  // NS_LOG_DEBUG ("Probability of success of a single packet: " << double(totalPackets - packetTracker.size ()) / double(totalPackets));
  // std::cout << "Probability of success of a single packet: " << double(totalPackets - packetTracker.size ()) / double(totalPackets) << std::endl;

  // std::cout << "Finished simulation with " << nDevices << " nodes" << std::endl;

  // std::cout << nDevices << " " <<
                // nGateways << " " <<
                // double(totalPackets - packetTracker.size ()) / double(totalPackets) << " " << // Probability of success of a packet
                // double(noMoreReceiversPackets)/double(totalPackets) << " " << // Fraction of packets that was not received because of too low power
                // double(interferedPackets)/double(totalPackets) << " " <<  // Fraction of packets lost due to interference
                // double (centralGwTotalPackets - centralGwPacketTracker.size ()) / double (centralGwRxBeginEvents - centralGwUnderSensitivityPackets) << " " <<
                // double(centralGwNoMoreReceiversPackets)/double(centralGwRxBeginEvents - centralGwUnderSensitivityPackets) << " " << // Fraction of packets that was not received because of too low power
                // double(centralGwInterferedPackets)/double(centralGwRxBeginEvents - centralGwUnderSensitivityPackets) << " " <<  // Fraction of packets lost due to interference
                // std::endl;

  /*************************************************************
  *  Used for throughput computations, with only one gateway  *
  *************************************************************/
  double g = 0;
  double S = 0;
  int totalSent = 0;
  for (int i = 0; i < 6; i++)
  {
    double sent = sentSpreadingFactorCounters.at(i);
    totalSent += sent;
    double received = centralGwSpreadingFactorCounters.at(i);
    // std::cout << sent << " " << received << std::endl;
    double pFail = double(sent - received) / double(sent);
    if (std::isnan(pFail))
    {
      pFail = 1;
    }
    // std::cout << pFail << std::endl;
    double currentG = double(nodesWithSpreadingFactor.at(i))/double(appPeriodSeconds)*durations[i];
    g += currentG;
    S += currentG*(double(1)-pFail);
    std::cout << "pSucc SF " << i+7 << " = " << double(1)-pFail << std::endl;
  }
  // std::cout << //appPeriodSeconds << " " <<
  //               // pSucc << " " <<
  //               g << " " <<
  //               S << std::endl;

  // Print number of sent packets
  std::cout << totalSent << " packets sent." << std::endl;

  /********************************************************************************
  *  Used for Psucc computations, only using the metrics regarding the inner gw  *
  ********************************************************************************/

  // std::cout <<
                // centralGwDevices << " " <<
                // double(centralGwSuccessfulPackets)/double(centralGwTotalPackets) << " " << // Probability of success
                // double(centralGwInterferedPackets)/double(centralGwTotalPackets) << " " <<  // Fraction of packets lost due to interference
                // double(centralGwNoMoreReceiversPackets)/double(centralGwTotalPackets) << " " << // Fraction of packets that was not received because of lack of receiver paths
                // double(centralGwUnderSensitivityPackets)/double(centralGwTotalPackets) << " "; // Fraction of packets that was not received because of packet under sensitivity


  // std::cout << "| ";
  // for (int i = 0; i < 6; ++i)
  // {
    // // NS_LOG_DEBUG (i+7 << ":" << spreadingFactorCounters.at (i) << "/" << sentSpreadingFactorCounters.at (i));
    // // std::cout << centralGwSpreadingFactorCounters.at (i) << " " << sentSpreadingFactorCounters.at (i) << " ";
    // if (sentSpreadingFactorCounters.at (i) != 0)
    // {
      // std::cout << double(centralGwSpreadingFactorCounters.at (i))/double(sentSpreadingFactorCounters.at (i)) << " ";
    // }
    // else
    // {
      // std::cout << 0 << " ";
    // }
  // }
  // std::cout << "| ";

  // for (int i = 0; i < 6; ++i)
  // {
    // // std::cout << spreadingFactorLossCounters.at(i) << " " << centralGwUnderSensitivityPackets << " ";
    // if (centralGwInterferedPackets != 0 && interferedCounters.at(i) !=0)
    // {
      // std::cout << double(interferedCounters.at(i))/double(sentSpreadingFactorCounters.at(i)) << " ";
    // }
    // else
    // {
      // std::cout << 0 << " ";
    // }
  // }

  // std::cout << "| ";

  // for (int i = 0; i < 6; ++i)
  // {
    // // std::cout << spreadingFactorLossCounters.at(i) << " " << centralGwUnderSensitivityPackets << " ";
    // if (centralGwUnderSensitivityPackets != 0 && underSensitivityCounters.at(i) !=0)
    // {
      // std::cout << double(underSensitivityCounters.at(i))/double(sentSpreadingFactorCounters.at(i)) << " ";
    // }
    // else
    // {
      // std::cout << 0 << " ";
    // }
  // }
  // std::cout << "| ";

  // for (int i = 0; i < 6; ++i)
  // {
    // // std::cout << spreadingFactorLossCounters.at(i) << " " << centralGwInterferedPackets << " ";
    // if (centralGwInterferedPackets != 0 && spreadingFactorLossCounters.at(i) !=0)
    // {
      // std::cout << double(spreadingFactorLossCounters.at(i))/double(totalSentSpreadingFactorCounters.at(i)) << " ";
    // }
    // else
    // {
      // std::cout << 0 << " ";
    // }
  // }
  // std::cout << std::endl;
  // std::cout << // nDevices << " " <<
                // nGateways << " " <<
                // double(totalPackets - packetTracker.size ()) / double(totalPackets) << " " << // Probability of success of a packet
                // double(noMoreReceiversPackets)/double(totalPackets) << " " << // Fraction of packets that was not received because of no more receivers
                // double(interferedPackets)/double(totalPackets) << " " <<  // Fraction of packets lost due to interference
                // double (centralGwTotalPackets - centralGwPacketTracker.size ()) / double (centralGwRxBeginEvents - centralGwUnderSensitivityPackets) << " " <<
                // double(centralGwNoMoreReceiversPackets)/double(centralGwRxBeginEvents - centralGwUnderSensitivityPackets) << " " << // Fraction of packets that was not received because of lack of receiver paths
                // double(centralGwInterferedPackets)/double(centralGwRxBeginEvents - centralGwUnderSensitivityPackets) << " " <<  // Fraction of packets lost due to interference
                // std::endl;

  // Reasons for losing a packet:
  // - Under sensitivity (no rxEndEvent scheduled)
  // - No more receivers (no rxEndEvent scheduled)
  // - Interference (rxEndEvent is scheduled)
  // Packet count: rxBeginEvents == underSensitivityPackets + noMoreReceiversPackets + rxEndEvents
  //                rxBeginEvents == underSensitivityPackets + noMoreReceiversPackets + interferedPackets + successfulPackets
  //                rxBeginEvents - underSensitivityPackets = # of packets that were lost due to interference or receivers number

  return 0;
}
