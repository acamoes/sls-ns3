#ifndef SlsLocationService_H
#define SlsLocationService_H

#include "ns3/node.h"
#include "ns3/location-service.h"
#include "ns3/ipv4-l3-protocol.h"
#include "gpsr-ltable.h"
#include <map>


namespace ns3
{
namespace gpsr
{
/**
 * \ingroup gpsr
 *
 * \brief RLS Location Service
 */
class SlsLocationService : public LocationService
{
public:
	static const uint16_t SLS_PORT;
	//static TypeId GetTypeId (void);
	void SetIpv4 (Ptr<Ipv4> ipv4);

	/* Função que devolve um vector inválido para quando uma posição
	 * nao se encontram na tabela */
	Vector GetInvalidPosition() {return Vector(-1, -1, 0); }
	/* Função a usar pelo routing quando quiser obter uma posição
	 * Se nao tiver a posição começa pesquisa
	 * FIXME: Ainda nao está definida */
	Vector GetPositionOf(Ipv4Address  adr);

	/* Confirm se tem posiçao e n ta searching, mas se n tiver, n começa a procura
	 * FIXME: Ainda n definida
	 */
	bool HasPosition(Ipv4Address  adr);

	/* Funções que permitem aceder à tabela directamente */
	Time GetEntryUpdateTime(Ipv4Address id){ return m_table.GetEntryUpdateTime(id); }

	Vector GODPredict(Ipv4Address dst);
	Vector Predict(Ipv4Address dst);

	void RefreshPosition(Ipv4Address dst);

	void AddEntry(Ipv4Address id, Vector position, int speed, bool flag = false, uint8_t seq = 0)
	{
		//FIXME Andre
		if(!GetFunction())
		{	//Sou RSU
			m_table.AddEntry(id, position, speed, flag, seq);
		}
		else
		{	//Sou OBU
			if(id == m_ipv4->GetAddress (1, 0).GetLocal ())
			{
				m_table.DeleteEntry(id);
				m_table.AddEntry(id, position, speed, flag, seq);
			}
			else
			{
				m_table.AddEntry(id, position, speed, flag, seq);
			}
		}

		//m_table.PrintTable();
	}

	void DeleteEntry(Ipv4Address id) {
		m_table.DeleteEntry(id);
	}

	Vector RSUSearch(Ipv4Address id);

	/*
	 * @return true if RLS is still in search of the address, false otherwise
	 * if in search for too long stops searching
	 * */
	bool IsInSearch(Ipv4Address id);

	Vector GetPosition(Ipv4Address id);
	int GetSpeed(Ipv4Address id);
	Time GetTime(Ipv4Address id);

	/* Devolve a distancia entre dois vectores */
	double PositionsDistance(Vector from, Vector to);

	void Purge() {m_table.Purge(); }
	void Clear() {m_table.Clear(); }

	Time GetMaxSearchTime(){ return m_maxSearchTime;}
	void SetMaxSearchTime(Time t){m_maxSearchTime = t;}


	SlsLocationService (Time tableLifeTime);
	virtual ~SlsLocationService();

	virtual void DoDispose ();

	/* Serve para limpar o mapa que contem informação sobre os resquest
	 * já enviados/recebidos
	 * FIXME so apagar os outdated, neste momento apaga tudo
	 */
	void PurgePacketMap(){ m_packetSended.clear(); }

	/* Função que vai ver se nao tem nada para ler no socket e posteriormente
	 * invocao um RcvReply ou um RcvRequest para tratar do pacote recebido */
	void RecvSls (Ptr<Socket> socket);
	/* Envia um pedido de localização do no dst com um numero maximo de ttl*/

	void SendReply(Ipv4Address dst, Ipv4Address src, Ipv4Address query, Vector position);

	/* Recebendo um reply o no verifica se é o destino desse reply caso
	 * contrário reenvia a informação */
	void RcvReply (Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);
	void SendLocHello(Ipv4Address dst, Ipv4Address src, Vector position);
	void SendLocUpdate(Ptr<Packet> p, Ipv4Address dst, Ipv4Address src, Vector Position);
	void ReceiveUpdate(Ptr<Packet> p, Ipv4Address dst, Ipv4Address src, Vector Position, int speed);

	void SendQuery(Ipv4Address src, Ipv4Address dst, Ipv4Address query);
	void ReceiveQuery(Ipv4Address src, Ipv4Address dst, Ipv4Address query);
	void ReceiveReply(Ipv4Address src, Ipv4Address dst, Vector pos);
	//Vector ReplyQuery(Ipv4Address dst, Ipv4Address src, Ipv4Address query);
	/* Fazem coisas que nao sei bem o que mas são necessárias para ter
	 * o meu m_ipv4 actualizado */
	void  NotifyInterfaceDown (uint32_t i);
	void  NotifyInterfaceUp (uint32_t i);
	void  NotifyRemoveAddress (uint32_t i, Ipv4InterfaceAddress address);
	void  NotifyAddAddress (uint32_t i, Ipv4InterfaceAddress address);

	// Find socket with local interface address iface
  	Ptr<Socket> FindSocketWithInterfaceAddress (Ipv4InterfaceAddress iface) const;
	// Compara dois vectores
	bool VectorComparator(Vector vec1, Vector vec2){return ((vec1.x == vec2.x) && (vec1.x == vec2.x));}

	/*** Funcoes relativas a se é OBU ou RSU ***/
	void SetFunction(bool f){
		m_function = f;
		NS_LOG_UNCOND("Node " << m_ipv4 << " is " << f );
	}
	// Funcao que devolve function: RSU or OBU. OBU is equal to true, RSU false
	bool GetFunction(){return m_function;}
	Ipv4Address GetMRsu(){ return m_rsu;}
	void SetMRsu(Ipv4Address rsu){m_rsu = rsu;}

	void Print();
	Vector GetMPosRsu(){ return m_posrsu;}
	void SetMPosRsu(Vector posrsu){m_posrsu = posrsu;}

private:
  // Start protocol operation
  void Start ();
  /* Guarda informação numa tabela sobre os dados de um nó, a cache descrita
   * no artigo */
  LocationTable m_table;
  /* Guarda um contador com o SequenceNumber do ultimo pacote enviado */
  uint32_t m_maxNumSeqSended;
  // Raw socket per each IP interface, map socket -> iface address (IP + mask)
  std::map< Ptr<Socket>, Ipv4InterfaceAddress > m_socketAddresses;
  /* Dos pacotes enviados guarda a origem do pacote e o id para evitar reenviar
   * request ou reply que ja tenham sido enviados anteriormente */
  std::map<Ipv4Address, uint32_t> m_packetSended;
  /* Permite fazer um sistema de timeout para que quando envio para os vizinhos
   * se ninguem responder ao fim do timeout manda para todos */
  Timer m_rreqRateLimitTimer;
  Time m_maxSearchTime;
  Ptr<Ipv4> m_ipv4;
  bool m_function; // Function in the context: If is RSU or OBU. RSU is equal to false, OBU true;
  Ipv4Address m_rsu; //Se nao sei nada, foi à minha RSU. É este o valor dela
  Vector m_posrsu; //Posicao da Rsu so para retornar logo
};

}
}
#endif /* SlsLocationService_H */

