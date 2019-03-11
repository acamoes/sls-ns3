
#include "gpsr-ltable.h"
#include "ns3/log.h"

namespace ns3{

	Time LocationTable::GetEntryUpdateTime(Ipv4Address id){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);
		if(i == m_table.end () && !id.IsEqual(i->first))
		{
			return Time(0);
		}
		return i->second.GetTime();
	}

	Vector LocationTable::GetPosition(Ipv4Address id, Vector invVec){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);

		//NS_LOG_UNCOND("LT: GetPosition foi chamado para " << id);

		if(i == m_table.end () && !id.IsEqual(i->first))
		{
			//NS_LOG_UNCOND("GetPosition: " << id << " not found");
			return invVec;
		}

//		Ipv4Address ip = i->first;
		Vector position = i->second.GetPosition();
		Time time = i->second.GetTime();
		int speed = i->second.GetSpeed();
//		bool flag = i->second.GetResearchFlag();
//		uint8_t seq = i->second.GetSeqNumber();

		//Isto é o Predict!
		if(Simulator::Now().GetSeconds() > time.GetSeconds())
		{
			int time2 = (Simulator::Now().GetSeconds() - time.GetSeconds());
			int speed2 = (time2 * speed);

			NS_LOG_UNCOND("Time2 " << time2);
			NS_LOG_UNCOND("Speed2 " << speed2);

			//Tenho de pensar neste valor, pode ser algo adaptativo
			int compensation = 200;

			Vector3D position2 = Vector3D(position.x+speed2+compensation, position.y+speed2+compensation, position.z);

			NS_LOG_UNCOND("Position2 " << position2);

			return position2;
		}

		return position;
	}

	void LocationTable::SetTime(Ipv4Address id,Time time){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);
		if(i == m_table.end () && !id.IsEqual(i->first))
		{
			return;
		}
		i->second.SetTime(time);
	}

	void LocationTable::SetResearchFlag(Ipv4Address id,bool value){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);
		if(i == m_table.end () && !id.IsEqual(i->first))
		{
			return;
		}
		i->second.SetResearchFlag(value);
	}

	void LocationTable::SetSeqNumber(Ipv4Address id, int num){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);
		if(i == m_table.end () && !id.IsEqual(i->first))
		{
			return;
		}
		i->second.SetSeqNumber(num);
	}

	Time LocationTable::GetTime(Ipv4Address id){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);
		if(i == m_table.end () && !id.IsEqual(i->first))
		{
			return Time(0);
		}
		return i->second.GetTime();
	}

	int LocationTable::GetSpeed(Ipv4Address id){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);
		if(i == m_table.end () && !id.IsEqual(i->first))
		{
			return 0;
		}
		return i->second.GetSpeed();
	}

	bool LocationTable::GetResearchFlag(Ipv4Address id){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);
		if(i == m_table.end () && !id.IsEqual(i->first))
		{
			return false;
		}
		return i->second.GetResearchFlag();
	}

	int LocationTable::GetSeqNumber(Ipv4Address id){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);
		if(i == m_table.end () && !id.IsEqual(i->first))
		{
			return 0;
		}
		return i->second.GetSeqNumber();

	}

	MapEntry::MapEntry(Vector position, Time time, int speed, bool researchFlag , uint8_t numSeq):
	 m_position(position), m_time(time), m_speed(speed), m_researchFlag(researchFlag), m_seqNumber(numSeq)
	{}

	void LocationTable::AddEntry(Ipv4Address id, Vector position, int speed, bool flag, uint8_t seq){
		std::map<Ipv4Address, MapEntry >::iterator i = m_table.find(id);
		if(i != m_table.end () || id.IsEqual(i->first))
		{
			//FIXME [AC] Nao pode ser por speed - Isto existe para ele nao inserir depois de ja ter speed fixe
			if(speed != 0)
			{
				m_table.erase(id);
				m_table.insert(std::make_pair(id, MapEntry(position, Simulator::Now(),speed, flag, seq)));
			}
			else
			{
				m_table.insert(std::make_pair(id, MapEntry(position, Simulator::Now(),speed, flag, seq)));
			}

			//PrintTable();
			return;
		}

		NS_LOG_UNCOND("AddEntry_table new: " << id << " " << position << " flag " << flag);
		m_table.insert(std::make_pair(id, MapEntry(position, Simulator::Now(), speed, flag, seq)));
		//PrintTable();
	}

	void LocationTable::Refresh()
	{
		for(std::map<Ipv4Address, MapEntry>::iterator i=m_table.begin();i!=m_table.end(); ++i)
		{
			Ipv4Address ip = (*i).first;
			Vector position = (*i).second.GetPosition();
			Time time = (*i).second.GetTime();
			int speed = (*i).second.GetSpeed();
			bool flag = (*i).second.GetResearchFlag();
			uint8_t seq = (*i).second.GetSeqNumber();

			int time2 = (Simulator::Now().GetSeconds() - time.GetSeconds());
			int speed2 = (time2 * speed);

			//DeleteEntry(ip);
			AddEntry(ip, position, speed2, flag, seq);
		}
	}


	void LocationTable::PrintTable(Ipv4Address id)
	{
		NS_LOG_UNCOND("== LOCATION_TABLE " << id << " ====================================================");
		
		//[AC] Iterate map to print all entries in the table
		for(std::map<Ipv4Address, MapEntry>::iterator i=m_table.begin();i!=m_table.end(); ++i)
		{

			//int time = (Simulator::Now().GetSeconds() - (*i).second.GetTime().GetSeconds());

			int speed = (*i).second.GetSpeed();
			//int speed = (time * (*i).second.GetSpeed());

			NS_LOG_UNCOND(" Nó " << (*i).first << " Position " << (*i).second.GetPosition()
					<< " Time " << (*i).second.GetTime() << " Speed " << speed << " ResearchFlag "<< (*i).second.GetResearchFlag() << " SeqNumber " <<
					(*i).second.GetSeqNumber());
		}

		NS_LOG_UNCOND("======================================================================");
	}

	void LocationTable::DeleteEntry(Ipv4Address id){
		m_table.erase(id);
		NS_LOG_UNCOND("Deleted entry " << id << " in m_table");
	}

	void LocationTable::Purge(){
//		if(1){
//			NS_LOG_UNCOND("Purge da LTABLE");//apparently not being called
//			return;
//		}
		NS_LOG_UNCOND("Purge na location table");
		std::map<Ipv4Address, MapEntry >::iterator i;
		for(i = m_table.begin(); !(i == m_table.end()); i++)
		{
			if(m_entryLifeTime + GetEntryUpdateTime(i->first) <= Simulator::Now())
			{
				m_table.erase(i->first); //FIXME cannot erase on list being iterated
			}	
		}
		/* last element is not used in for cicle*/
		if(m_entryLifeTime + GetEntryUpdateTime(i->first) <= Simulator::Now())
		{
			m_table.erase(i->first);
		}
	}

	void LocationTable::Clear(){
		m_table.clear ();
	}

}
