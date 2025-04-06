/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_EVENTHANDLER_H
#define GAME_SERVER_EVENTHANDLER_H

class CEventHandler
{
	static const int MAX_EVENTS = 128;
	static const int MAX_DATASIZE = 128 * 64;

	int m_aTypes[MAX_EVENTS]; // TODO: remove some of these arrays
	int m_aOffsets[MAX_EVENTS];
	int m_aSizes[MAX_EVENTS];
	int64_t m_aClientMasks[MAX_EVENTS];
	char m_aData[MAX_DATASIZE];

	class CGS* m_pGameServer;
	int m_CurrentOffset;
	int m_NumEvents;

public:
	CGS* GS() const { return m_pGameServer; }
	void SetGameServer(CGS* pGameServer);

	CEventHandler();

	template<typename T>
	T* Create(int64_t Mask = -1) { return static_cast<T*>(Create(T::ms_MsgId, sizeof(T), Mask)); }
	void* Create(int Type, int Size, int64_t Mask = -1);
	void Clear();
	void Snap(int SnappingClient);
};

#endif