#ifndef SLS_PTABLE_H
#define SLS_PTABLE_H

#include "ns3/timer.h"
#include "ns3/vector.h"
#include "ns3/ipv4.h"
#include <map>

namespace ns3{

//Representa cada entrada da tabela
class MapEntry{

public:
	MapEntry(Vector position, Time time, int speed, bool researchFlag, uint8_t numSeq);

	void SetPosition(Vector position) { m_position = position; }
	void SetTime(Time time) { m_time = time; }
	void SetResearchFlag(bool value) { m_researchFlag = value; }
	void SetSeqNumber(int num) { m_seqNumber = num;}
	void SetSpeed(int speed){m_speed = speed;}

	Vector GetPosition() { return m_position; }
	Time GetTime() { return m_time; }
	bool GetResearchFlag() { return m_researchFlag; }
	int GetSeqNumber(){ return m_seqNumber;}
	int GetSpeed(){return m_speed;}


private:

	Vector m_position;
	Time m_time;
	int m_speed;
	bool m_researchFlag;//When m_researchFlag true this is the time when the research starts
	uint8_t m_seqNumber;

};

class LocationTable{
public:

	LocationTable(Time Lifetime) : m_entryLifeTime (m_entryLifeTime) {}
	
	/* O seguinte conjunto de funções permite que tendo acesso ao mapa
	 * possa os valores de cada entrada sem ter que aceder ao 2º membro
	 * do mapa 
	 */
	Time GetEntryUpdateTime(Ipv4Address id);

	Ipv4Address GetRsu(Ipv4Address id);
	void SetRsu(Ipv4Address id, Ipv4Address rsu);

	int GetSpeed(Ipv4Address id);
	void SetSpeed(Ipv4Address id, int speed);


	Vector GetPosition(Ipv4Address id, Vector invVec);
	void SetPosition(Ipv4Address id, Vector position);

	void SetTime(Ipv4Address id,Time time);
	void SetResearchFlag(Ipv4Address id,bool value);
	void SetSeqNumber(Ipv4Address id, int num);

	Time GetTime(Ipv4Address id);

	bool GetResearchFlag(Ipv4Address id);
	int GetSeqNumber(Ipv4Address id);
	void PrintTable(Ipv4Address id);

	/* Permite adicionar entradas à tabela tem alguns valores por omissão
	 * pois quando o protocolo de Routing pede para adicionar algo nao 
	 * conhecimentos desses valores, bastando mandar os dois primeiros.
	 */
	void AddEntry(Ipv4Address id, Vector position, int speed, bool flag = false, uint8_t seq = 0);
	void DeleteEntry(Ipv4Address id);

	void Refresh();
	void Purge();
	void Clear();

private:
	Time m_entryLifeTime;
	std::map<Ipv4Address, MapEntry> m_table;

}; 
} 
#endif /* SLS_PTABLE_H */
