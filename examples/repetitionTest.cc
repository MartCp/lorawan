/*
 * This script simulates a simple network in which one end device sends two
 * confirmed packets to the gateway
 * In the first case, the gateeay does not answer with an acknowledgment and the
 * end device keeps retransmittng until it reaches the maximum number of transmissions
 * allowed (default is 8).
 * Then, a packet with carrying an acknowledgment is manually created and sent by the gateway
 * to the end device.
 * In the second case, the gateway answers to the end device after the second transmisson
 * attempt, in the second receive window. Since the ACK is received, the end device does
 * not perform further retransmissions. 

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

NS_LOG_COMPONENT_DEFINE ("SimpleMacCommandExample");

int main (int argc, char *argv[])
{

  // Set up logging
  LogComponentEnable ("SimpleMacCommandExample", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
  LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("GatewayLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraMac", LOG_LEVEL_ALL);
  LogComponentEnable ("EndDeviceLoraMac", LOG_LEVEL_ALL);
  LogComponentEnable ("GatewayLoraMac", LOG_LEVEL_ALL);
  LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraMacHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("OneShotSenderHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("OneShotSender", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraMacHeader", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraFrameHeader", LOG_LEVEL_ALL);
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
  loss->SetReference (1, 8.1);

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);


  /************************
  *  Create the helpers  *
  ************************/

  NS_LOG_INFO ("Setting up helpers...");

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  // Position of the end device
  allocator->Add (Vector (500,0,0)); 
  // Position of the gateway 
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

  NS_LOG_INFO ("Creating the end device...");

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (1);

  // Assign a mobility model to the node
  mobility.Install (endDevices);

  // Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  helper.Install (phyHelper, macHelper, endDevices);

  
  uint32_t id= endDevices.Get(0)->GetId();
  Vector pos= endDevices.Get(0)->GetObject<MobilityModel>()->GetPosition();

  NS_LOG_DEBUG ("End device id: " << id);
  NS_LOG_DEBUG ("End device position: " << pos);

  NS_LOG_DEBUG ("End device successfully created with PHY, MAC, mobility model. \n ");


  /*********************
  *  Create Gateways  *
  *********************/

  NS_LOG_INFO ("Creating the gateway...");
  NodeContainer gateways;
  gateways.Create (1);

  mobility.Install (gateways);

  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  NS_LOG_DEBUG ("Gateway successfully created with PHY, MAC, mobility model. \n ");


  /***************************************
  *  Set DataRate according to rx power  *
  ****************************************/
  std::vector<int> sfQuantity (6);
  sfQuantity = macHelper.SetSpreadingFactorsUp(endDevices, gateways, channel);



/***************************************************************************************************************************************
***************************************************************************************************************************************/

/*******************************
*   Building uplink packet  *    
*******************************/

 // double frequency= 868.3; // set the frequency of uplink and downlink packets


 // Creating packet for downlink transmission
  NS_LOG_INFO ("Creating First Packet for Uplink transmission...");

  // Setting ED's address
  LoraDeviceAddress addr= LoraDeviceAddress(2311);   
  Ptr<LoraMac> edMac= endDevices.Get(0)->GetDevice(0)->GetObject<LoraNetDevice>()->GetMac();
  Ptr<EndDeviceLoraMac> edLoraMac = edMac->GetObject<EndDeviceLoraMac>();
  edLoraMac-> SetDeviceAddress(addr);
  edLoraMac->SetMType(LoraMacHeader::CONFIRMED_DATA_UP);  // this device will send packets requiring Ack

  Ptr<Packet> pkt1= Create<Packet>(5);

  Simulator::Schedule(Seconds(2), &LoraMac::Send, edMac, pkt1);

  NS_LOG_DEBUG ("Sent first confirmed packet");


// Second packet

  Ptr<Packet> pkt2= Create<Packet>(8);
  Simulator::Schedule(Seconds(62.8), &LoraMac::Send, edMac, pkt2);
  NS_LOG_DEBUG (" Sent second confirmed packet ");

  /*******************************
   *   Building downlink packet  *    
   *******************************/

  // Creating packet for downlink transmussion
  NS_LOG_INFO ("Creating Packet for Downlink transmission...");

  Ptr<Packet> reply= Create<Packet>(5);


  // Setting frame header
  LoraFrameHeader downframeHdr;
  downframeHdr.SetAsDownlink();
  downframeHdr.SetAddress(addr);    // indirizzo ED dst
  downframeHdr.SetAck(true);
  //frameHdr.SetFPort(0);       // FPort=0 when there are only MAC commands. 
                                // This instruction not necessary because it is 0 by default
  reply->AddHeader(downframeHdr);
  NS_LOG_INFO ("Added frame header of size " << downframeHdr.GetSerializedSize () << " bytes");


  // Setting Mac header
  LoraMacHeader downmacHdr;
  downmacHdr.SetMType(LoraMacHeader::UNCONFIRMED_DATA_DOWN);
  reply->AddHeader(downmacHdr);

  NS_LOG_INFO ("\n Setting parameters for Downlink Transmission...");

  // The spreading factor has been set manually, looking at the results of the previous transmision
  LoraTxParameters downparams;
  downparams.sf = 12;
  downparams.headerDisabled = 1;
  downparams.codingRate = 1;
  downparams.bandwidthHz =  125000; 
  downparams.nPreamble = 8;
  downparams.crcEnabled = 1;
  downparams.lowDataRateOptimizationEnabled = 0;


  Ptr<LoraPhy> gwPhy = gateways.Get(0)->GetDevice(0)->GetObject<LoraNetDevice>()->GetPhy();

  // The end device open its first receive window 1 second after the transmission.
  // Scheduling sending of the reply packet after and giving the inputs for function "Send", The frequency has
  // been set looking at the frequency of the previous uplink transmission.
  // Simulator::Schedule(Seconds(63.8), &LoraPhy::Send, gwPhy, reply, downparams, 869.525, 27); // 2nd rx window: freq= 869.525 MHz, SF=12


  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (Hours (1));

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}
