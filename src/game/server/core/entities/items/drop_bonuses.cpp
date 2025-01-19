/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "drop_bonuses.h"

#include <game/server/gamecontext.h>

CEntityDropBonuses::CEntityDropBonuses(CGameWorld *pGameWorld, vec2 Pos, vec2 Vel, int Type, int Subtype, int Value)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_BONUS_DROP, Pos, 24), m_Vel(Vel), m_Type(Type), m_Subtype(Subtype)
{
	m_Value = Value;
	m_LifeSpan = Server()->TickSpeed() * 15;
	GameWorld()->InsertEntity(this);
}

void CEntityDropBonuses::Tick()
{
	m_LifeSpan--;
	if (m_LifeSpan < 0)
	{
		GS()->CreatePlayerSpawn(m_Pos);
		GameWorld()->DestroyEntity(this);
		return;
	}

	m_Flash.Tick(m_LifeSpan);
	GS()->Collision()->MovePhysicalBox(&m_Pos, &m_Vel, vec2(m_Radius, m_Radius), 0.5f);

	// interactive
	CCharacter *pChar = (CCharacter*)GameWorld()->ClosestEntity(m_Pos, 16.0f, CGameWorld::ENTTYPE_CHARACTER, nullptr);
	if(pChar && !pChar->GetPlayer()->IsBot())
	{
		// health
		if (m_Type == POWERUP_HEALTH)
		{
			GS()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH);
		}

		// experience
		else if (m_Type == POWERUP_ARMOR)
		{
			pChar->GetPlayer()->Account()->AddExperience(m_Value);
			GS()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
		}

		// weapons
		else if (m_Type == POWERUP_WEAPON)
		{
			// shotgun
			if (m_Subtype == WEAPON_SHOTGUN && pChar->GetPlayer()->IsEquipped(ItemType::EquipShotgun))
			{
				pChar->GiveWeapon(m_Subtype, m_Value);
				GS()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
			}
			// grenade
			else if (m_Subtype == WEAPON_GRENADE && pChar->GetPlayer()->IsEquipped(ItemType::EquipGrenade))
			{
				pChar->GiveWeapon(m_Subtype, m_Value);
				GS()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE);
			}
			// laser
			else if (m_Subtype == WEAPON_LASER && pChar->GetPlayer()->IsEquipped(ItemType::EquipLaser))
			{
				pChar->GiveWeapon(m_Subtype, m_Value);
				GS()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN);
			}
		}

		GameWorld()->DestroyEntity(this);
	}
}

void CEntityDropBonuses::Snap(int SnappingClient)
{
	if(m_Flash.IsFlashing() || NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, GetID(), sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = m_Type;
	pP->m_Subtype = m_Subtype;
}