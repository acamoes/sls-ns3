#define NS_LOG_APPEND_CONTEXT                                   \
  if (m_ipv4) { std::clog << "[node " << m_ipv4->GetObject<Node> ()->GetId () << "] "; }

#include "gpsr-sls.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/random-variable.h"
#include "ns3/inet-socket-address.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"
#include <algorithm>
#include <limits>
#include "gpsr-packet.h"
#include "ns3/mobility-model.h"
#include "ns3/uinteger.h"
#include "gpsr.h"

NS_LOG_COMPONENT_DEFINE ("SlsLocationService");

namespace ns3
{
namespace gpsr
{
const uint16_t SlsLocationService::SLS_PORT = 666;
LocationTable lt(Time("10s"));



//-----------------------------------------------------------------------------
SlsLocationService::SlsLocationService (Time tableLifeTime = Time("10s"))
: m_table (tableLifeTime)
{
	m_maxSearchTime = tableLifeTime;
}

/*TypeId
SlsLocationService::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::rls::SlsLocationService")
      .SetParent<SlsLocationService> ()
      .AddConstructor<SlsLocationService> ();
  return tid;
}*/

SlsLocationService::~SlsLocationService ()
{
}

void
SlsLocationService::DoDispose ()
{
  return;
}


void
SlsLocationService::RecvSls (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Address sourceAddress;
  Ptr<Packet> packet = socket->RecvFrom (sourceAddress);
  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address sender = inetSourceAddr.GetIpv4 ();
  Ipv4Address receiver = m_ipv4->GetAddress (1, 0).GetLocal (); //FIXME so funciona com uma interface
  NS_LOG_DEBUG ("RLS node " << this << " received a RLS packet from " << sender << " to " << receiver);

  TypeHeader tHeader (SLS_LOCATION_QUERY);
  packet->RemoveHeader (tHeader);
  if (!tHeader.IsValid ())
    {
      NS_LOG_DEBUG ("SLS message " << packet->GetUid() << " with unknown type received: " << tHeader.Get() << ". Drop");
      return; // drop
    }
  switch (tHeader.Get ())
  {
  case SLS_LOCATION_HELLO:
  {
	  //RcvRequest (packet, receiver, sender);
	  break;
  }
  case SLS_LOCATION_UPDATE:
  {
	  //RcvReply (packet, receiver, sender);
	  break;
  }
  case SLS_LOCATION_QUERY:
  {
	  //RcvReply (packet, receiver, sender);
	  break;
  }
  case SLS_LOCATION_REPLY:
  {
	  //RcvReply (packet, receiver, sender);
	  break;
  }
  default:
	  break;
  }
}

Vector
SlsLocationService::GODPredict(Ipv4Address dst)
{
	uint32_t n = NodeList().GetNNodes ();
	uint32_t i;
	Ptr<Node> node;

	NS_LOG_UNCOND("::GOD PREDICT:: Obtaining position of " << dst);

	for(i = 0; i < n; i++)
	{
		node = NodeList().GetNode (i);
		Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

		//NS_LOG_UNCOND("Have " << ipv4->GetAddress (1, 0).GetLocal ());
		if(ipv4->GetAddress (1, 0).GetLocal () == dst)
		{
			return (*node->GetObject<MobilityModel>()).GetPosition ();
		}
	}
	Vector v;
	NS_LOG_UNCOND("::GOD PREDICT:: Position: <" << v.x << "," << v.y << ">");
	return v;
}

Vector
SlsLocationService::Predict(Ipv4Address dst)
{
	NS_LOG_UNCOND("Predict called to " << dst);

	m_table.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());

	Vector pos = m_table.GetPosition(dst, GetInvalidPosition());

	if (VectorComparator(pos, GetInvalidPosition()))
	{
		NS_LOG_UNCOND("Predict deu posicao invalida");
		return GetInvalidPosition();
	}

	int64_t timeOld = m_table.GetTime(dst).GetNanoSeconds();
	int64_t timeNow = Simulator::Now().GetNanoSeconds();
	int speed = m_table.GetSpeed(dst);

	int64_t dif = (timeNow)-(timeOld);
	int64_t dif1 = dif / 1000000000;

	int meters = speed * dif1;

	//NS_LOG_UNCOND("dif " << dif << " dif1 " << dif1 << " meters " << meters << " speed " << speed << " timeOld " << timeOld
	//		<< " timeNow " << timeNow);

	Vector newpos (pos.x + meters, pos.y,pos.z);

	NS_LOG_UNCOND("Pos Antiga " << pos);
	NS_LOG_UNCOND("Predicted Pos " << newpos);
	NS_LOG_UNCOND("GOD Predition " << GODPredict(dst));

