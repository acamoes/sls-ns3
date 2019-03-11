/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef GPSRPACKET_H
#define GPSRPACKET_H

#include <iostream>
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include <map>
#include "ns3/nstime.h"
#include "ns3/vector.h"

namespace ns3 {
namespace gpsr {


enum MessageType
{
  GPSRTYPE_HELLO  = 1,         //!< GPSRTYPE_HELLO
  GPSRTYPE_POS = 2,            //!< GPSRTYPE_POS
  SLS_LOCATION_HELLO = 3,
  SLS_LOCATION_UPDATE = 4,
  SLS_LOCATION_QUERY = 5,
  SLS_LOCATION_REPLY = 6,
};

/**
 * \ingroup gpsr
 * \brief GPSR types
 */

/**************************************************************************************
 * TYPEHEADER
 **************************************************************************************/

class TypeHeader : public Header
{
public:
  /// c-tor
  TypeHeader (MessageType t);

  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  //\}

  /// Return type
  MessageType Get () const
  {
    return m_type;
  }
  /// Check that type if valid
  bool IsValid () const
  {
    return m_valid; //FIXME that way it wont work
  }
  bool operator== (TypeHeader const & o) const;
private:
  MessageType m_type;
  bool m_valid;
};

std::ostream & operator<< (std::ostream & os, TypeHeader const & h);

class HelloHeader : public Header
{
public:
  /// c-tor
  HelloHeader (uint64_t originPosx = 0, uint64_t originPosy = 0);

  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  //\}

  ///\name Fields
  //\{
  void SetOriginPosx (uint64_t posx)
  {
    m_originPosx = posx;
  }
  uint64_t GetOriginPosx () const
  {
    return m_originPosx;
  }
  void SetOriginPosy (uint64_t posy)
  {
    m_originPosy = posy;
  }
  uint64_t GetOriginPosy () const
  {
    return m_originPosy;
  }
  //\}


  bool operator== (HelloHeader const & o) const;
private:
  uint64_t         m_originPosx;          ///< Originator Position x
  uint64_t         m_originPosy;          ///< Originator Position x
};

std::ostream & operator<< (std::ostream & os, HelloHeader const &);

/**************************************************************************************
 * PositionHeader
 **************************************************************************************/

class PositionHeader : public Header
{
public:
  /// c-tor
  PositionHeader (uint64_t dstPosx = 0, uint64_t dstPosy = 0, uint32_t updated = 0, uint64_t recPosx = 0, uint64_t recPosy = 0, uint8_t inRec  = 0, uint64_t lastPosx = 0, uint64_t lastPosy = 0);

  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  //\}

  ///\name Fields
  //\{
  void SetDstPosx (uint64_t posx)
  {
    m_dstPosx = posx;
  }
  uint64_t GetDstPosx () const
  {
    return m_dstPosx;
  }
  void SetDstPosy (uint64_t posy)
  {
    m_dstPosy = posy;
  }
  uint64_t GetDstPosy () const
  {
    return m_dstPosy;
  }
  void SetUpdated (uint32_t updated)
  {
    m_updated = updated;
  }
  uint32_t GetUpdated () const
  {
    return m_updated;
  }
  void SetRecPosx (uint64_t posx)
  {
    m_recPosx = posx;
  }
  uint64_t GetRecPosx () const
  {
    return m_recPosx;
  }
  void SetRecPosy (uint64_t posy)
  {
    m_recPosy = posy;
  }
  uint64_t GetRecPosy () const
  {
    return m_recPosy;
  }
  void SetInRec (uint8_t rec)
  {
    m_inRec = rec;
  }
  uint8_t GetInRec () const
  {
    return m_inRec;
  }
  void SetLastPosx (uint64_t posx)
  {
    m_lastPosx = posx;
  }
  uint64_t GetLastPosx () const
  {
    return m_lastPosx;
  }
  void SetLastPosy (uint64_t posy)
  {
    m_lastPosy = posy;
  }
  uint64_t GetLastPosy () const
  {
    return m_lastPosy;
  }


  bool operator== (PositionHeader const & o) const;

private:
  uint64_t         m_dstPosx;          ///< Destination Position x
  uint64_t         m_dstPosy;          ///< Destination Position x
  uint32_t         m_updated;          ///< Time of last update
  uint64_t         m_recPosx;          ///< x of position that entered Recovery-mode
  uint64_t         m_recPosy;          ///< y of position that entered Recovery-mode
  uint8_t          m_inRec;          ///< 1 if in Recovery-mode, 0 otherwise
  uint64_t         m_lastPosx;          ///< x of position of previous hop
  uint64_t         m_lastPosy;          ///< y of position of previous hop

};

std::ostream & operator<< (std::ostream & os, PositionHeader const &);

/**************************************************************************************
 * LocHelloHeader
 **************************************************************************************/

class LocHelloHeader : public Header
{
public:
	// NodeID - NodePosition - NodeSpeed
	LocHelloHeader(Ipv4Address nodeid = Ipv4Address(), uint64_t nodePosx = 0, uint64_t nodePosy = 0);

	static TypeId GetTypeId ();
	TypeId GetInstanceTypeId () const;
	uint32_t GetSerializedSize () const;
	void Serialize (Buffer::Iterator start) const;
	uint32_t Deserialize (Buffer::Iterator start);
	void Print (std::ostream &os) const;

