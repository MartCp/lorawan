#include "ns3/point-to-point-module.h"
#include "ns3/forwarder-helper.h"
#include "ns3/network-server-helper.h"
#include "ns3/lora-channel.h"
#include "ns3/mobility-helper.h"
#include "ns3/lora-phy-helper.h"
#include "ns3/lora-mac-helper.h"
#include "ns3/lora-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/periodic-sender.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/lora-device-address-generator.h"
#include "ns3/one-shot-sender-helper.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"

#include "ns3/end-device-lora-phy.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/node-container.h"
#include "ns3/position-allocator.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include <algorithm>

#include "ns3/okumura-hata-propagation-loss-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CompleteNetworkPerformances");

// Network settings
int nDevices = 20;
int gatewayRings = 1;
int nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
double radius = 6300;
double gatewayRadius = 7500/((gatewayRings-1)*2+1);
double simulationTime = 600;
int appPeriodSeconds = 600;
int periodsToSimulate = 1;
int transientPeriods = 0;
std::vector<int> sfQuantity (6);

int noMoreReceivers = 0;
int interfered = 0;
int received = 0;
int underSensitivity = 0;
int lostBecauseTx = 0;
int totalPktsSent = 0;

// RetransmissionParameters
int maxNumbTx = 8;
bool DRAdapt = false;
bool mixedPeriods = false;
uint8_t maxPacketSize = 0;

// Output control
bool print = false;
bool buildingsEnabled = false;
bool shadowingEnabled = false;

// Improvements
bool propAckToImprovement = false;
bool subBandPriorityImprovement = false;
bool secRWDataRateImprovement = false;
bool doubleAck = false;
int maxReceptionPaths = 8;
bool txPriority = true;

/**********************
 *  Global Callbacks  *
 **********************/