	return newpos;

}


void
SlsLocationService::RefreshPosition(Ipv4Address dst)
{
	NS_LOG_UNCOND("Refresh position from " << dst);
	//Vector pos = m_table.GetPosition(dst, GetInvalidPosition());
	//Time time = m_table.GetTime(dst);
	int seq = m_table.GetSeqNumber(dst);
	bool flag = m_table.GetResearchFlag(dst);
	int speed = m_table.GetSpeed(dst);

	m_table.DeleteEntry(dst);

	Vector newPos = Predict(dst);

	NS_LOG_UNCOND("NewPos by Predict " << newPos);

	m_table.AddEntry(dst, newPos, speed, flag, seq);
}



//Funcao a ser utilizada aquando o hello da RSU para a OBU
void
SlsLocationService::SendLocHello(Ipv4Address dst, Ipv4Address src, Vector position)
{
	if(m_table.GetSpeed(dst)!=0)
	{
		NS_LOG_UNCOND("HELLOSLS duplicado, nao vai mandar lochello");
		return;
	}
	else{

		Vector newPosition = m_ipv4->GetObject<MobilityModel>()->GetPosition();
		NS_LOG_UNCOND ("Chamado SendLocHello por " << src << " to " << dst << " with pos:" << newPosition);

		for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator j =
				m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
		{
			LocHelloHeader lochelloHeader(src, newPosition.x, newPosition.y); //FIXME
			Ptr<Socket> socket = j->first;
			//Ipv4InterfaceAddress iface = j->second;
			Ptr<Packet> packet = Create<Packet> ();
			packet->AddHeader (lochelloHeader);
			TypeHeader tHeader (SLS_LOCATION_HELLO);
			packet->AddHeader (tHeader);
			uint16_t port = SLS_PORT;
			socket->SendTo (packet, 0, InetSocketAddress (dst, port)); //Vai chamar o RouteOutput
			NS_LOG_UNCOND("SendLocHello sent message");
		}
	}
}


void
SlsLocationService::SendLocUpdate(Ptr<Packet> p, Ipv4Address dst, Ipv4Address src, Vector Position)
{
	NS_LOG_UNCOND("SendLocUpdate called " << src << " to " << dst);

	for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
	{
		Vector pos = m_ipv4->GetObject<MobilityModel>()->GetPosition();
		int c1 = m_ipv4->GetObject<MobilityModel >()->GetVelocity().x;
		int c2 = m_ipv4->GetObject<MobilityModel >()->GetVelocity().y;
		int speed = sqrt((c1*c1) + (c2*c2));
		//TODO Velocity só está a passar o valor de X
		LocUpdateHeader locupdateHeader(src, pos.x, pos.y, speed); //FIXME É aqui que está o problema do Speed vectorial

		Ptr<Socket> socket = j->first;
		//Ipv4InterfaceAddress iface = j->second;
		Ptr<Packet> packet = Create<Packet> ();
		packet->AddHeader (locupdateHeader);
		TypeHeader tHeader (SLS_LOCATION_UPDATE);
		packet->AddHeader (tHeader);
		uint16_t port = SLS_PORT;
		socket->SendTo (packet, 0, InetSocketAddress (dst, port));
	}
}

void
SlsLocationService::ReceiveUpdate(Ptr<Packet> p, Ipv4Address dst, Ipv4Address src, Vector Position, int speed)
{
	NS_LOG_UNCOND("ReceiveUpdate from " << src << " to " << dst << " speed = " << speed );

	//FIXME Olhar para a Speed vectorial
	m_table.AddEntry(dst, Position, speed, false, 1);
	//lt.AddEntry(dst, Position, speed, false, 1);
	NS_LOG_UNCOND("ReceiveUpdate: Adicionada entrada na m_table");

}

void
SlsLocationService::SendQuery(Ipv4Address src, Ipv4Address dst, Ipv4Address query)
{
	//NS_LOG_UNCOND ("Chamado SendQuery " << src << " to " << dst << " with query:" << query);

	for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
	{
		LocQueryHeader locqueryHeader(src, dst, query); //FIXME
		Ptr<Socket> socket = j->first;
		//Ipv4InterfaceAddress iface = j->second;
		Ptr<Packet> packet = Create<Packet> ();
		packet->AddHeader (locqueryHeader);
		TypeHeader tHeader (SLS_LOCATION_QUERY);
		NS_LOG_UNCOND("QUERY PACKET " << src << " " << dst << " " << query << " " << SLS_LOCATION_QUERY);
		packet->AddHeader (tHeader);
		uint16_t port = SLS_PORT;
		if(socket->SendTo (packet, 0, InetSocketAddress (dst, port)) != -1) //Vai chamar o RouteOutput
		{
			TypeHeader tHeader (SLS_LOCATION_QUERY);
			packet->RemoveHeader (tHeader);

			NS_LOG_UNCOND("QSR PEDIDO " << src << " " << query << " Time " << Simulator::Now());
			NS_LOG_UNCOND("Foi mandado um sls_location_query " << tHeader.Get());
			NS_LOG_UNCOND("SendQuery " << src << " " << query << " " << Simulator::Now() << " " << packet->GetUid());
		}
		else
		{ NS_LOG_UNCOND("Error Sendquery");}
	}

}

Vector
SlsLocationService::RSUSearch(Ipv4Address adr)
{
	uint32_t n = NodeList().GetNNodes ();
	uint32_t i;
	Ptr<Node> node;
	Vector position;
	int speed;
	Time timeOld;
	//Vector inVec = GetInvalidPosition();

	NS_LOG_UNCOND("::RSUSEARCH:: Obtaining position of " << adr);

	for(i = 0; i < n; i++)
	{
		node = NodeList().GetNode (i);
		Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

		//NS_LOG_UNCOND("Have " << ipv4->GetAddress (1, 0).GetLocal ());
		if(ipv4->GetAddress (1, 0).GetLocal ().IsEqual("10.0.0.1") || ipv4->GetAddress (1, 0).GetLocal ().IsEqual("10.0.0.2")
				|| ipv4->GetAddress (1, 0).GetLocal ().IsEqual("10.0.0.3"))
		{
			//pos = (*node->GetObject<SlsLocationService>()).GetPosition(adr);
			Ptr<SlsLocationService> LS = (*node->GetObject<RoutingProtocol>()).GetLS();
			position = LS->GetPosition(adr);

			if(!VectorComparator(position, GetInvalidPosition()))
			{
				NS_LOG_UNCOND("::RSUSEARCH:: Position find is " << position << " from RSU "
						<< ipv4->GetAddress (1, 0).GetLocal ());

				//Predict Phase
				speed = LS->GetSpeed(adr);
				timeOld = LS->GetTime(adr);
				Time timeNow = Simulator::Now();

				int dif = (timeNow.GetInteger()/1000000000)-(timeOld.GetInteger()/1000000000);
				int meters = speed * dif;

				NS_LOG_UNCOND("dif " << dif << " meters " << meters << " speed " << speed << " timeOld " << timeOld
						<< " timeNow " << timeNow);

				Vector newpos (position.x + meters, position.y,position.z);

				NS_LOG_UNCOND("::RSUSEARCH:: Position predicted is " << newpos);

				return newpos;
			}
		}
	}
	return position;
}


void
SlsLocationService::ReceiveQuery(Ipv4Address src, Ipv4Address dst, Ipv4Address query)
{
	NS_LOG_UNCOND ("Chamado ReceiveQuery " << src << " to " << dst << " with query:" << query << " time " << Simulator::Now());

	m_table.Purge();

	Vector pos = m_table.GetPosition(query, GetInvalidPosition());
	NS_LOG_UNCOND("Pos returned " << pos);

	if(VectorComparator(pos, GetInvalidPosition()))
	{
		NS_LOG_UNCOND("RSU nao sabe posicao e vai consultar outras");

		pos = RSUSearch(query);
		NS_LOG_UNCOND("RSU já sabe posição " << pos);
		Vector posGOD = GODPredict(query);
		NS_LOG_UNCOND("Com o GODPredict dava " << posGOD);
	}
	else
	{
		Vector posGOD = GODPredict(query);
		Vector pos = Predict(query);
		NS_LOG_UNCOND("ReceiveQuery vai fazer um SendReply com pos by predict = " << pos);
		NS_LOG_UNCOND("Com o GODPredict dava " << posGOD);
	}

	SendReply(dst, src,query, pos);

	//FIXME

}

void
SlsLocationService::ReceiveReply(Ipv4Address src, Ipv4Address dst, Vector position)
{
	NS_LOG_UNCOND("RECEIVEREPLY " << src << " " << dst << " " << position);
	m_table.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());

