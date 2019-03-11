/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * GPSR
 *
 */
#define NS_LOG_APPEND_CONTEXT                                           \
  if (m_ipv4) { std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#include "gpsr.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include <algorithm>
#include <limits>

#define GPSR_LS_GOD 0
#define GPSR_LS_RLS 1
#define GPSR_LS_SLS 2

NS_LOG_COMPONENT_DEFINE ("GpsrRoutingProtocol");

namespace ns3 {
namespace gpsr {



struct DeferredRouteOutputTag : public Tag
{
  /// Positive if output device is fixed in RouteOutput
  uint32_t m_isCallFromL3;

  DeferredRouteOutputTag () : Tag (),
                              m_isCallFromL3 (0)
  {
  }

  static TypeId GetTypeId ()
  {
    static TypeId tid = TypeId ("ns3::gpsr::DeferredRouteOutputTag").SetParent<Tag> ();
    return tid;
  }

  TypeId  GetInstanceTypeId () const
  {
    return GetTypeId ();
  }

  uint32_t GetSerializedSize () const
  {
    return sizeof(uint32_t);
  }

  void  Serialize (TagBuffer i) const
  {
    i.WriteU32 (m_isCallFromL3);
  }

  void  Deserialize (TagBuffer i)
  {
    m_isCallFromL3 = i.ReadU32 ();
  }

  void  Print (std::ostream &os) const
  {
    os << "DeferredRouteOutputTag: m_isCallFromL3 = " << m_isCallFromL3;
  }
};



/********** Miscellaneous constants **********/

/// Maximum allowed jitter.
#define GPSR_MAXJITTER          (HelloInterval.GetSeconds () / 2)
/// Random number between [(-GPSR_MAXJITTER)-GPSR_MAXJITTER] used to jitter HELLO packet transmission.
#define JITTER (Seconds (UniformVariable ().GetValue (-GPSR_MAXJITTER, GPSR_MAXJITTER))) 
#define FIRST_JITTER (Seconds (UniformVariable ().GetValue (0, GPSR_MAXJITTER))) //first Hello can not be in the past, used only on SetIpv4



NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

/// UDP Port for GPSR control traffic, not defined by IANA yet
const uint32_t RoutingProtocol::GPSR_PORT = 666;

RoutingProtocol::RoutingProtocol ()
  : HelloInterval (Seconds (1)),
    MaxQueueLen (64),
    MaxQueueTime (Seconds (30)),
    m_queue (MaxQueueLen, MaxQueueTime),
    HelloIntervalTimer (Timer::CANCEL_ON_DESTROY),
    PerimeterMode (false)
{

  m_neighbors = PositionTable ();
}

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::gpsr::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .AddConstructor<RoutingProtocol> ()
    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&RoutingProtocol::HelloInterval),
                   MakeTimeChecker ())
    .AddAttribute ("LocationServiceName", "Indicates wich Location Service is enabled",
                   EnumValue (GPSR_LS_GOD),
                   MakeEnumAccessor (&RoutingProtocol::LocationServiceName),
                   MakeEnumChecker (GPSR_LS_GOD, "GOD",
                                    GPSR_LS_RLS, "RLS",
                                    GPSR_LS_SLS, "SLS"))
    .AddAttribute ("PerimeterMode ", "Indicates if PerimeterMode is enabled",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RoutingProtocol::PerimeterMode),
                   MakeBooleanChecker ())
  ;
  return tid;
}

RoutingProtocol::~RoutingProtocol ()
{
}

void
RoutingProtocol::DoDispose ()
{
  m_ipv4 = 0;
  Ipv4RoutingProtocol::DoDispose ();
}

Ptr<SlsLocationService>
RoutingProtocol::GetLS ()
{
  return m_locationService;
}

void
RoutingProtocol::SetLS (Ptr<SlsLocationService> locationService)
{
  m_locationService = locationService;
}


bool RoutingProtocol::RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                                  UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                  LocalDeliverCallback lcb, ErrorCallback ecb)
{
	NS_LOG_UNCOND("RouteInput from " << m_ipv4->GetAddress(1,0).GetLocal() << " do "
			<< header.GetSource () << " do pacote " << p->GetUid());
//	Ptr<Packet>  packet = p->Copy();
//	TypeHeader tHeader (GPSRTYPE_HELLO);
//	packet->RemoveHeader (tHeader);

	m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
//
//	NS_LOG_UNCOND("RouteInput " << tHeader.Get());

	NS_LOG_FUNCTION (this << p->GetUid () << header.GetDestination () << idev->GetAddress ());

	NS_LOG_UNCOND("Adicionada entrada da RSU " << m_locationService->GetMRsu());
	m_neighbors.AddEntry(m_locationService->GetMRsu(), m_locationService->GetMPosRsu());
	m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
	m_neighbors.Purge((m_ipv4->GetObject<MobilityModel>())->GetPosition(), m_locationService->GetFunction());
	m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());

	if (m_socketAddresses.empty ())
	{
		NS_LOG_LOGIC ("No gpsr interfaces");
		return false;
	}
	NS_ASSERT (m_ipv4 != 0);
	NS_ASSERT (p != 0);
	// Check if input device supports IP
	NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
	int32_t iif = m_ipv4->GetInterfaceForDevice (idev);
	Ipv4Address dst = header.GetDestination ();
	Ipv4Address origin = header.GetSource ();
	//Ipv4Address myIP = m_ipv4->GetAddress (iif, 0).GetLocal ();


  DeferredRouteOutputTag tag; //FIXME since I have to check if it's in origin for it to work it means I'm not taking some tag out...
  if (p->PeekPacketTag (tag) && IsMyOwnAddress (origin))
    {
	  NS_LOG_UNCOND("tag no routeinput");
      Ptr<Packet> packet = p->Copy (); //FIXME ja estou a abusar de tirar tags
      packet->RemovePacketTag(tag);
      DeferredRouteOutput (packet, header, ucb, ecb);
      return true;
    }

  if (m_ipv4->IsDestinationAddress (dst, iif))
    {

      Ptr<Packet> packet = p->Copy ();
      TypeHeader tHeader (GPSRTYPE_POS);
      packet->RemoveHeader (tHeader);
      if (!tHeader.IsValid ())
        {
          NS_LOG_DEBUG ("GPSR message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Ignored");
          return false;
        }
      
      if (tHeader.Get () == GPSRTYPE_POS)
        {
          PositionHeader phdr;
          packet->RemoveHeader (phdr);
        }


      if (dst != m_ipv4->GetAddress (1, 0).GetBroadcast ())
        {
          NS_LOG_LOGIC ("Unicast local delivery to " << dst << " from " << origin);
      	NS_LOG_UNCOND("AEED: " << origin << " " << dst << " " << "RX Packet " << p->GetUid() << " " << p->GetSize() << " Time " << Simulator::Now());

        }
      else
        {
          NS_LOG_LOGIC ("Broadcast local delivery to " << dst);
        }

      lcb (packet, header, iif);
      return true;
    }


  return Forwarding (p, header, ucb, ecb);
}


