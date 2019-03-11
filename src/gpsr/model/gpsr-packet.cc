/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "gpsr-packet.h"
#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("GpsrPacket");

namespace ns3 {
namespace gpsr {

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t = GPSRTYPE_HELLO)
  : m_type (t),
    m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::gpsr::TypeHeader")
    .SetParent<Header> ()
    .AddConstructor<TypeHeader> ()
  ;
  return tid;
}

TypeId
TypeHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
TypeHeader::GetSerializedSize () const
{
  return 1;
}

void
TypeHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
TypeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();

  m_valid = true;
  switch (type)
  {
  case GPSRTYPE_HELLO:
  case GPSRTYPE_POS:
  case SLS_LOCATION_HELLO:
  case SLS_LOCATION_UPDATE:
  case SLS_LOCATION_QUERY:
  case SLS_LOCATION_REPLY:
  {
          m_type = (MessageType) type;
          break;
  }
  default:
          m_valid = false;
          break;
  }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TypeHeader::Print (std::ostream &os) const
{
  switch (m_type)
    {
    case GPSRTYPE_HELLO:
      {
        os << "HELLO";
        break;
      }
    case GPSRTYPE_POS:
      {
        os << "POSITION";
        break;
      }
    case SLS_LOCATION_HELLO:
      {
        os << "LOCATION_HELLO";
        break;
      }
    case SLS_LOCATION_UPDATE:
      {
        os << "LOCATION_UPDATE";
        break;
      }
    case SLS_LOCATION_QUERY:
         {
           os << "LOCATION_QUERY";
           break;
         }
    case SLS_LOCATION_REPLY:
         {
           os << "LOCATION_REPLY";
           break;
         }
    default:
      os << "UNKNOWN_TYPE";
      break;
    }
}

bool
TypeHeader::operator== (TypeHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream & os, TypeHeader const & h)
{
  h.Print (os);
  return os;
}

//-----------------------------------------------------------------------------
// HELLO
//-----------------------------------------------------------------------------
HelloHeader::HelloHeader (uint64_t originPosx, uint64_t originPosy)
  : m_originPosx (originPosx),
    m_originPosy (originPosy)
{
}

NS_OBJECT_ENSURE_REGISTERED (HelloHeader);

TypeId
HelloHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::gpsr::HelloHeader")
    .SetParent<Header> ()
    .AddConstructor<HelloHeader> ()
  ;
  return tid;
}

TypeId
HelloHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
HelloHeader::GetSerializedSize () const
{
  return 16;
}

void
HelloHeader::Serialize (Buffer::Iterator i) const
{
  //NS_LOG_UNCOND ("Serialize X " << m_originPosx << " Y " << m_originPosy);

  i.WriteHtonU64 (m_originPosx);
  i.WriteHtonU64 (m_originPosy);

}

uint32_t
HelloHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  //NS_LOG_UNCOND("Origin1 " << i.ReadNtohU64 ());
  m_originPosx = i.ReadNtohU64 ();
  m_originPosy = i.ReadNtohU64 ();

  uint32_t dist = i.GetDistanceFrom (start);

  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
HelloHeader::Print (std::ostream &os) const
{
  os << " PositionX: " << m_originPosx
     << " PositionY: " << m_originPosy;
}

std::ostream &
operator<< (std::ostream & os, HelloHeader const & h)
{
  h.Print (os);
  return os;
}



bool
HelloHeader::operator== (HelloHeader const & o) const
{
  return (m_originPosx == o.m_originPosx && m_originPosy == m_originPosy);
}

//-----------------------------------------------------------------------------
// Position
//-----------------------------------------------------------------------------
PositionHeader::PositionHeader (uint64_t dstPosx, uint64_t dstPosy, uint32_t updated, uint64_t recPosx, uint64_t recPosy, uint8_t inRec, uint64_t lastPosx, uint64_t lastPosy)
  : m_dstPosx (dstPosx),
    m_dstPosy (dstPosy),
    m_updated (updated),
    m_recPosx (recPosx),
    m_recPosy (recPosy),
    m_inRec (inRec),
    m_lastPosx (lastPosx),
    m_lastPosy (lastPosy)
{
}

NS_OBJECT_ENSURE_REGISTERED (PositionHeader);

TypeId
PositionHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::gpsr::PositionHeader")
    .SetParent<Header> ()
    .AddConstructor<PositionHeader> ()
  ;
  return tid;
}