	m_table.DeleteEntry(dst);
	m_table.AddEntry(dst, position, 0, false, 0);
	NS_LOG_UNCOND("QSR RESPOSTA " << dst << " " << src << " Time " << Simulator::Now());
	m_table.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
	Vector posGOD = GODPredict(dst);
	NS_LOG_UNCOND("Precision " << dst << " SLSPOS " << position << " " << " GODPOS " << posGOD);
	NS_LOG_UNCOND("ReceiveReply " << " dst " << dst << " RSU " << src << " position " << position << " Time " << Simulator::Now());

}

void
SlsLocationService::SendReply(Ipv4Address src, Ipv4Address dst, Ipv4Address query, Vector position)
{
	NS_LOG_UNCOND("SENDREPLY");
	m_table.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());

	for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator j =
				m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
	{
		LocReplyHeader locreplyHeader(src, query, position.x, position.y); //FIXME
		Ptr<Socket> socket = j->first;
		//Ipv4InterfaceAddress iface = j->second;
		Ptr<Packet> packet = Create<Packet> ();
		packet->AddHeader (locreplyHeader);
		TypeHeader tHeader (SLS_LOCATION_REPLY);
		packet->AddHeader (tHeader);
		uint16_t port = SLS_PORT;
		if(socket->SendTo (packet, 0, InetSocketAddress (dst, port)) != -1) //Vai chamar o RouteOutput
		{
			TypeHeader tHeader (SLS_LOCATION_REPLY);
			packet->RemoveHeader (tHeader);

			NS_LOG_UNCOND("Foi mandado um sls_location_reply " << tHeader.Get());
			NS_LOG_UNCOND("SendReply " << dst << " " << query << " " << Simulator::Now() << " " << packet->GetUid());

		}
		else
		{ NS_LOG_UNCOND("Error Sendreply");}
	}

}