int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("gatewayRings", "Number of gateway rings to include", gatewayRings);
  cmd.AddValue ("radius", "The radius of the area to simulate", radius);
  cmd.AddValue ("gatewayRadius", "The distance between two gateways", gatewayRadius);
  // cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue ("appPeriod", "The period in seconds to be used by periodically transmitting applications", appPeriodSeconds);
  cmd.AddValue ("periodsToSimulate", "The number of application periods to simulate", periodsToSimulate);
  cmd.AddValue ("transientPeriods", "The number of periods we consider as a transient", transientPeriods);
  cmd.AddValue ("maxNumbTx", "The maximum number of transmissions allowed.", maxNumbTx);
  cmd.AddValue ("DRAdapt", "Enable data rate adaptation", DRAdapt);
  cmd.AddValue ("mixedPeriods", "Enable mixed application periods", mixedPeriods);
  cmd.AddValue ("print", "Whether or not to print a file containing the ED's positions and a file containing buildings", print);
  cmd.AddValue("maxPacketSize", "The maximum of the RV selecting the value added to the default packet size (10)",maxPacketSize);
  cmd.AddValue("maxReceptionPaths", "The number of gateway's reception paths",maxReceptionPaths);
  cmd.AddValue("buildingsEnabled", "Whether to use buildings in the simulation or not", buildingsEnabled);
  cmd.AddValue("shadowingEnabled", "Whether to use shadowing in the simulation or not", shadowingEnabled);
  cmd.AddValue("propAckToImprovement", "Whether to activate proportional ACK TO improvement", propAckToImprovement);
  cmd.AddValue("subBandPriorityImprovement", "Whether to acrivate subBand priority improvement",subBandPriorityImprovement);
  cmd.AddValue("RW2DataRateImprovement", "Whether to activate RW2 data rate improvement",secRWDataRateImprovement);
  cmd.AddValue("doubleAckImprovement", "Whether to send acks both in RX1 and RX2 when DC allows it",doubleAck);
  cmd.AddValue("txPriority", "Whether to give priority to downlink transmission over reception at the gateways (true)",txPriority);

  cmd.Parse (argc, argv);

  // Set up logging
  LogComponentEnable ("CompleteNetworkPerformances", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraPacketTracker", LOG_LEVEL_ERROR);
  // LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraMac", LOG_LEVEL_ALL);
  // LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraMacHelper", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraMacHeader", LOG_LEVEL_ALL);
  // LogComponentEnable("MacCommand", LOG_LEVEL_ALL);
  // LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("LoraChannel", LOG_LEVEL_ALL);
  // LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable ("SimpleEndDeviceLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("EndDeviceLoraMac", LOG_LEVEL_ALL);
  // LogComponentEnable("PointToPointNetDevice", LOG_LEVEL_ALL);
  // LogComponentEnable("PeriodicSenderHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("PeriodicSender", LOG_LEVEL_ALL);
  // LogComponentEnable ("SimpleNetworkServer", LOG_LEVEL_ALL);
  // LogComponentEnable ("GatewayLoraMac", LOG_LEVEL_ALL);
  // LogComponentEnable ("Forwarder", LOG_LEVEL_ALL);
  // LogComponentEnable ("DeviceStatus", LOG_LEVEL_ALL);
  // LogComponentEnable ("GatewayStatus", LOG_LEVEL_ALL);

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  NS_LOG_INFO ("Setting up network...");

  /***********
   *  Setup  *
   ***********/

  // Compute the number of gateways
  nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;

  // Create the time value from the period
  Time appPeriod = Seconds (appPeriodSeconds);

  // Mobility
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                 "rho", DoubleValue (radius),
                                 "X", DoubleValue (0.0),
                                 "Y", DoubleValue (0.0));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  /************************
   *  Create the channel  *
   ************************/

  // Create the lora channel object
  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
  loss->SetPathLossExponent (3.76);
  loss->SetReference (1, 8.1);

  if(shadowingEnabled)
    {
      // Create the correlated shadowing component
      Ptr<CorrelatedShadowingPropagationLossModel> shadowing = CreateObject<CorrelatedShadowingPropagationLossModel> ();

      // Aggregate shadowing to the logdistance loss
      loss->SetNext(shadowing);

      // Add the effect to the channel propagation loss
      Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss> ();

      shadowing->SetNext(buildingLoss);
    }

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

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
  helper.EnablePacketTracking ("performance"); // Output filename
  // helper.EnableSimulationTimePrinting ();

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

  // Create a LoraDeviceAddressGenerator
  uint8_t nwkId = 54;
  uint32_t nwkAddr = 1864;
  Ptr<LoraDeviceAddressGenerator> addrGen = CreateObject<LoraDeviceAddressGenerator> (nwkId,nwkAddr);

  // Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  macHelper.SetAddressGenerator (addrGen);
  helper.Install (phyHelper, macHelper, endDevices);

  // Now end devices are connected to the channel

  // Configure improvements
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j)
    {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();

      Ptr<EndDeviceLoraMac> mac= loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac>();

      // Set message type, otherwise the NS does not send ACKs
      mac->SetMType (LoraMacHeader::CONFIRMED_DATA_UP);
      mac-> SetMaxNumberOfTransmissions(maxNumbTx);
      mac->SetDataRateAdaptation(DRAdapt);

      // Setting proposed improvements
      mac-> SetProportionalAckToImprovement(propAckToImprovement);
      mac-> SetSubBandPriorityImprovement (subBandPriorityImprovement);
      mac-> SetSecondReceiveWindowDataRateImprovement (secRWDataRateImprovement);
    }

  /*********************
   *  Create Gateways  *
   *********************/

  // Create the gateway nodes (allocate them uniformely on the disc)
  NodeContainer gateways;
  gateways.Create (nGateways);

  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  // Make it so that nodes are at a certain height > 0
  allocator->Add (Vector (0.0, 0.0, 15.0));
  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);


  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  phyHelper.SetMaxReceptionPaths (maxReceptionPaths);
  phyHelper.SetGatewayTransmissionPriority (txPriority);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  macHelper.SetMaxReceptionPaths (maxReceptionPaths);
  helper.Install (phyHelper, macHelper, gateways);

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
  if (print)
    {
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

    }

  /**********************************************
   *  Set up the end device's spreading factor  *
   **********************************************/

  sfQuantity = macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

  /**********************************************
   *              Create Network Server          *
   **********************************************/

  NodeContainer networkServers;
  networkServers.Create (1);

  // Install the SimpleNetworkServer application on the network server
  NetworkServerHelper networkServerHelper;
  networkServerHelper.SetGateways (gateways);
  networkServerHelper.SetEndDevices (endDevices);
  networkServerHelper.SetSubBandPriorityImprovement(subBandPriorityImprovement);
  networkServerHelper.SetSecondReceiveWindowDataRateImprovement(secRWDataRateImprovement);
  networkServerHelper.SetDoubleAckImprovement(doubleAck);
  networkServerHelper.Install (networkServers);

  // Install the Forwarder application on the gateways
  ForwarderHelper forwarderHelper;
  forwarderHelper.Install (gateways);

  /*********************************************
   *  Install applications on the end devices  *
   *********************************************/

  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();

  if (mixedPeriods)
    {
      appHelper.SetPeriod (Seconds(0));
      // In this case, as application period we take
      // the maximum of the possible application periods, i.e. 1 day
      appPeriodSeconds= (24*60*60);
      appPeriod = Seconds(appPeriodSeconds);
    }
  else
    {
      appHelper.SetPeriod (appPeriod);
    }

  //Set random packet size
  appHelper.SetPacketSize (10);
  if (maxPacketSize > 0)
    {
      Ptr<UniformRandomVariable> randomComponent = CreateObject<UniformRandomVariable> ();
      randomComponent-> SetAttribute ("Min", DoubleValue (0));
      randomComponent-> SetAttribute ("Max", DoubleValue (maxPacketSize));
      appHelper.SetPacketSizeRandomVariable(randomComponent);
    }

  ApplicationContainer appContainer = appHelper.Install (endDevices);

  Time appStopTime = appPeriod * periodsToSimulate;

  appContainer.Start (Seconds (0));

  /**********************
   * Print output files *
   *********************/
  if (print)
    {
      helper.PrintEndDevices (endDevices, gateways,
                              "src/lorawan/examples/endDevices.dat");
    }

  ////////////////
  // Simulation //
  ////////////////

  Simulator::Stop (appStopTime + Hours (1000));

  NS_LOG_INFO ("Running simulation...");
  Simulator::Run ();

  Simulator::Destroy ();

  ///////////////////////////
  // Print results to file //
  ///////////////////////////
  NS_LOG_INFO ("Computing performance metrics...");
  helper.PrintPerformance(transientPeriods * appPeriod, appStopTime);

  return 0;
}