TypeId
PositionHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
PositionHeader::GetSerializedSize () const
{
  return 53;
}

void
PositionHeader::Serialize (Buffer::Iterator i) const
{

  i.WriteU64 (m_dstPosx);
  i.WriteU64 (m_dstPosy);
  i.WriteU32 (m_updated);
  i.WriteU64 (m_recPosx);
  i.WriteU64 (m_recPosy);
  i.WriteU8 (m_inRec);
  i.WriteU64 (m_lastPosx);
  i.WriteU64 (m_lastPosy);
}

uint32_t
PositionHeader::Deserialize (Buffer::Iterator start)
{

  Buffer::Iterator i = start;
  m_dstPosx = i.ReadU64 ();
  m_dstPosy = i.ReadU64 ();
  m_updated = i.ReadU32 ();
  m_recPosx = i.ReadU64 ();
  m_recPosy = i.ReadU64 ();
  m_inRec = i.ReadU8 ();
  m_lastPosx = i.ReadU64 ();
  m_lastPosy = i.ReadU64 ();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
PositionHeader::Print (std::ostream &os) const
{
  os << " PositionX: "  << m_dstPosx
     << " PositionY: " << m_dstPosy
     << " Updated: " << m_updated
     << " RecPositionX: " << m_recPosx
     << " RecPositionY: " << m_recPosy
     << " inRec: " << m_inRec
     << " LastPositionX: " << m_lastPosx
     << " LastPositionY: " << m_lastPosy;
}

std::ostream &
operator<< (std::ostream & os, PositionHeader const & h)
{
  h.Print (os);
  return os;
}



bool
PositionHeader::operator== (PositionHeader const & o) const
{
  return (m_dstPosx == o.m_dstPosx && m_dstPosy == m_dstPosy && m_updated == o.m_updated && m_recPosx == o.m_recPosx && m_recPosy == o.m_recPosy && m_inRec == o.m_inRec && m_lastPosx == o.m_lastPosx && m_lastPosy == o.m_lastPosy);
}


//-----------------------------------------------------------------------------
// LOCATION Hello
//-----------------------------------------------------------------------------

LocHelloHeader::LocHelloHeader(Ipv4Address nodeid, uint64_t nodePosx, uint64_t nodePosy):
	m_nodeid (nodeid),
	m_nodePosx(nodePosx),
	m_nodePosy(nodePosy)
	{}

NS_OBJECT_ENSURE_REGISTERED (LocHelloHeader);

TypeId
LocHelloHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::gpsr::LocHelloHeader")
      .SetParent<Header> ()
      .AddConstructor<LocHelloHeader> ()
      ;
  return tid;
}

TypeId
LocHelloHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
LocHelloHeader::GetSerializedSize () const
{
  return 20;
}

void
LocHelloHeader::Serialize (Buffer::Iterator i) const
{
  WriteTo(i, m_nodeid);
  i.WriteHtonU64(m_nodePosx);
  i.WriteHtonU64(m_nodePosy);
}

uint32_t
LocHelloHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  ReadFrom(i, m_nodeid);
  m_nodePosx = i.ReadNtohU64();
  m_nodePosy = i.ReadNtohU64();

  uint32_t dist = i.GetDistanceFrom (start);

  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
LocHelloHeader::Print (std::ostream &os) const
{
  os << " Nodeid " << m_nodeid;
}

bool
LocHelloHeader::operator== (LocHelloHeader const & o) const
{
  return (m_nodeid == o.m_nodeid);
}

std::ostream &
operator<< (std::ostream & os, LocHelloHeader const & h)
{
  h.Print (os);
  return os;
}

//-----------------------------------------------------------------------------
// LOCATION Update
//-----------------------------------------------------------------------------
LocUpdateHeader::LocUpdateHeader(Ipv4Address nodeid, uint64_t originPosx, uint64_t originPosy, uint32_t speed):
	m_nodeid (nodeid),
	m_originPosx (originPosx),
	m_originPosy (originPosy),
	m_speed (speed)
	{}

NS_OBJECT_ENSURE_REGISTERED (LocUpdateHeader);

TypeId
LocUpdateHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::gpsr::LocUpdateHeader")
      .SetParent<Header> ()
      .AddConstructor<LocUpdateHeader> ()
      ;
  return tid;
}

TypeId
LocUpdateHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
LocUpdateHeader::GetSerializedSize () const
{
  return 24;
}

