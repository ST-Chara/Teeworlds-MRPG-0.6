/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "drop_quest_items.h"

#include <game/server/gamecontext.h>

CDropQuestItem::CDropQuestItem(CGameWorld* pGameWorld, vec2 Pos, vec2 Vel, float AngleForce, int ItemID, int Needed, int QuestID, int Step, int ClientID)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_QUEST_DROP, Pos, 24.0f)
{
	m_Pos = Pos;
	m_Vel = Vel;
	m_Angle = 0.0f;
	m_AngleForce = AngleForce;
	m_ClientID = ClientID;
	m_ItemID = ItemID;
	m_Needed = Needed;
	m_QuestID = QuestID;
	m_Step = Step;
	m_LifeSpan = Server()->TickSpeed() * 60;

	GameWorld()->InsertEntity(this);
	for (int& m_ID : m_IDs)
	{
		m_ID = Server()->SnapNewID();
	}
}

CDropQuestItem::~CDropQuestItem()
{
	for(const int m_ID : m_IDs)
	{
		Server()->SnapFreeID(m_ID);
	}
}

void CDropQuestItem::Tick()
{
	CPlayer* pPlayer = GS()->GetPlayer(m_ClientID);

	m_LifeSpan--;
	if (m_LifeSpan < 0 || !pPlayer)
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	m_Flash.Tick(m_LifeSpan);
	GS()->Collision()->MovePhysicalAngleBox(&m_Pos, &m_Vel, vec2(m_Radius, m_Radius), &m_Angle, &m_AngleForce, 0.5f);

	CPlayerItem* pItem = pPlayer->GetItem(m_ItemID);
	const CPlayerQuest* pQuest = pPlayer->GetQuest(m_QuestID);
	if(pQuest->GetState() != QuestState::Accepted || pQuest->GetStepPos() != m_Step || pItem->GetValue() >= m_Needed)
	{
		GameWorld()->DestroyEntity(this);
		return;
	}

	// pickup
	if (pPlayer->GetCharacter() && distance(m_Pos, pPlayer->GetCharacter()->m_Core.m_Pos) < 32.0f)
	{
		if(Server()->Input()->IsKeyClicked(m_ClientID, KEY_EVENT_FIRE_HAMMER))
		{
			pItem->Add(1);
			GS()->Chat(m_ClientID, "You got '{}'.", pItem->Info()->GetName());
			GameWorld()->DestroyEntity(this);
			return;
		}

		GS()->Broadcast(m_ClientID, BroadcastPriority::GameInformation, 10, "Press hammer 'Fire', to pick up an item");
	}
}


void CDropQuestItem::Snap(int SnappingClient)
{
	if(m_Flash.IsFlashing() || m_ClientID != SnappingClient || NetworkClipped(SnappingClient))
		return;

	// vanilla box
	CNetObj_Projectile* pProj = static_cast<CNetObj_Projectile*>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetID(), sizeof(CNetObj_Projectile)));
	if (pProj)
	{
		pProj->m_X = (int)m_Pos.x;
		pProj->m_Y = (int)m_Pos.y;
		pProj->m_VelX = 0;
		pProj->m_VelY = 0;
		pProj->m_StartTick = Server()->Tick();
		pProj->m_Type = WEAPON_HAMMER;
	}

	static const float Radius = 16.0f;
	const float AngleStep = 2.0f * pi / (float)CDropQuestItem::NUM_IDS;
	const float AngleStart = (pi / (float)CDropQuestItem::NUM_IDS) + (2.0f * pi * m_Angle);
	for(int i = 0; i < CDropQuestItem::NUM_IDS; i++)
	{
		CNetObj_Laser *pRifleObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_IDs[i], sizeof(CNetObj_Laser)));
		if(!pRifleObj)
			return;

		vec2 Pos = m_Pos + vec2(Radius * cos(AngleStart + AngleStep * i), Radius * sin(AngleStart + AngleStep * i));
		vec2 PosTo = m_Pos + vec2(Radius * cos(AngleStart + AngleStep * (i+1)), Radius * sin(AngleStart + AngleStep * (i+1)));
		pRifleObj->m_X = (int)Pos.x;
		pRifleObj->m_Y = (int)Pos.y;
		pRifleObj->m_FromX = (int)PosTo.x;
		pRifleObj->m_FromY = (int)PosTo.y;
		pRifleObj->m_StartTick = Server()->Tick() - 4;
	}
}
