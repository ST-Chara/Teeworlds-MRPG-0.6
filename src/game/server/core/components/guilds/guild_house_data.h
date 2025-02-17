/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_COMPONENT_GUILD_HOUSE_DATA_H
#define GAME_SERVER_COMPONENT_GUILD_HOUSE_DATA_H

#include <game/server/core/components/houses/farmzone_manager.h>

#define TW_GUILDS_HOUSES "tw_guilds_houses"
#define TW_GUILD_HOUSES_DECORATION_TABLE "tw_guilds_decorations"
static constexpr float s_GuildChancePlanting = 0.025f;

class CGS;
class CPlayer;
class CGuild;
class CDrawingData;
class CEntityHouseDecoration;
class CEntityDrawboard;
class CEntityGuildDoor;
class EntityPoint;
class CEntityGatheringNode;
using GuildHouseIdentifier = int;

class CGuildHouse : public MultiworldIdentifiableData< std::deque < CGuildHouse* > >
{
	friend class CGuild;
	friend class CGuildHouseDoorManager;
	friend class CGuildHouseDecorationManager;

public:

	/* -------------------------------------
	 * Decorations impl
	 * ------------------------------------- */
	using DecorationIdentifier = int;
	using DecorationsContainer = std::vector<CEntityHouseDecoration*>;
	class CDecorationManager
	{
		CGS* GS() const;
		CEntityDrawboard* m_pDrawBoard {};
		CGuildHouse* m_pHouse {};

	public:
		CDecorationManager() = delete;
		CDecorationManager(CGuildHouse* pHouse);
		~CDecorationManager();

		bool StartDrawing(CPlayer* pPlayer) const;
		bool EndDrawing(CPlayer* pPlayer);
		bool HasFreeSlots() const;

	private:
		void Init();
		static bool DrawboardToolEventCallback(DrawboardToolEvent Event, CPlayer* pPlayer, const EntityPoint* pPoint, void* pUser);
		bool Add(const EntityPoint* pPoint) const;
		bool Remove(const EntityPoint* pPoint) const;
	};

	/* -------------------------------------
	 * Doors impl
	 * ------------------------------------- */
	class CDoorManager
	{
		CGS* GS() const;
		CGuildHouse* m_pHouse {};
		ska::unordered_map<int, CEntityGuildDoor*> m_apEntDoors {};

	public:
		CDoorManager() = delete;
		CDoorManager(CGuildHouse* pHouse, std::string&& JsonDoors);
		~CDoorManager();

		ska::unordered_map<int, CEntityGuildDoor*>& GetContainer() { return m_apEntDoors; }
		void Open(int Number); // Open a specific door
		void Close(int Number); // Close a specific door
		void Reverse(int Number); // Reverse the state of a specific door
		void OpenAll(); // Open all doors
		void CloseAll(); // Close all doors
		void ReverseAll(); // Reverse the state of all doors
		void AddDoor(const char* pDoorname, vec2 Position);
		void RemoveDoor(const char* pDoorname, vec2 Position);
	};

private:
	CGS* GS() const;

	CGuild* m_pGuild {};
	GuildHouseIdentifier m_ID{};
	float m_Radius {};
	vec2 m_Position{};
	vec2 m_TextPosition{};
	int m_InitialFee {};
	int m_WorldID{};
	int m_RentDays {};

	CDoorManager* m_pDoors {};
	CDecorationManager* m_pDecorationManager {};
	CFarmzonesManager* m_pFarmzonesManager {};

public:
	CGuildHouse() = default;
	~CGuildHouse();

	static CGuildHouse* CreateElement(const GuildHouseIdentifier& ID)
	{
		auto pData = new CGuildHouse;
		pData->m_ID = ID;
		return m_pData.emplace_back(std::move(pData));
	}

	void Init(CGuild* pGuild, int RentDays, int InitialFee, int WorldID, std::string&& JsonDoors, std::string&& JsonFarmzones, std::string&& JsonProperties)
	{
		m_InitialFee = InitialFee;
		m_RentDays = RentDays;
		m_WorldID = WorldID;

		InitProperties(std::move(JsonDoors), std::move(JsonFarmzones), std::move(JsonProperties));
		UpdateGuild(pGuild);
	}

	void InitProperties(std::string&& JsonDoors, std::string&& JsonFarmzones, std::string&& JsonProperties);

	CGuild* GetGuild() const { return m_pGuild; }
	CDoorManager* GetDoorManager() const { return m_pDoors; }
	CDecorationManager* GetDecorationManager() const { return m_pDecorationManager; }
	CFarmzonesManager* GetFarmzonesManager() const { return m_pFarmzonesManager; }

	GuildHouseIdentifier GetID() const { return m_ID; }
	vec2 GetPos() const { return m_Position; }
	float GetRadius() const { return m_Radius; }
	int GetWorldID() const { return m_WorldID; }
	int GetInitialFee() const { return m_InitialFee; }
	int GetRentPrice() const;
	int GetRentDays() const { return m_RentDays; }
	bool IsPurchased() const { return m_pGuild != nullptr; }
	const char* GetOwnerName() const;

	bool ExtendRentDays(int Days);
	bool ReduceRentDays(int Days);
	void UpdateText(int Lifetime) const;
	void UpdateGuild(CGuild* pGuild);
	void Save();
};

#endif