void
RoutingProtocol::DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header,
                                      UnicastForwardCallback ucb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << p << header);
  NS_ASSERT (p != 0 && p != Ptr<Packet> ());

  if (m_queue.GetSize () == 0)
    {
      CheckQueueTimer.Cancel ();
      CheckQueueTimer.Schedule (Time ("500ms"));
    }

  QueueEntry newEntry (p, header, ucb, ecb);
  bool result = m_queue.Enqueue (newEntry);

  m_queuedAddresses.insert (m_queuedAddresses.begin (), header.GetDestination ());
  m_queuedAddresses.unique ();

  if (result)
    {
      NS_LOG_LOGIC ("Add packet " << p->GetUid () << " to queue. Protocol " << (uint16_t) header.GetProtocol ());

    }

}

void
RoutingProtocol::CheckQueue ()
{
	NS_LOG_UNCOND("CheckQueue");

  CheckQueueTimer.Cancel ();

  std::list<Ipv4Address> toRemove;

  for (std::list<Ipv4Address>::iterator i = m_queuedAddresses.begin (); i != m_queuedAddresses.end (); ++i)
    {

      if (SendPacketFromQueue (*i))
        {
          //Insert in a list to remove later
          toRemove.insert (toRemove.begin (), *i);
        }
    }

  //remove all that are on the list
  for (std::list<Ipv4Address>::iterator i = toRemove.begin (); i != toRemove.end (); ++i)
    {
      m_queuedAddresses.remove (*i);
    }

  if (!m_queuedAddresses.empty ()) //Only need to schedule if the queue is not empty
    {
      CheckQueueTimer.Schedule (Time ("500ms"));
    }
}

bool
RoutingProtocol::SendPacketFromQueue (Ipv4Address dst)
{
  NS_LOG_UNCOND(this << "SendPacketFromQueue");
  NS_LOG_FUNCTION (this);
  bool recovery = false;
  QueueEntry queueEntry;

  //in case of a broadcast message (not HELLOs that never are queued)
  if(dst == m_ipv4->GetAddress (1, 0).GetBroadcast ()){
	  while(m_queue.Dequeue (dst, queueEntry))
	  {
		  Ptr<Packet> packet = ConstCast<Packet> (queueEntry.GetPacket ());

		  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
		  {
			  Ptr<Socket> socket = j->first;
			  Ipv4InterfaceAddress iface = j->second;
			  if(iface.GetBroadcast()==dst)
				  socket->SendTo (packet, 0, InetSocketAddress (dst, GPSR_PORT));

		  }
	  }
	  return true;
  }

  NS_LOG_UNCOND("FROM QUEUE ");

  m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());

  if (m_locationService->IsInSearch (dst))
  {
	  NS_LOG_UNCOND("ISINSEARCH freom queue");
	  return false;
  }

  if (!m_locationService->HasPosition (dst)) // Location-service stoped looking for the dst
  {
      m_queue.DropPacketWithDst (dst);
      NS_LOG_UNCOND("DROPING Packet from QUEUE");
      NS_LOG_UNCOND ("Location Service did not find dst. Drop packet to " << dst);
      return true;
    }

  NS_LOG_UNCOND("I'm Sending");
  Vector myPos;
  
  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
  myPos.x = MM->GetPosition ().x;
  myPos.y = MM->GetPosition ().y;
  Ipv4Address nextHop;

  if(m_neighbors.isNeighbour (dst))
    {
      nextHop = dst;
    }
  else{
	  Vector dstPos = m_locationService->GetPosition (dst);
	  nextHop = m_neighbors.BestNeighbor (dstPos, myPos);
	  if (nextHop == Ipv4Address::GetZero ())
	  {
		  NS_LOG_LOGIC ("Fallback to recovery-mode. Packets to " << dst);
		  recovery = true;
	  }
	  if(recovery)
	  {

		  Vector Position;
		  Vector previousHop;
		  uint32_t updated;

		  while(m_queue.Dequeue (dst, queueEntry))
		  {
			  Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());
			  UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
			  Ipv4Header header = queueEntry.GetIpv4Header ();

			  TypeHeader tHeader (GPSRTYPE_POS);
			  p->RemoveHeader (tHeader);
			  if (!tHeader.IsValid ())
			  {
				  NS_LOG_DEBUG ("GPSR message " << p->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
				  return false;     // drop
			  }
			  if (tHeader.Get () == GPSRTYPE_POS)
			  {
				  PositionHeader hdr;
				  p->RemoveHeader (hdr);
				  Position.x = hdr.GetDstPosx ();
				  Position.y = hdr.GetDstPosy ();
				  updated = hdr.GetUpdated ();
			  }

			  PositionHeader posHeader (Position.x, Position.y,  updated, myPos.x, myPos.y, (uint8_t) 1, Position.x, Position.y);
			  p->AddHeader (posHeader); //enters in recovery with last edge from Dst
			  p->AddHeader (tHeader);

			  NS_LOG_UNCOND("FROM QUEUE  Recovery pacote " << p->GetUid ());
			  RecoveryMode(dst, p, ucb, header);
		  }
		  return true;
	  }
  }

  Ptr<Ipv4Route> route = Create<Ipv4Route> ();
  route->SetDestination (dst);
  route->SetGateway (nextHop);

  NS_LOG_UNCOND("NextHop is " << nextHop);
  // FIXME: Does not work for multiple interfaces
  route->SetOutputDevice (m_ipv4->GetNetDevice (1));

  while (m_queue.Dequeue (dst, queueEntry))
  {
	  DeferredRouteOutputTag tag;
	  Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());

	  UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
	  Ipv4Header header = queueEntry.GetIpv4Header ();

	  if (header.GetSource () == Ipv4Address ("102.102.102.102"))
	  {
		  route->SetSource (m_ipv4->GetAddress (1, 0).GetLocal ());
		  header.SetSource (m_ipv4->GetAddress (1, 0).GetLocal ());
	  }
	  else
	  {
		  route->SetSource (header.GetSource ());
		  NS_LOG_UNCOND("headerdestination " << header.GetDestination());

	  }

	  NS_LOG_UNCOND("Chega ao fim do SendPacketFromQueue, ja tendo mandado para ucb, pacote " << p->GetUid ());

	  ucb (route, p, header);

	  m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
  }

  return true;
}