void
LocUpdateHeader::Serialize (Buffer::Iterator i) const
{
  WriteTo (i, m_nodeid);
  i.WriteHtonU64((uint64_t) m_originPosx);
  i.WriteHtonU64((uint64_t) m_originPosy);
  i.WriteHtonU32 (m_speed);
}

uint32_t
LocUpdateHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  ReadFrom(i, m_nodeid);
  m_originPosx = i.ReadNtohU64();
  m_originPosy = i.ReadNtohU64();
  m_speed = i.ReadNtohU32();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
LocUpdateHeader::Print (std::ostream &os) const
{
  os << " Nodeid " << m_nodeid << " PositionX " << m_originPosx << " PositionY " << m_originPosy << " speed "
		  << m_speed;
}

bool
LocUpdateHeader::operator== (LocUpdateHeader const & o) const
{
  return (m_nodeid == o.m_nodeid && m_originPosx == o.m_originPosx && m_originPosy == o.m_originPosy &&
          m_speed == o.m_speed);
}

std::ostream &
operator<< (std::ostream & os, LocUpdateHeader const & h)
{
  h.Print (os);
  return os;
}

//-----------------------------------------------------------------------------
// LOCATION Query
//-----------------------------------------------------------------------------
LocQueryHeader::LocQueryHeader(Ipv4Address nodeid, Ipv4Address destid, Ipv4Address queryid):
	m_nodeid (nodeid),
	m_destid(destid),
	m_queryid(queryid)
{}

NS_OBJECT_ENSURE_REGISTERED (LocQueryHeader);

TypeId
LocQueryHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::gpsr::LocQueryHeader")
      .SetParent<Header> ()
      .AddConstructor<LocUpdateHeader> ()
      ;
  return tid;
}

TypeId
LocQueryHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
LocQueryHeader::GetSerializedSize () const
{
  return 12;
}

void
LocQueryHeader::Serialize (Buffer::Iterator i) const
{
  WriteTo (i, m_nodeid);
  WriteTo (i, m_destid);
  WriteTo (i, m_queryid);
}

uint32_t
LocQueryHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  ReadFrom(i, m_nodeid);
  ReadFrom(i, m_destid);
  ReadFrom(i, m_queryid);

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
LocQueryHeader::Print (std::ostream &os) const
{
  os << " Nodeid " << m_nodeid << " Destid " << m_destid << " Queryid " << m_queryid;
}

bool
LocQueryHeader::operator== (LocQueryHeader const & o) const
{
  return (m_nodeid == o.m_nodeid && m_destid == o.m_destid && m_queryid == o.m_queryid);
}

std::ostream &
operator<< (std::ostream & os, LocQueryHeader const & h)
{
  h.Print (os);
  return os;
}

//-----------------------------------------------------------------------------
// LOCATION Reply
//-----------------------------------------------------------------------------
LocReplyHeader::LocReplyHeader(Ipv4Address nodeid, Ipv4Address destid, uint64_t posx, uint64_t posy):
	m_nodeid (nodeid),
	m_destid(destid),
	m_posx(posx),
	m_posy(posy)
{}

NS_OBJECT_ENSURE_REGISTERED (LocReplyHeader);

TypeId
LocReplyHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::gpsr::LocReplyHeader")
      .SetParent<Header> ()
      .AddConstructor<LocUpdateHeader> ()
      ;
  return tid;
}

TypeId
LocReplyHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
LocReplyHeader::GetSerializedSize () const
{
  return 24;
}

void
LocReplyHeader::Serialize (Buffer::Iterator i) const
{
  WriteTo (i, m_nodeid);
  WriteTo (i, m_destid);
  i.WriteHtonU64((uint64_t) m_posx);
  i.WriteHtonU64((uint64_t) m_posy);

}

uint32_t
LocReplyHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  ReadFrom(i, m_nodeid);
  ReadFrom(i, m_destid);
  m_posx = i.ReadNtohU64();
  m_posy = i.ReadNtohU64();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
LocReplyHeader::Print (std::ostream &os) const
{
  os << " Nodeid " << m_nodeid << " Destid " << m_destid << " Posx " << m_posx << " Posy " << m_posy;
}

bool
LocReplyHeader::operator== (LocReplyHeader const & o) const
{
  return (m_nodeid == o.m_nodeid && m_destid == o.m_destid && m_posx == o.m_posx && m_posy == o.m_posy);
}

std::ostream &
operator<< (std::ostream & os, LocReplyHeader const & h)
{
  h.Print (os);
  return os;
}


}
}
