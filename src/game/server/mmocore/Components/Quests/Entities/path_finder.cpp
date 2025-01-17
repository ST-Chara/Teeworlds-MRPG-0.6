/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "path_finder.h"

#include <game/server/mmocore/Components/Worlds/WorldManager.h>
#include <game/server/gamecontext.h>

#include <game/server/mmocore/GameEntities/Tools/path_navigator.h>

CEntityPathFinder::CEntityPathFinder(CGameWorld* pGameWorld, vec2 SearchPos, int WorldID, int ClientID, float AreaClipped, bool* pComplete, std::deque < CEntityPathFinder* >* apCollection)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_FINDQUEST, SearchPos)
{
	vec2 GetterPos{0,0};
	GS()->Mmo()->WorldSwap()->FindPosition(WorldID, SearchPos, &GetterPos);

	m_PosTo = GetterPos;
	m_ClientID = ClientID;
	m_AreaClipped = AreaClipped;
	m_WorldID = WorldID;
	m_pPlayer = GS()->GetPlayer(m_ClientID, true, true);
	m_pComplete = pComplete;
	m_apCollection = apCollection;
	GameWorld()->InsertEntity(this);

	// quest navigator finder
	if(m_pPlayer && m_pPlayer->GetItem(itShowQuestNavigator)->IsEquipped())
	{
		new CEntityPathNavigator(&GS()->m_World, this, true, m_pPlayer->m_ViewPos, SearchPos, WorldID, true, CmaskOne(ClientID));
	}
}

CEntityPathFinder::~CEntityPathFinder()
{
	if(m_apCollection && !m_apCollection->empty())
	{
		for(auto it = m_apCollection->begin(); it != m_apCollection->end(); ++it)
		{
			if(mem_comp((*it), this, sizeof(CEntityPathFinder)) == 0)
			{
				m_apCollection->erase(it);
				break;
			}
		}
	}
}

void CEntityPathFinder::Tick()
{
	if(!m_pPlayer || !m_pPlayer->GetCharacter() || !length_squared(m_PosTo))
	{
		GameWorld()->DestroyEntity(this);
		return;
	}
	
	if (!m_pComplete || (*m_pComplete) == true)
	{
		GS()->CreatePlayerSpawn(m_Pos, CmaskOne(m_ClientID));
		GameWorld()->DestroyEntity(this);
	}
}

void CEntityPathFinder::ClearPointers()
{
	m_apCollection = nullptr;
}

void CEntityPathFinder::Snap(int SnappingClient)
{
	if(m_ClientID != SnappingClient || !m_pPlayer || !m_pPlayer->GetCharacter() || (m_AreaClipped > 1.f && distance(m_PosTo, m_pPlayer->m_ViewPos) < m_AreaClipped))
		return;

	CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if(pPickup)
	{
		vec2 CorePos = m_pPlayer->GetCharacter()->m_Core.m_Pos;
		m_Pos = CorePos;
		m_Pos -= normalize(CorePos - m_PosTo) * clamp(distance(m_Pos, m_PosTo), 32.0f, 90.0f);

		pPickup->m_X = (int)m_Pos.x;
		pPickup->m_Y = (int)m_Pos.y;
		pPickup->m_Type = POWERUP_ARMOR;
		pPickup->m_Subtype = 0;
	}
}