void 
RoutingProtocol::RecoveryMode(Ipv4Address dst, Ptr<Packet> p, UnicastForwardCallback ucb, Ipv4Header header){

  NS_LOG_UNCOND(this << "RecoveryMode");
  Vector Position;
  Vector previousHop;
  uint32_t updated;
  uint64_t positionX;
  uint64_t positionY;
  Vector myPos;
  Vector recPos;


  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
  positionX = MM->GetPosition ().x;
  positionY = MM->GetPosition ().y;
  myPos.x = positionX;
  myPos.y = positionY;  

  TypeHeader tHeader (GPSRTYPE_POS);
  p->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("GPSR message " << p->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return;     // drop
    }
  if (tHeader.Get () == GPSRTYPE_POS)
    {
	  NS_LOG_UNCOND("Aqui 4");
      PositionHeader hdr;
      p->RemoveHeader (hdr);
      Position.x = hdr.GetDstPosx ();
      Position.y = hdr.GetDstPosy ();
      updated = hdr.GetUpdated (); 
      recPos.x = hdr.GetRecPosx ();
      recPos.y = hdr.GetRecPosy ();
      previousHop.x = hdr.GetLastPosx ();
      previousHop.y = hdr.GetLastPosy ();
   }

  NS_LOG_UNCOND("Aqui 5");
  PositionHeader posHeader (Position.x, Position.y,  updated, recPos.x, recPos.y, (uint8_t) 1, myPos.x, myPos.y); 
  p->AddHeader (posHeader);
  p->AddHeader (tHeader);


  NS_LOG_UNCOND("previousHop " << previousHop << " myPos " << myPos);

  Vector newPreviousHop =  m_locationService->GODPredict(dst);

  NS_LOG_UNCOND("New PreviousHop " << newPreviousHop << " my pos " << myPos << " myposGod " << m_locationService
		  ->GODPredict(m_ipv4->GetAddress(1,0).GetLocal()));

  Ipv4Address nextHop;

  nextHop = m_neighbors.BestAngle (newPreviousHop, myPos);
  //Ipv4Address nextHop = m_neighbors.BestAngle (previousHop, myPos);


  if (nextHop == Ipv4Address::GetZero ())
    {
      return;
    }

  NS_LOG_UNCOND("Próximo hop " << nextHop);
  Ptr<Ipv4Route> route = Create<Ipv4Route> ();
  route->SetDestination (dst);
  route->SetGateway (nextHop);

  // FIXME: Does not work for multiple interfaces
  route->SetOutputDevice (m_ipv4->GetNetDevice (1));
  route->SetSource (header.GetSource ());

  NS_LOG_UNCOND("NextHop " << nextHop << " Dst " << dst << " src " << header.GetSource() << " " << header.GetDestination());

  ucb (route, p, header);

  NS_LOG_UNCOND("Fez unicast callback");
  return;
}


void
RoutingProtocol::NotifyInterfaceUp (uint32_t interface)
{
	NS_LOG_UNCOND(this << "NotifyInterfaceUp");
	m_interface = interface;
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (interface, 0).GetLocal ());
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (l3->GetNAddresses (interface) > 1)
    {
      NS_LOG_WARN ("GPSR does not work with more then one address per each interface.");
    }
  Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    {
      return;
    }

  // Create a socket to listen only on this interface
  Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                             UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvGPSR, this));
  socket->BindToNetDevice (l3->GetNetDevice (interface));
  socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), GPSR_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetAttribute ("IpTtl", UintegerValue (1));
  m_socketAddresses.insert (std::make_pair (socket, iface));


  // Allow neighbor manager use this interface for layer 2 feedback if possible
  Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
  if (wifi == 0)
    {
      return;
    }
  Ptr<WifiMac> mac = wifi->GetMac ();
  if (mac == 0)
    {
      return;
    }

  mac->TraceConnectWithoutContext ("TxErrHeader", m_neighbors.GetTxErrorCallback ());

}