void
SlsLocationService::Print()
{
	m_table.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
}

void
SlsLocationService::SetIpv4 (Ptr<Ipv4> ipv4)
{
  m_ipv4 = ipv4;

  Simulator::ScheduleNow (&SlsLocationService::Start, this);
}

void
SlsLocationService::Start ()
{NS_LOG_FUNCTION (this);}

void
SlsLocationService::NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << " interface " << i << " address " << address << " (RLS)");
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (!l3->IsUp (i))
    return;
  if (l3->GetNAddresses (i) == 1)
    {
      Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
      if (!socket)
        {
          if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
            return;
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
              UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (MakeCallback (&SlsLocationService::RecvSls,this));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          // Bind to any IP address so that broadcasts can be received
          socket->Bind (InetSocketAddress (Ipv4Address::GetAny(), SLS_PORT));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));
        }
    }
  else
    {
      NS_LOG_LOGIC ("RLS does not work with more then one address per each interface. Ignore added address");
    }
}

void
SlsLocationService::NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address)
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
          socket->SetRecvCallback (MakeCallback (&SlsLocationService::RecvSls, this));
          // Bind to any IP address so that broadcasts can be received
          socket->Bind (InetSocketAddress (Ipv4Address::GetAny(), SLS_PORT));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));

        }
      if (m_socketAddresses.empty ())
        {
          NS_LOG_LOGIC ("No RLS interfaces");
          return;
        }
    }
  else
    {
      NS_LOG_LOGIC ("Remove address not participating in RLS operation");
    }
}

void
SlsLocationService::NotifyInterfaceUp (uint32_t i)
{
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (l3->GetNAddresses (i) > 1)
    {
      NS_LOG_WARN ("SLS does not work with more then one address per each interface .");
    }
  Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    return;

  // Create a socket to listen only on this interface
  Ptr<Socket> socket = Socket::CreateSocket (m_ipv4->GetObject<Node> (),
      UdpSocketFactory::GetTypeId ());
  NS_ASSERT (socket != 0);
  socket->SetRecvCallback (MakeCallback (&SlsLocationService::RecvSls, this));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), SLS_PORT));
  socket->SetAllowBroadcast (true);
  socket->SetAttribute ("IpTtl", UintegerValue (1));
  m_socketAddresses.insert (std::make_pair (socket, iface));
}

