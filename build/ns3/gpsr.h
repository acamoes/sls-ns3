/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*

 */
#ifndef GPSR_H
#define GPSR_H

#include "gpsr-ptable.h"
#include "ns3/node.h"
#include "gpsr-packet.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-l4-protocol.h"
#include "ns3/mobility-model.h"
#include "gpsr-rqueue.h"

#include "ns3/ipv4-header.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/location-service.h"
#include "ns3/god.h"
#include "gpsr-sls.h"

#include <map>
#include <complex>

namespace ns3 {
namespace gpsr {
/**
 * \ingroup gpsr
 *
 * \brief GPSR routing protocol
 */
class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
  static TypeId GetTypeId (void);
  static const uint32_t GPSR_PORT;

  /// c-tor                        
  RoutingProtocol ();
  virtual ~RoutingProtocol ();
  virtual void DoDispose ();


  ///\name From Ipv4RoutingProtocol
  //
  //
  Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);
  bool RouteInput (Ptr<const Packet> p, const Ipv4Header &header, Ptr<const NetDevice> idev,
                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                   LocalDeliverCallback lcb, ErrorCallback ecb);
  int GetProtocolNumber (void) const;
  virtual void AddHeaders (Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
  virtual void SetIpv4 (Ptr<Ipv4> ipv4);
  virtual void RecvGPSR (Ptr<Socket> socket);
  virtual void UpdateRouteToNeighbor (Ipv4Address sender, Ipv4Address receiver, Vector Pos, int speed);
  virtual void SendHello ();
  virtual bool IsMyOwnAddress (Ipv4Address src);
  bool VectorComparator(Vector vec1, Vector vec2){return ((vec1.x == vec2.x) && (vec1.x == vec2.x));}

  Ptr<Ipv4> m_ipv4;
  /// Raw socket per each IP interface, map socket -> iface address (IP + mask)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;
  /// Loopback device used to defer RREQ until packet will be fully formed
  Ptr<NetDevice> m_lo;

  Ptr<SlsLocationService> GetLS ();
  void SetLS (Ptr<SlsLocationService> locationService);

  /// Broadcast ID
  uint32_t m_requestId;
  /// Request sequence number
  uint32_t m_seqNo;

  uint32_t m_interface; //FIXME cheat used to pass interface to RLS
  Ipv4InterfaceAddress m_address; //FIXME cheat used to pass address to RLS

  /// Number of RREQs used for RREQ rate control
  uint16_t m_rreqCount;
  Time HelloInterval;

  void SetDownTarget (Ipv4L4Protocol::DownTargetCallback callback);
  Ipv4L4Protocol::DownTargetCallback GetDownTarget (void) const;

  virtual void PrintRoutingTable (ns3::Ptr<ns3::OutputStreamWrapper>) const
  {
    return;
  }


private:
  /// Start protocol operation
  void Start ();
  /// Queue packet and send route request
  void DeferredRouteOutput (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);
  /// If route exists and valid, forward packet.
  void HelloTimerExpire ();

  /// Queue packet and send route request
  Ptr<Ipv4Route> LoopbackRoute (const Ipv4Header & header, Ptr<NetDevice> oif);

  /// If route exists and valid, forward packet.
  bool Forwarding (Ptr<const Packet> p, const Ipv4Header & header, UnicastForwardCallback ucb, ErrorCallback ecb);

  /// Find socket with local interface address iface
  Ptr<Socket> FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;

  //Check packet from deffered route output queue and send if position is already available
//returns true if the IP should be erased from the list (was sent/droped)
  bool SendPacketFromQueue (Ipv4Address dst);

  //Calls SendPacketFromQueue and re-schedules
  void CheckQueue ();

  void RecoveryMode(Ipv4Address dst, Ptr<Packet> p, UnicastForwardCallback ucb, Ipv4Header header);
  
  uint32_t MaxQueueLen;    ///< The maximum number of packets that we allow a routing protocol to buffer.
  Time MaxQueueTime;       ///< The maximum period of time that a routing protocol is allowed to buffer a packet for.
  RequestQueue m_queue;

  Timer HelloIntervalTimer;
  Timer CheckQueueTimer;
  uint8_t LocationServiceName;
  PositionTable m_neighbors;
  bool PerimeterMode;
  std::list<Ipv4Address> m_queuedAddresses;
  Ptr<SlsLocationService> m_locationService;

  Ipv4L4Protocol::DownTargetCallback m_downTarget;



};
}
}
#endif /* GPSRROUTINGPROTOCOL_H */