void
RoutingProtocol::RecvGPSR (Ptr<Socket> socket)
{
  NS_LOG_UNCOND("Time " << Simulator::Now());
  NS_LOG_UNCOND(" RECVGPSR CALLED by " << m_locationService->GetFunction());
  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address sender = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiver = m_socketAddresses[socket].GetLocal ();

  TypeHeader tHeader (GPSRTYPE_HELLO);
  packet->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
  {
	  NS_LOG_DEBUG ("GPSR message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Ignored");
	  return;
  }

  if(tHeader.Get () == SLS_LOCATION_HELLO)
  {
	  //Se eu sou OBU
	  if(m_locationService->GetFunction())
	  {
		  NS_LOG_UNCOND("****** Sou OBU " << receiver << " e recebi Location Hello from " << sender);
		  //Nao precisava de fazer updateroute contudo fazendo fica com posicao mais actual
		  LocHelloHeader lochello;
		  packet->RemoveHeader(lochello);

		  Vector Position;
		  Position.x = lochello.GetOriginPosx ();
		  Position.y = lochello.GetOriginPosy ();

		  //UpdateRouteToNeighbor(sender, receiver, Position);
		  Ipv4Address rsu = m_locationService->GetMRsu();

		  if(sender==rsu)
		  {
			  NS_LOG_UNCOND("Continuo na minha rsu " << rsu);
		  }
		  else
		  {
			 NS_LOG_UNCOND("Sou " << m_ipv4->GetAddress(1,0).GetLocal() << " e vou actualizar m_rsu de " << rsu <<
			 //Se vou actualizar RSU entao tenho de apagar entrada na RSU antiga.
					 " para " << " " << sender << " e m_posrsu " << Position);
			 m_locationService->DeleteEntry(m_ipv4->GetAddress(1,0).GetLocal());
			 m_locationService->SetMRsu(sender);
			 m_locationService->SetMPosRsu(Position);
		  }

		  m_locationService->SendLocUpdate(packet,sender, receiver, Position);
		  return;
	  }
	  //Se eu sou RSU
	  else
	  {
		  NS_LOG_UNCOND("****** Sou RSU" << receiver << " e recebi Location Hello");
	  }

  }

  if(tHeader.Get () == SLS_LOCATION_UPDATE)
  {
	  //Se eu sou OBU
	  if(m_locationService->GetFunction())
	  {
		  NS_LOG_UNCOND("****** Sou OBU " << receiver << " e recebi um SLS Update from " << sender);
	  }
	  //Se eu sou RSU
	  else
	  {
		  NS_LOG_UNCOND("****** Sou RSU e recebi um SLS Update from " << sender);
		  //Nao precisava de fazer updateroute contudo fazendo fica com posicao mais actual
		  LocUpdateHeader locupdate;
		  packet->RemoveHeader(locupdate);

		  Vector Position;
		  Position.x = locupdate.GetOriginPosx ();
		  Position.y = locupdate.GetOriginPosy ();

		  int speed = locupdate.GetSpeed();
		  NS_LOG_UNCOND("SPEED " << speed);
		  UpdateRouteToNeighbor(sender, receiver, Position, speed);
		  m_locationService->ReceiveUpdate(packet,sender, receiver,Position, speed);
		  return;
	  }
  }

  if(tHeader.Get () == SLS_LOCATION_QUERY)
  {
	  NS_LOG_UNCOND("** query");
	  if(m_locationService->GetFunction())
	  {
		  NS_LOG_UNCOND("****** Sou OBU " << receiver << " e recebi um Location Query");
	  }
	  //Se eu sou RSU
	  else
	  {
		  NS_LOG_UNCOND("****** Sou RSU " << receiver << " e recebi um Location Query from " << sender);

		  LocQueryHeader locquery;
		  packet->RemoveHeader(locquery);
		  //m_neighbors.AddEntry(query, Position);
		  NS_LOG_UNCOND("ReceiveQuery " << sender << " " << locquery.GetQueryID() << " " << Simulator::Now() << " " << packet->GetUid());
		  m_locationService->ReceiveQuery(sender, receiver, locquery.GetQueryID());

	  }

	  return;
  }

  if(tHeader.Get () == SLS_LOCATION_REPLY)
    {
  	  //Se eu sou OBU
  	  if(m_locationService->GetFunction())
  	  {
  		  NS_LOG_UNCOND("****** Sou OBU " << receiver << " e recebi um Location Reply from " << sender);

  		  LocReplyHeader locreply;
  		  packet->RemoveHeader(locreply);

  		  Vector Position;
  		  Position.x = locreply.GetPosx();
  		  Position.y = locreply.GetPosy();

  		  NS_LOG_UNCOND(" -> " << locreply.GetDestID()); //E este que quero
  		  NS_LOG_UNCOND(" -> " << locreply.GetNodeID());
  		  NS_LOG_UNCOND("Sender " << sender);
  		  NS_LOG_UNCOND("Receiver " << receiver);

  		  Ipv4Address query = locreply.GetDestID();
  		  //m_neighbors.AddEntry(query, Position);
  		  NS_LOG_UNCOND("ReceiveReply " << receiver << " " << query << " " << Simulator::Now() << " " << packet->GetUid());
  		  m_locationService->ReceiveReply(sender, query, Position);

  	  }
  	  //Se eu sou RSU
  	  else
  	  {
  		 NS_LOG_UNCOND("****** Sou RSU e recebi um Location Reply");
  	  }

    }


//  if(tHeader.Get () == GPSRTYPE_POS)
//  {
//	  //Se eu sou OBU
//	  if(m_locationService->GetFunction())
//	  {
//		  NS_LOG_UNCOND("Sou OBU e recebi um HELLO POSITION");
//	  }
//	  //Se eu sou RSU
//	  else
//	  {
//		  NS_LOG_UNCOND("Sou RSU e recebi um HELLO POSITION");
//	  }
//  }

  else{

	  //HELLO GPSR
	  HelloHeader hdr;
	  packet->RemoveHeader (hdr);

	  Vector Position;
	  Position.x = hdr.GetOriginPosx ();
	  Position.y = hdr.GetOriginPosy ();

	  //Se eu sou OBU
	  if(m_locationService->GetFunction())
	  {
		  NS_LOG_UNCOND("****** Sou OBU " << receiver << " e recebi um Hello Gpsr from " << sender << " mas nao vou fazer nada");
		  UpdateRouteToNeighbor (sender, receiver, Position, 0);

		  return;
	  }
	  //Se eu sou RSU
	  else
	  {
		  NS_LOG_UNCOND("****** Sou RSU " << receiver << " e recebi um Hello Gpsr from " << sender);
		  UpdateRouteToNeighbor (sender, receiver, Position, 0);
		  m_locationService->SendLocHello(sender, receiver, Position);
		  return;
	  }
  }

}