	Ipv4Address GetNodeID() const
	{
		return m_nodeid;
	}
	void SetNodeID(Ipv4Address node)
	{
		m_nodeid = node;
	}
	void SetOriginPosx (uint64_t posx)
	{
		m_nodePosx = posx;
	}
	uint64_t GetOriginPosx () const
	{
		return m_nodePosx;
	}
	void SetOriginPosy (uint64_t posy)
	{
		m_nodePosy = posy;
	}
	uint64_t GetOriginPosy () const
	{
		return m_nodePosy;
	}
	bool operator== (LocHelloHeader const & o) const;

private:
	 Ipv4Address m_nodeid;
	 uint64_t         m_nodePosx;
	 uint64_t         m_nodePosy;
};

std::ostream & operator<< (std::ostream & os, LocHelloHeader const &);

/**************************************************************************************
 * LocUpdateHeader
 **************************************************************************************/

class LocUpdateHeader : public Header
{
public:
	// NodeID - NodePosition - NodeSpeed
	LocUpdateHeader(Ipv4Address nodeid = Ipv4Address(), uint64_t originPosx = 0, uint64_t originPosy = 0, uint32_t speed = 0);

	static TypeId GetTypeId ();
	TypeId GetInstanceTypeId () const;
	uint32_t GetSerializedSize () const;
	void Serialize (Buffer::Iterator start) const;
	uint32_t Deserialize (Buffer::Iterator start);
	void Print (std::ostream &os) const;

	Ipv4Address GetNodeID() const
	{
		return m_nodeid;
	}
	void SetNodeID(Ipv4Address node)
	{
		m_nodeid = node;
	}
	void SetOriginPosx (uint64_t posx)
	{
		m_originPosx = posx;
	}
	uint64_t GetOriginPosx () const
	{
		return m_originPosx;
	}
	void SetOriginPosy (uint64_t posy)
	{
		m_originPosy = posy;
	}
	uint64_t GetOriginPosy () const
	{
		return m_originPosy;
	}
	uint32_t GetSpeed() const
	{
		return m_speed;
	}
	void SetSpeed(uint32_t speed)
	{
		m_speed = speed;
	}

	 bool operator== (LocUpdateHeader const & o) const;

private:
	  Ipv4Address 		   m_nodeid;
	  uint64_t         m_originPosx;          ///< Originator Position x
	  uint64_t         m_originPosy;          ///< Originator Position Y
	  uint32_t 		   m_speed;
};

std::ostream & operator<< (std::ostream & os, LocUpdateHeader const &);

/**************************************************************************************
 * LocQueryHeader
 **************************************************************************************/

class LocQueryHeader : public Header
{
public:
	// NodeID - NodePosition - NodeSpeed
	LocQueryHeader(Ipv4Address nodeid = Ipv4Address(), Ipv4Address destid = Ipv4Address(),Ipv4Address queryid = Ipv4Address());

	static TypeId GetTypeId ();
	TypeId GetInstanceTypeId () const;
	uint32_t GetSerializedSize () const;
	void Serialize (Buffer::Iterator start) const;
	uint32_t Deserialize (Buffer::Iterator start);
	void Print (std::ostream &os) const;

	Ipv4Address GetNodeID() const
	{
		return m_nodeid;
	}
	void SetNodeID(Ipv4Address node)
	{
		m_nodeid = node;
	}

	Ipv4Address GetDestID() const
	{
		return m_destid;
	}
	void SetDestID(Ipv4Address node)
	{
		m_destid = node;
	}
	Ipv4Address GetQueryID() const
	{
		return m_queryid;
	}
	void SetQueryID(Ipv4Address node)
	{
		m_queryid = node;
	}


	 bool operator== (LocQueryHeader const & o) const;

private:
	  Ipv4Address 		   m_nodeid;
	  Ipv4Address 		   m_destid;
	  Ipv4Address		   m_queryid;
};

std::ostream & operator<< (std::ostream & os, LocQueryHeader const &);

/**************************************************************************************
 * LocReplyHeader
 **************************************************************************************/

class LocReplyHeader : public Header
{
public:
	// NodeID - NodePosition - NodeSpeed
	LocReplyHeader(Ipv4Address nodeid = Ipv4Address(), Ipv4Address destid = Ipv4Address(),uint64_t posx = 0,
			uint64_t posy=0);

	static TypeId GetTypeId ();
	TypeId GetInstanceTypeId () const;
	uint32_t GetSerializedSize () const;
	void Serialize (Buffer::Iterator start) const;
	uint32_t Deserialize (Buffer::Iterator start);
	void Print (std::ostream &os) const;

	Ipv4Address GetNodeID() const
	{
		return m_nodeid;
	}
	void SetNodeID(Ipv4Address node)
	{
		m_nodeid = node;
	}

	Ipv4Address GetDestID() const
	{
		return m_destid;
	}
	void SetDestID(Ipv4Address node)
	{
		m_destid = node;
	}
	uint64_t GetPosx() const
	{
		return m_posx;
	}
	void SetPosx(uint64_t posx)
	{
		m_posx = posx;
	}
	uint64_t GetPosy() const
	{
		return m_posy;
	}
	void SetPosy(uint64_t posy)
	{
		m_posy = posy;
	}


	 bool operator== (LocReplyHeader const & o) const;

private:
	  Ipv4Address 		   m_nodeid;
	  Ipv4Address 		   m_destid;
	  uint64_t    		   m_posx;
	  uint64_t			   m_posy;
};

std::ostream & operator<< (std::ostream & os, LocReplyHeader const &);


}
}
#endif /* GPSRPACKET_H */
