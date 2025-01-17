/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "tutorial.h"

#include <game/server/gamecontext.h>

#include "game/server/mmocore/Components/Tutorial/TutorialManager.h"

CGameControllerTutorial::CGameControllerTutorial(class CGS* pGS)
	: IGameController(pGS)
{
	m_GameFlags = 0;
}

void CGameControllerTutorial::Tick()
{
	IGameController::Tick();

	// handle tutorial world
	for(int i = 0; i < MAX_PLAYERS; i++)
	{
		if(GS()->IsPlayerEqualWorld(i))
		{
			if(CPlayer* pPlayer = GS()->GetPlayer(i, true, true); pPlayer)
			{
				CTutorialManager* pTutorialManager = GS()->Mmo()->Tutorial();
				pTutorialManager->HandleTutorial(pPlayer);

				if(pPlayer->m_TutorialStep >= pTutorialManager->GetSize() && !pPlayer->GetItem(itAdventurersBadge)->HasItem())
					pPlayer->GetItem(itAdventurersBadge)->Add(1);
			}
		}
	}
}

bool CGameControllerTutorial::OnCharacterSpawn(CCharacter* pChr)
{
	return IGameController::OnCharacterSpawn(pChr);
}

void CGameControllerTutorial::Snap()
{
	// vanilla snap
	CNetObj_GameInfo* pGameInfoObj = (CNetObj_GameInfo*)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	pGameInfoObj->m_RoundStartTick = 0;
	pGameInfoObj->m_WarmupTimer = 0;
	pGameInfoObj->m_RoundNum = 0;
	pGameInfoObj->m_RoundCurrent = 1;

	// ddnet snap
	CNetObj_GameInfoEx* pGameInfoEx = (CNetObj_GameInfoEx*)Server()->SnapNewItem(NETOBJTYPE_GAMEINFOEX, 0, sizeof(CNetObj_GameInfoEx));
	if(!pGameInfoEx)
		return;

	pGameInfoEx->m_Flags = GAMEINFOFLAG_GAMETYPE_PLUS | GAMEINFOFLAG_ALLOW_EYE_WHEEL | GAMEINFOFLAG_ALLOW_HOOK_COLL | GAMEINFOFLAG_PREDICT_VANILLA;
	pGameInfoEx->m_Flags2 = GAMEINFOFLAG2_GAMETYPE_CITY | GAMEINFOFLAG2_ALLOW_X_SKINS | GAMEINFOFLAG2_HUD_DDRACE;
	pGameInfoEx->m_Version = GAMEINFO_CURVERSION;
}