void
RoutingProtocol::UpdateRouteToNeighbor (Ipv4Address sender, Ipv4Address receiver, Vector Pos, int speed)
{
	//FIXME Este update tem de levar com a speed, para que o addentry no LS esteja correcto
	NS_LOG_UNCOND("UpdateRouteToNeighbor " << sender << " " << Pos.x << ":" << Pos.y);
	m_neighbors.AddEntry (sender, Pos);

	if(m_locationService->GetFunction())
	{
		m_locationService->AddEntry(receiver, Pos, speed, false, 0);
	}
	else
	{
		m_locationService->AddEntry(sender, Pos, speed, false, 0);
	}

}



void
RoutingProtocol::NotifyInterfaceDown (uint32_t interface)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (interface, 0).GetLocal ());

  // Disable layer 2 link state monitoring (if possible)
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ptr<NetDevice> dev = l3->GetNetDevice (interface);
  Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice> ();
  if (wifi != 0)
    {
      Ptr<WifiMac> mac = wifi->GetMac ()->GetObject<AdhocWifiMac> ();
      if (mac != 0)
        {
          mac->TraceDisconnectWithoutContext ("TxErrHeader",m_neighbors.GetTxErrorCallback ());
        }
    }

  // Close socket
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_ipv4->GetAddress (interface, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_socketAddresses.erase (socket);
  if (m_socketAddresses.empty ())
    {
      NS_LOG_LOGIC ("No gpsr interfaces");
      m_neighbors.Clear ();
      m_locationService->Clear ();
      return;
    }
}


Ptr<Socket>
RoutingProtocol::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_UNCOND("FindSocketwithInterface");
  NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        {
          return socket;
        }
    }
  Ptr<Socket> socket;
  return socket;
}



void RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
	NS_LOG_UNCOND("NotifyAddAddress");
	m_address = address;
  NS_LOG_FUNCTION (this << " interface " << interface << " address " << address);
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (!l3->IsUp (interface))
    {
      return;
    }
  if (l3->GetNAddresses ((interface) == 1))
    {
      Ipv4InterfaceAddress iface = l3->GetAddress (interface, 0);
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
      if (!socket)
        {
          if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
            {
              return;
            }
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                     UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvGPSR,this));
          socket->BindToNetDevice (l3->GetNetDevice (interface));
          // Bind to any IP address so that broadcasts can be received
          socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), GPSR_PORT));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));

          Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
        }
    }
  else
    {
      NS_LOG_LOGIC ("GPSR does not work with more then one address per each interface. Ignore added address");
    }
}

void
RoutingProtocol::NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address)
{

  NS_LOG_FUNCTION (this);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (address);
  if (socket)
    {

      m_socketAddresses.erase (socket);
      Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
      if (l3->GetNAddresses (i))
        {
          Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                     UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvGPSR, this));
          // Bind to any IP address so that broadcasts can be received
          socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), GPSR_PORT));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));

          // Add local broadcast record to the routing table
          Ptr<NetDevice> dev = m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));

        }
      if (m_socketAddresses.empty ())
        {
          NS_LOG_LOGIC ("No gpsr interfaces");
          m_neighbors.Clear ();
          m_locationService->Clear ();
          return;
        }
    }
  else
    {
      NS_LOG_LOGIC ("Remove address not participating in GPSR operation");
    }
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);

  m_ipv4 = ipv4;

  HelloIntervalTimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
  HelloIntervalTimer.Schedule (FIRST_JITTER);

  //Schedule only when it has packets on queue
  CheckQueueTimer.SetFunction (&RoutingProtocol::CheckQueue, this);

  Simulator::ScheduleNow (&RoutingProtocol::Start, this);
}

void
RoutingProtocol::HelloTimerExpire ()
{
	//TODO - Isto podia estar mais "bonito"
	if(m_address.GetLocal().IsEqual("10.0.0.1")||m_address.GetLocal().IsEqual("10.0.0.2")
			|| m_address.GetLocal().IsEqual("10.0.0.3"))
	{}
	else{
		SendHello ();
		HelloIntervalTimer.Cancel ();
		HelloIntervalTimer.Schedule (HelloInterval + JITTER);
	}
}
void
RoutingProtocol::SendHello ()
{
  NS_LOG_FUNCTION (this);
  double positionX;
  double positionY;

  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
  positionX = MM->GetPosition ().x;
  positionY = MM->GetPosition ().y;

  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      HelloHeader helloHeader (((uint64_t) positionX),((uint64_t) positionY));

      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader (helloHeader);
      TypeHeader tHeader (GPSRTYPE_HELLO);
      packet->AddHeader (tHeader);
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      socket->SendTo (packet, 0, InetSocketAddress (destination, GPSR_PORT));

    }
}

