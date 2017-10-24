/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */

#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/position-allocator.h"
#include "ns3/one-shot-sender-helper.h"
#include "ns3/command-line.h"
#include <algorithm>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleLorawanNetworkExample");

int main (int argc, char *argv[])
{

  // Set up logging
  LogComponentEnable ("SimpleLorawanNetworkExample", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
  LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("GatewayLoraPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraMac", LOG_LEVEL_ALL);
  LogComponentEnable ("EndDeviceLoraMac", LOG_LEVEL_ALL);
  LogComponentEnable ("GatewayLoraMac", LOG_LEVEL_ALL);
  LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraMacHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("OneShotSenderHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("OneShotSender", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraMacHeader", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraFrameHeader", LOG_LEVEL_ALL);
  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  /************************
  *  Create the channel  *
  ************************/

  NS_LOG_INFO ("Creating the channel...");

  // Create the lora channel object
  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
  loss->SetPathLossExponent (3.76);
  loss->SetReference (1, 8.1);  //sets: at distance 1 the path loss must be 8.1

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

  /************************
  *  Create the helpers  *
  ************************/

  NS_LOG_INFO ("Setting up helpers...");

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  allocator->Add (Vector (50,0,0));
  allocator->Add (Vector (0,0,0));
  mobility.SetPositionAllocator (allocator);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

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

  NS_LOG_INFO ("1- Creating the end device...");

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (2);

  // Assign a mobility model to the node
  mobility.Install (endDevices);

  // Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  helper.Install (phyHelper, macHelper, endDevices);

  
  uint32_t id= endDevices.Get(0)->GetId();
  Vector pos= endDevices.Get(0)->GetObject<MobilityModel>()->GetPosition();

  NS_LOG_INFO ("1- End device id: " << id);
  NS_LOG_INFO ("1- End device position: " << pos);

  /* come vedere la posizione del nodo in questo momento?? */

  NS_LOG_INFO ("1- End device created with PHY, MAC, mobility model. \n ");

  /*********************
  *  Create Gateways  *
  *********************/

  NS_LOG_INFO ("2- Creating the gateway (one)...");
  NodeContainer gateways;
  gateways.Create (1);

  //mobility.SetPositionAllocator (allocator); /**** questa non serve perchè era già stato settato?*/
  mobility.Install (gateways);

  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  NS_LOG_INFO ("2- Gateway created with PHY, MAC, mobility model. \n ");


  /***************************************
  *  Set DataRate according to rx power  *
  ****************************************/

  macHelper.SetSpreadingFactorsUp(endDevices, gateways, channel);


  /*********************************************
  *  Install applications on the end devices   *
  **********************************************/

  OneShotSenderHelper oneShotSenderHelper;
  oneShotSenderHelper.SetSendTime (Seconds (10));
  OneShotSenderHelper oneShotSenderHelper2;
  oneShotSenderHelper2.SetSendTime (Seconds (18));

  oneShotSenderHelper.Install (endDevices);
  oneShotSenderHelper2.Install (endDevices);



  /*******************
  *    Downlink tx   *     //make a tx from GW to ED (will be used for setting params)
  ********************/
 
  //Set ED's address
  LoraDeviceAddress addr= LoraDeviceAddress(123);     // Create the address
  Ptr<LoraMac> edMac= endDevices.Get(0)->GetDevice(0)->GetObject<LoraNetDevice>()->GetMac();
  Ptr<EndDeviceLoraMac> edLoraMac = edMac->GetObject<EndDeviceLoraMac>();
  edLoraMac-> SetDeviceAddress(addr);

  
  Ptr<LoraPhy> gwPhy = gateways.Get(0)->GetDevice(0)->GetObject<LoraNetDevice>()->GetPhy();
  
  NS_LOG_INFO ("\n Creating Packet for Downlink transmission...");

  Ptr<Packet> reply= Create<Packet>(5);

  LoraFrameHeader frameHdr;
  frameHdr.SetAsDownlink();
  frameHdr.SetAddress(addr);    // indirizzo ED dst
  frameHdr.SetAdr(true);        // ADR flag
  //frameHdr.SetFPort(0);       //FPort=0 when there are only MAC commands It is 0 by default
  frameHdr.AddLinkAdrReq(0, 0, std::list<int>(1,1), 1);
  reply->AddHeader(frameHdr);
  NS_LOG_INFO ("Added frame header of size " << frameHdr.GetSerializedSize () <<
                   " bytes");

  LoraMacHeader macHdr;
  macHdr.SetMType(LoraMacHeader::UNCONFIRMED_DATA_DOWN);

  reply->AddHeader(macHdr);

  NS_LOG_INFO ("\n Setting parameters for Downlink Transmission...");

  LoraTxParameters params;
  params.sf = 7;
  params.headerDisabled = 1;
  params.codingRate = 1;
  params.bandwidthHz =  125000; //125 kHz
  params.nPreamble = 8;
  params.crcEnabled = 1;
  params.lowDataRateOptimizationEnabled = 0;


  //gwPhy.Send(reply, params, 868.3, 27); 
  Simulator::Schedule(Seconds(11.1), &LoraPhy::Send, gwPhy, reply, params, 868.3, 27);


  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (Hours (1));

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}
