/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_QUEST_DATA_H
#define GAME_SERVER_COMPONENT_QUEST_DATA_H

#define QUEST_PREFIX_DEBUG "quest_system"

#include "QuestDataInfo.h"

class CPlayerQuest : public MultiworldIdentifiableStaticData< std::map < int, std::map <int, CPlayerQuest > > >
{
	int m_ClientID {};
	QuestIdentifier m_ID {};
	QuestState m_State {};
	int m_Step {};

	std::map < int, CPlayerQuestStep > m_aPlayerSteps {};
	std::deque < class CStepPathFinder* > m_apEntityNPCNavigator{};

	class CGS* GS() const;
	class CPlayer* GetPlayer() const;

public:
	friend class CQuestManager;

	CPlayerQuest() = default;
	CPlayerQuest(QuestIdentifier ID, int ClientID) : m_ClientID(ClientID) { m_ID = ID; }
	~CPlayerQuest();

	void Init(QuestState State)
	{
		m_State = State;
		m_pData[m_ClientID][m_ID] = *this;
		m_pData[m_ClientID][m_ID].LoadSteps();
	}

	CQuestDescription* Info() const;
	std::string GetJsonFileName() const;
	QuestIdentifier GetID() const { return m_ID; }
	QuestState GetState() const { return m_State; }
	bool IsCompleted() const { return m_State == QuestState::FINISHED; }
	bool IsActive() const { return m_State == QuestState::ACCEPT; }

	// steps
	void InitSteps();
	void LoadSteps();
	bool SaveSteps();
	void ClearSteps();
	int GetCurrentStepPos() const { return m_Step; }
	CPlayerQuestStep* GetStepByMob(int MobID) { return &m_aPlayerSteps[MobID]; }

	// steps path finder tools
	class CStepPathFinder* FoundEntityNPCNavigator(int SubBotID) const;
	class CStepPathFinder* AddEntityNPCNavigator(class QuestBotInfo* pBot);

	// main
	void CheckAvailableNewStep();
	bool Accept();
	void Refuse();
	void Reset();

private:
	void Finish();
};

#endif