bool
RoutingProtocol::IsMyOwnAddress (Ipv4Address src)
{
	NS_LOG_UNCOND(this << "IsmyOwnAddress");
  NS_LOG_FUNCTION (this << src);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (src == iface.GetLocal ())
        {
          return true;
        }
    }
  return false;
}




void
RoutingProtocol::Start ()
{
  NS_LOG_FUNCTION (this);
  m_queuedAddresses.clear ();

  //FIXME ajustar timer, meter valor parametrizavel
  Time tableTime ("5s");

  LocationServiceName = GPSR_LS_SLS;//FIXME cheat to use sls only

  switch (LocationServiceName)
    {
    case GPSR_LS_GOD:
      NS_LOG_DEBUG (this << "GodLS in use");
      //m_locationService = CreateObject<GodLocationService> ();
      break;
    case GPSR_LS_SLS:
      NS_LOG_UNCOND (this << "SLS in use");
      m_locationService = CreateObject<ns3::gpsr::SlsLocationService> (tableTime);
      m_locationService->SetIpv4(m_ipv4);
      m_locationService->NotifyInterfaceUp(m_interface);
      m_locationService->NotifyAddAddress(m_interface, m_address);

      if(m_address.GetLocal().IsEqual("10.0.0.1")||m_address.GetLocal().IsEqual("10.0.0.2") || m_address.GetLocal().IsEqual("10.0.0.3"))
      {
    	  //RSU
    	  m_locationService->SetFunction(false);
      }
      else
      {
    	  //OBU
    	  m_locationService->SetFunction(true);
      }

      break;
    }
  Vector myPos;
    Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
    myPos.x = MM->GetPosition ().x;
    myPos.y = MM->GetPosition ().y;

    NS_LOG_UNCOND("Initial Position: " << myPos.x << "," << myPos.y);


}