void
SlsLocationService::NotifyInterfaceDown (uint32_t i)
{

  // Close socket
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (m_ipv4->GetAddress (i, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_socketAddresses.erase (socket);
  if (m_socketAddresses.empty ())
    {
      NS_LOG_LOGIC ("No  interfaces (RLS)");
    }
}

bool
SlsLocationService::IsInSearch(Ipv4Address id) {
	//NS_LOG_UNCOND("ISINSEARCH: " << id << " pos " << m_table.GetPosition(id, Vector(-1, -1, 0)) << " flag " << m_table.GetResearchFlag(id));
	NS_LOG_UNCOND("IsinSearch");
	m_table.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
	NS_LOG_UNCOND("ISINSEARCH updateTime " << m_table.GetEntryUpdateTime(id) << " maxTime " << m_maxSearchTime << " timeNOW " << Simulator::Now());

	if((m_table.GetEntryUpdateTime(id) + m_maxSearchTime) < Simulator::Now())
	{
		NS_LOG_UNCOND("ISINSEARCH ERASED");
		m_table.DeleteEntry(id);
		return false;
	}
	//NS_LOG_UNCOND("ISINSEARCH will return " << m_table.GetResearchFlag(id) << " id " << id);

	if(m_table.GetResearchFlag(id))
	{
		NS_LOG_UNCOND("Está a procura do " << id);
	}
	else
	{
		NS_LOG_UNCOND("Já não esta a procura do " << id);
	}
	return m_table.GetResearchFlag(id); }


double 
SlsLocationService::PositionsDistance(Vector from, Vector to) {

	return sqrt(pow(from.x - to.x, 2) + pow(from.y - to.y, 2) + pow(from.z - to.z, 2));
}

Ptr<Socket>
SlsLocationService::FindSocketWithInterfaceAddress (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket> , Ipv4InterfaceAddress>::const_iterator j =
      m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        return socket;
    }
  Ptr<Socket> socket;
  return socket;
}

/*FIXME if return == getInvsalid no lado do gpsr por pa defered outra vez*/
Vector
SlsLocationService::GetPosition(Ipv4Address id)
{
	/*** Modificar este GetPosition, criar m_rsu **/
	//if((m_table.GetEntryUpdateTime(id) + m_maxSearchTime) < Simulator::Now())
	if((m_table.GetEntryUpdateTime(id)!=0) && (m_table.GetEntryUpdateTime(id) + Time("10s")) < Simulator::Now())
	{
		m_table.PrintTable(m_ipv4->GetAddress (1, 0).GetLocal());
		NS_LOG_UNCOND("GetEntryUpdateTime " << m_table.GetEntryUpdateTime(id));
		NS_LOG_UNCOND("m_maxSearchTime " << m_maxSearchTime);
		NS_LOG_UNCOND("SimulatorNow " << Simulator::Now());

		if(id!="10.0.0.1" && id!="10.0.0.2" && id!="10.0.0.3")
		{
			m_table.DeleteEntry(id);
		}

	}

	Ipv4Address sender = m_ipv4->GetAddress (1, 0).GetLocal ();
	NS_LOG_UNCOND("GetPosition SENDER " << sender << " query to " << id << " e obu? " << GetFunction());
	Vector pos;

	if(GetFunction())
	{
		//Sou OBU
		if(id==m_rsu)
		{
			return m_posrsu;
		}

		pos = m_table.GetPosition(id, GetInvalidPosition());

		if(VectorComparator(pos, GetInvalidPosition()))
		{
			m_table.AddEntry(id, pos, 0, true, 1);
			SendQuery(sender, m_rsu, id);
			//FIXME TODO SendQuery(sender,m_ipv4->GetAddress(1,0).GetBroadcast(), id);
			Vector posi = GODPredict(id);
			NS_LOG_UNCOND("Com o GodPredicti posição dava " << posi);
			m_table.AddEntry(id, pos, 0, true, 1);
			pos=m_posrsu;

		}

		return pos;
	}
	else
	{
		//Sou RSU e vou procurar id
		pos = m_table.GetPosition(id, GetInvalidPosition());
	}

	return pos;


}

int
SlsLocationService::GetSpeed(Ipv4Address  adr)
{
	int speed;
	speed = m_table.GetSpeed(adr);
	return speed;
}

Time
SlsLocationService::GetTime(Ipv4Address adr)
{
	Time time;
	time = m_table.GetTime(adr);
	return time;
}

Vector
SlsLocationService::GetPositionOf(Ipv4Address  adr)
{
return Vector (-1, -1, 0);
}

bool
SlsLocationService::HasPosition(Ipv4Address  adr){

	NS_LOG_UNCOND("HasPosition " << adr);

	if((m_table.GetEntryUpdateTime(adr) + m_maxSearchTime) < Simulator::Now())
	{
		m_table.DeleteEntry(adr);
		NS_LOG_UNCOND("Delete Entry no Hasposition");
		return false;
	}

	Vector pos = GetPosition(adr);

	//if(VectorComparator(GetPosition(adr, GetInvalidPosition()),GetInvalidPosition())){
	if(!VectorComparator(pos,GetInvalidPosition())){
		NS_LOG_UNCOND("Já tem a posicao do " << adr);
		return true;
	}
	return false;
}
}
}
