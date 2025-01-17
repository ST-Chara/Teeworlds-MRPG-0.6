/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_BOT_DATA_H
#define GAME_SERVER_COMPONENT_BOT_DATA_H

#include <game/server/mmocore/Components/Inventory/ItemData.h>
#include "DialogsData.h"

#include <game/server/mmocore/Utils/DBSet.h>

/************************************************************************/
/*  Global data information bot                                         */
/************************************************************************/
class DataBotInfo
{
public:
	char m_aNameBot[MAX_NAME_LENGTH] {};
	CTeeInfo m_TeeInfos {};
	int m_aEquipSlot[NUM_EQUIPPED] {};
	bool m_aVisibleActive[MAX_PLAYERS] {};
	DBSet m_EquippedModules {};

	static bool IsDataBotValid(int BotID) { return (ms_aDataBot.find(BotID) != ms_aDataBot.end()); }
	static std::map<int, DataBotInfo> ms_aDataBot;

	static class MobBotInfo* FindMobByBot(int BotID);
};

/************************************************************************/
/*  Global data npc bot                                                 */
/************************************************************************/
class NpcBotInfo
{
public:
	bool m_Static {};
	vec2 m_Position {};
	int m_Emote {};
	int m_WorldID {};
	int m_BotID {};
	int m_Function {};
	int m_GiveQuestID {};
	std::vector<CDialogElem> m_aDialogs {};

	const char* GetName() const { return DataBotInfo::ms_aDataBot[m_BotID].m_aNameBot; }
	static bool IsValid(int MobID) { return ms_aNpcBot.find(MobID) != ms_aNpcBot.end() && DataBotInfo::IsDataBotValid(ms_aNpcBot[MobID].m_BotID); }
	static std::map<int, NpcBotInfo> ms_aNpcBot;
};

/************************************************************************/
/*  Global data quest bot                                               */
/************************************************************************/
class QuestBotInfo
{
	bool m_AutoCompletesQuestStep {};

public:
	char m_aGeneratedNickname[MAX_NAME_LENGTH] {};
	vec2 m_Position {};
	int m_QuestID {};
	int m_Step {};
	int m_WorldID {};
	int m_BotID {};
	int m_SubBotID {};
	bool m_GenerateNick {};
	bool m_HasAction {};
	std::string m_EventJsonData {};
	std::vector<CDialogElem> m_aDialogs {};

	CItemsContainer m_RewardItems;

	struct TaskRequiredItems
	{
		enum class Type : short
		{
			DEFAULT,
			PICKUP,
			SHOW,
		};

		CItem m_Item {};
		Type m_Type {};
	};
	std::deque < TaskRequiredItems > m_RequiredItems;

	struct TaskRequiredDefeat
	{
		int m_BotID {};
		int m_Value {};
	};
	std::deque < TaskRequiredDefeat > m_RequiredDefeat;

	struct TaskRequiredMoveTo
	{
		enum Types : unsigned int
		{
			// Define each value in the enumeration with a unique identifier and bit position
			EMPTY = 0,
			MOVE_ONLY = 1 << 0,          // 00000001
			PICKUP_ITEM = 1 << 1,        // 00000010
			REQUIRED_ITEM = 1 << 2,      // 00000100
			DEFEAT_MOB = 1 << 3,         // 00001000
			INTERACTIVE = 1 << 4,        // 00010000

			// Combine multiple values using bitwise OR operation
			DEFEAT_MOB_PICKUP = DEFEAT_MOB | PICKUP_ITEM,             // 00001010
			INTERACTIVE_PICKUP = INTERACTIVE | PICKUP_ITEM,           // 00010010
			INTERACTIVE_REQUIRED = INTERACTIVE | REQUIRED_ITEM        // 00010100
		};

		struct DefeatMob
		{
			int m_BotID {};
			int m_AttributePower {};
			int m_AttributeSpread {};
			int m_WorldID {};
		};

		struct Interaction
		{
			vec2 m_Position {};
		};

		vec2 m_Position {};
		int m_WorldID {};
		int m_Step {};
		bool m_Navigator {};
		unsigned int m_Type {};
		CItem m_PickupItem {};
		CItem m_RequiredItem {};
		std::string m_CompletionText {};
		std::string m_TaskName {};
		int m_Cooldown {};

		int m_QuestBotID {};
		DefeatMob m_DefeatMobInfo {};
		Interaction m_Interaction {};

		bool IsHasDefeatMob() const { return m_DefeatMobInfo.m_BotID >= 1; };
	};

	std::deque < TaskRequiredMoveTo > m_RequiredMoveTo;
	bool IsAutoCompletesQuestStep() const { return m_AutoCompletesQuestStep; }


	const char* GetName() const { return DataBotInfo::ms_aDataBot[m_BotID].m_aNameBot; }
	static bool IsValid(int MobID) { return ms_aQuestBot.find(MobID) != ms_aQuestBot.end() && DataBotInfo::IsDataBotValid(ms_aQuestBot[MobID].m_BotID); }
	void InitTasks(const std::string& JsonData);

	static std::map<int, QuestBotInfo> ms_aQuestBot;
};

/************************************************************************/
/*  Global data mob bot                                                 */
/************************************************************************/

class CMobBuffDebuff
{
	float m_Chance {};
	std::string m_Effect {};
	std::tuple<int, int> m_Time {};

public:
	CMobBuffDebuff() = default;
	CMobBuffDebuff(float Chance, std::string Effect, std::tuple<int, int> Time) : m_Chance(Chance), m_Effect(Effect), m_Time(Time) {}

	enum
	{
		SECONDS,
		RANGE
	};

	const char* getEffect() const { return m_Effect.c_str(); }
	int getTime() const
	{
		int Range = std::get<RANGE>(m_Time);
		int Time = std::get<SECONDS>(m_Time) - Range / 2;
		return Time + rand() % Range;
	}
	float getChance() const { return m_Chance; }
};

class MobBotInfo
{
	friend class CBotManager;
	char m_aBehavior[512] {};

	std::deque < CMobBuffDebuff > m_Effects;

public:
	bool m_Boss {};
	int m_Power {};
	int m_Spread {};
	vec2 m_Position;
	int m_Level {};
	int m_RespawnTick {};
	int m_WorldID {};
	int m_aDropItem[MAX_DROPPED_FROM_MOBS] {};
	int m_aValueItem[MAX_DROPPED_FROM_MOBS] {};
	float m_aRandomItem[MAX_DROPPED_FROM_MOBS] {};
	int m_BotID {};

	const char* GetName() const
	{
		return DataBotInfo::ms_aDataBot[m_BotID].m_aNameBot;
	}

	std::deque < CMobBuffDebuff >& GetEffects()
	{
		return m_Effects;
	}

	[[nodiscard]] CMobBuffDebuff* GetRandomEffect()
	{
		return m_Effects.empty() ? nullptr : &m_Effects[rand() % m_Effects.size()];
	}

	void InitDebuffs(int Seconds, int Range, float Chance, std::string& buffSets);

	bool IsIncludedBehavior(const char* pBehavior) const
	{
		return str_find(m_aBehavior, pBehavior) != nullptr;
	}

	static bool IsValid(int MobID)
	{
		return ms_aMobBot.find(MobID) != ms_aMobBot.end() && DataBotInfo::IsDataBotValid(ms_aMobBot[MobID].m_BotID);
	}

	static std::map<int, MobBotInfo> ms_aMobBot;
};

#endif