Ptr<Ipv4Route>
RoutingProtocol::LoopbackRoute (const Ipv4Header & hdr, Ptr<NetDevice> oif)
{
	NS_LOG_UNCOND("LoopbackRoute");
  NS_LOG_UNCOND (this << hdr);
  m_lo = m_ipv4->GetNetDevice (0);
  NS_ASSERT (m_lo != 0);
  Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
  rt->SetDestination (hdr.GetDestination ());

  std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j = m_socketAddresses.begin ();
  if (oif)
    {
      // Iterate to find an address on the oif device
      for (j = m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
        {
          Ipv4Address addr = j->second.GetLocal ();
          int32_t interface = m_ipv4->GetInterfaceForAddress (addr);
          if (oif == m_ipv4->GetNetDevice (static_cast<uint32_t> (interface)))
            {
              rt->SetSource (addr);
              break;
            }
        }
    }
  else
    {
      rt->SetSource (j->second.GetLocal ());
    }
  NS_ASSERT_MSG (rt->GetSource () != Ipv4Address (), "Valid GPSR source address not found");
  rt->SetGateway (Ipv4Address ("127.0.0.1"));
  rt->SetOutputDevice (m_lo);
  return rt;
}


int
RoutingProtocol::GetProtocolNumber (void) const
{
  return GPSR_PORT;
}

void
RoutingProtocol::AddHeaders (Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
{
	Vector myPos;
	Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
	myPos.x = MM->GetPosition ().x;
	myPos.y = MM->GetPosition ().y;

	m_neighbors.Purge(myPos, m_locationService->GetFunction());

	NS_LOG_UNCOND("AddHeaderssh ");
	Ptr<Packet> packet = p->Copy ();
	TypeHeader tHeader1 (GPSRTYPE_HELLO);
	packet->RemoveHeader (tHeader1);
	NS_LOG_UNCOND("AddHeaders " << " source " << source << " destination " << destination << " theader " << tHeader1.Get());

  Ipv4Address nextHop;

  Vector position;

  if(destination != m_ipv4->GetAddress (1, 0).GetBroadcast ())
  {
	  //FIXME NOW
	  Vector position3 = m_locationService->GODPredict(destination);
	  position = m_locationService->GetPosition(destination);
	  //Vector position3 = m_locationService->GetPosition (destination);
	  NS_LOG_UNCOND("GodPredict " << position3);
  }
  else
  {

	  position = m_locationService->GetInvalidPosition();
  }

  if(m_neighbors.isNeighbour (destination))
    {

      nextHop = destination;
    }
  else
    {
      nextHop = m_neighbors.BestNeighbor (position, myPos);

    }

  NS_LOG_UNCOND("ADDHEADERS NEXT HOP CHOSEN WAS " << nextHop);

  uint16_t positionX = 0;
  uint16_t positionY = 0;
  uint32_t hdrTime = 0;

  if(destination != m_ipv4->GetAddress (1, 0).GetBroadcast ())
    {
      positionX = position.x;
      positionY = position.y;
      NS_LOG_UNCOND("Fez GetEntryUpdateTime");
      hdrTime = (uint32_t) m_locationService->GetEntryUpdateTime (destination).GetSeconds ();
    }

  PositionHeader posHeader (positionX, positionY,  hdrTime, (uint64_t) 0,(uint64_t) 0, (uint8_t) 0, myPos.x, myPos.y);
  NS_LOG_UNCOND("Source " << source << " Dest " << destination << " "
		  << positionX << " " << positionY << " " << myPos.x << " " << myPos.y);

  p->AddHeader (posHeader);
  TypeHeader tHeader (GPSRTYPE_POS);
  p->AddHeader (tHeader);
  NS_LOG_UNCOND("Adicionou header de posicao");

  m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());

  m_downTarget (p, source, destination, protocol, route);
  NS_LOG_UNCOND("Message Sended from " << source << " to " << destination <<
		  "nexthop " <<  nextHop << " !!" << " I'm " << m_ipv4->GetAddress (1, 0).GetLocal()
		  << " myPosition " << myPos);

  NS_LOG_UNCOND("Real position from dst " << m_locationService->GODPredict(destination));

  m_locationService->Print();
}

bool
RoutingProtocol::Forwarding (Ptr<const Packet> packet, const Ipv4Header & header,
                             UnicastForwardCallback ucb, ErrorCallback ecb)
{

  Ptr<Packet> p = packet->Copy ();
  NS_LOG_FUNCTION (this);
  Ipv4Address dst = header.GetDestination ();
  Ipv4Address origin = header.GetSource ();
  NS_LOG_UNCOND(this << "Forwarding : Origin " << origin << " destination " << dst);

  m_neighbors.Purge ((m_ipv4->GetObject<MobilityModel>())->GetPosition(), m_locationService->GetFunction());
  m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
  
  m_locationService->Print();

  uint32_t updated = 0;
  Vector Position;
  Vector RecPosition;
  uint8_t inRec = 0;

  TypeHeader tHeader (GPSRTYPE_POS);
  PositionHeader hdr;
  p->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("GPSR message " << p->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
      return false;     // drop
    }
  if (tHeader.Get () == GPSRTYPE_POS)
    {

      p->RemoveHeader (hdr);
      Position.x = hdr.GetDstPosx ();
      Position.y = hdr.GetDstPosy ();
      updated = hdr.GetUpdated ();
      RecPosition.x = hdr.GetRecPosx ();
      RecPosition.y = hdr.GetRecPosy ();
      inRec = hdr.GetInRec ();
      NS_LOG_UNCOND("INREC " << inRec);
    }

  Vector myPos;
  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
  myPos.x = MM->GetPosition ().x;
  myPos.y = MM->GetPosition ().y;  

  if(inRec == 1 && CalculateDistance (myPos, Position) < CalculateDistance (RecPosition, Position)){
    inRec = 0;
    hdr.SetInRec(0);
  NS_LOG_UNCOND ("No longer in Recovery to " << dst << " in " << myPos);
  }

  NS_LOG_UNCOND("INREC " << inRec);

  if(inRec){
	  NS_LOG_UNCOND("Entra aqui1");
    p->AddHeader (hdr);
    p->AddHeader (tHeader); //put headers back so that the RecoveryMode is compatible with Forwarding and SendFromQueue
    RecoveryMode (dst, p, ucb, header);
    return true;
  }


  uint32_t myUpdated = (uint32_t) m_locationService->GetEntryUpdateTime (dst).GetSeconds ();
  if (myUpdated > updated) //check if node has an update to the position of destination
    {
	  Position.x = m_locationService->GetPosition (dst).x;
      Position.y = m_locationService->GetPosition (dst).y;
      updated = myUpdated;
      NS_LOG_UNCOND("Update");
    }
  NS_LOG_UNCOND("Entra aqui2");

  Ipv4Address nextHop;

  NS_LOG_UNCOND("Forwarding vai chamar bestNeighbor com destino " << Position);

  if(m_neighbors.isNeighbour (dst))
  {
	  nextHop = dst;
	  NS_LOG_UNCOND("Tem de estar aqui");
  }
  else
  {
	  nextHop = m_neighbors.BestNeighbor (Position, myPos);
  }

  if (nextHop != Ipv4Address::GetZero ())
  {
	  PositionHeader posHeader (Position.x, Position.y,  updated, (uint64_t) 0, (uint64_t) 0, (uint8_t) 0, myPos.x, myPos.y);
	  p->AddHeader (posHeader);
	  p->AddHeader (tHeader);


	  Ptr<NetDevice> oif = m_ipv4->GetObject<NetDevice> ();
	  Ptr<Ipv4Route> route = Create<Ipv4Route> ();
	  route->SetDestination (dst);
	  route->SetSource (header.GetSource ());
	  route->SetGateway (nextHop);

	  // FIXME: Does not work for multiple interfaces
	  route->SetOutputDevice (m_ipv4->GetNetDevice (1));
	  route->SetDestination (header.GetDestination ());
	  NS_ASSERT (route != 0);
	  NS_LOG_UNCOND ("Exist route to " << route->GetDestination () << " from interface " << route->GetOutputDevice ());


	  NS_LOG_UNCOND (route->GetOutputDevice () << " forwarding to " << dst << " from " << origin << " through " << route->GetGateway () << " packet " << p->GetUid ());

	  ucb (route, p, header);
	  return true;
  }

  hdr.SetInRec(1);
  hdr.SetRecPosx (myPos.x);
  hdr.SetRecPosy (myPos.y); 
  hdr.SetLastPosx (Position.x); //when entering Recovery, the first edge is the Dst
  hdr.SetLastPosy (Position.y); 

  NS_LOG_UNCOND("No forwarding esta é a posicao do dst " << dst << " Pos: " <<
  			  Position.x << ":" << Position.y);

  NS_LOG_UNCOND("posicao real do dst e " << m_locationService->GODPredict(dst));

  p->AddHeader (hdr);
  p->AddHeader (tHeader);
  RecoveryMode (dst, p, ucb, header);

  NS_LOG_UNCOND("HDR " << header.GetDestination());

  m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
  NS_LOG_LOGIC ("Entering recovery-mode to " << dst << " in " << m_ipv4->GetAddress (1, 0).GetLocal ());
  return true;
}



void
RoutingProtocol::SetDownTarget (Ipv4L4Protocol::DownTargetCallback callback)
{
	NS_LOG_UNCOND(this << "SetDownTarget");
  m_downTarget = callback;
}


Ipv4L4Protocol::DownTargetCallback
RoutingProtocol::GetDownTarget (void) const
{
	NS_LOG_UNCOND(this << "GetDownTarget");
	return m_downTarget;
}


Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header,
                              Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  Ptr<Packet>  packet = p->Copy();
  TypeHeader tHeader (GPSRTYPE_HELLO);
  packet->RemoveHeader (tHeader);

  //Adiciono o M_rsu à tabela de vizinhos para possibilitar a Rsu ser nexthop, pode ser logo limpa depois
  NS_LOG_UNCOND("Adicionada entrada da RSU " << m_locationService->GetMRsu());
  m_neighbors.AddEntry(m_locationService->GetMRsu(), m_locationService->GetMPosRsu());
  m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
  m_neighbors.Purge((m_ipv4->GetObject<MobilityModel>())->GetPosition(), m_locationService->GetFunction());
  m_neighbors.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());

  if (!p)
  {
	  return LoopbackRoute (header, oif);     // later
  }
  if (m_socketAddresses.empty ())
  {
	  sockerr = Socket::ERROR_NOROUTETOHOST;
	  NS_LOG_LOGIC ("No gpsr interfaces");
	  Ptr<Ipv4Route> route;
	  return route;
  }

  sockerr = Socket::ERROR_NOTERROR;
  Ptr<Ipv4Route> route = Create<Ipv4Route> ();
  Ipv4Address dst = header.GetDestination ();

  NS_LOG_UNCOND("RouteOutput from " << m_ipv4->GetAddress(1,0).GetLocal() << " destination " << dst << " packet " << p->GetUid() << " Time " << Simulator::Now()) ;

  Vector dstPos = m_locationService->GetInvalidPosition();

  if (!(dst == m_ipv4->GetAddress (1, 0).GetBroadcast ()))
    {
	  NS_LOG_UNCOND("GetPosition: from " << m_ipv4->GetAddress (1, 0));
      //dstPos = m_locationService->GetPosition (dst);

	  //FIXME NOW
      Vector dstPos1 = m_locationService->GODPredict(dst);
      dstPos = m_locationService->GetPosition(dst);
      //Vector Position3 = m_locationService->GetPosition(dst);
      NS_LOG_UNCOND("GodPredict" << dstPos1);

      NS_LOG_UNCOND("GetPosition retornou para GPSR pos " << dstPos << " do " << dst);
    }

  if (CalculateDistance (dstPos, m_locationService->GetInvalidPosition ()) == 0 && dst != m_ipv4->GetAddress (1, 0).GetBroadcast () && m_locationService->IsInSearch (dst))
  //if (CalculateDistance (dstPos, m_locationService->GetInvalidPosition ()) == 0 && dst != m_ipv4->GetAddress (1, 0).GetBroadcast ())
  {
	  DeferredRouteOutputTag tag;
	  if (!p->PeekPacketTag (tag))
	  {
		  NS_LOG_UNCOND("Adicionou tag");
		  p->AddPacketTag (tag);
	  }
	  return LoopbackRoute (header, oif);
  }
  else{
	  NS_LOG_UNCOND("GetPosition deu posicao válida e nao estava inSearch");
  }

  NS_LOG_UNCOND("AEED: " << m_ipv4->GetAddress(1,0).GetLocal() << " " << dst << " " << "TX Packet " << p->GetUid() << " " << p->GetSize() << " Time " << Simulator::Now());

  NS_LOG_UNCOND("UID do pacote " << p->GetUid() << " I'm " << m_ipv4->GetAddress(1,0).GetLocal());

  Vector myPos;
  Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
  myPos.x = MM->GetPosition ().x;
  myPos.y = MM->GetPosition ().y;  

  Ipv4Address nextHop;

  if(m_neighbors.isNeighbour (dst) || dst == m_ipv4->GetAddress (1, 0).GetBroadcast ())
  {
	  nextHop = dst;
  }
  else
  {
	  nextHop = m_neighbors.BestNeighbor (dstPos, myPos);
  }

  NS_LOG_UNCOND("BestNeighbor retornou " << nextHop);

  if (nextHop != Ipv4Address::GetZero ())
    {
      NS_LOG_DEBUG ("Destination: " << dst);

      route->SetDestination (dst);
      if (header.GetSource () == Ipv4Address ("102.102.102.102"))
        {
          route->SetSource (m_ipv4->GetAddress (1, 0).GetLocal ());
        }
      else
        {
          route->SetSource (header.GetSource ());
        }
      route->SetGateway (nextHop);
      route->SetOutputDevice (m_ipv4->GetNetDevice (m_ipv4->GetInterfaceForAddress (route->GetSource ())));
      route->SetDestination (header.GetDestination ());
      NS_ASSERT (route != 0);
      NS_LOG_DEBUG ("Exist route to " << route->GetDestination () << " from interface " << route->GetSource ());
      if (oif != 0 && route->GetOutputDevice () != oif)
        {
          NS_LOG_DEBUG ("Output device doesn't match. Dropped.");
          sockerr = Socket::ERROR_NOROUTETOHOST;
          return Ptr<Ipv4Route> ();
        }

      return route;
    }

  else
    {
      DeferredRouteOutputTag tag;
      if (!p->PeekPacketTag (tag))
        {
          p->AddPacketTag (tag); 
        }
      return LoopbackRoute (header, oif);     //in RouteInput the recovery-mode is called
    }

}
}
}
