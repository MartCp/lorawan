#include "ns3/end-device-lora-mac.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/simulator.h"

#include "ns3/log.h"
#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PacketTest");

int main (int argc, char *argv[])
{
  	LogComponentEnable ("PacketTest", LOG_LEVEL_ALL);

	Ptr<Packet> packet1= Create<Packet>(5);
	Ptr<Packet> packet2= Create<Packet>(5);
	Ptr<Packet> packet1bis= packet1;
	Ptr<Packet> packet1copy= packet1 -> Copy();

	NS_LOG_INFO("packet1 == packet2 " << (packet1==packet2));
	NS_LOG_INFO("packet1 == packet1 " << (packet1==packet1));
	NS_LOG_INFO("packet1 == packet1bis " << (packet1==packet1bis));
	NS_LOG_INFO("packet1 == packet1copy " << (packet1==packet1copy));
	NS_LOG_INFO("packet1 size " << (packet1 -> GetSize()));


	LoraFrameHeader frameHdr;
	frameHdr.SetAck(1);
	packet1->AddHeader (frameHdr);
	NS_LOG_INFO("packet1 size after header " << (packet1 -> GetSize() ));

	// What we do
/*	Ptr<Packet> m_packet = 0;
	Ptr<Packet> packet = Create<Packet> (10);

	//prima iterazione
	m_packet= packet;
	NS_LOG_INFO("Assegnazione. m_packet == packet " << (m_packet==packet));

	//aggiungo header
	packet -> AddHeader (frameHdr);
	NS_LOG_INFO("Dopo l'aggiunta dell'header. m_packet == packet " << (m_packet==packet));
*/


	// con la copia

	Ptr<Packet> m_packet= 0;
	Ptr<Packet> packet = Create<Packet> (10);

	m_packet= packet -> Copy();

	NS_LOG_INFO("Assegnazione della copia. m_packet == packet " << (m_packet==packet));

	//aggiungo header
	packet -> AddHeader (frameHdr);
	NS_LOG_INFO("Dopo l'aggiunta dell'header. m_packet size " << (m_packet -> GetSize() ));
	NS_LOG_INFO("Dopo l'aggiunta dell'header. packet size " << (packet -> GetSize() ));


}