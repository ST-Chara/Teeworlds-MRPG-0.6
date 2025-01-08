/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_PATH_NAVIGATOR_POINT_H
#define GAME_SERVER_ENTITIES_PATH_NAVIGATOR_POINT_H

#include <game/server/entity.h>

class CPlayer;
class CEntityPathNavigator : public CEntity
{
	PathRequestHandle m_PathHandle{};
	bool m_StartByCreating{};
	int m_StepPos {};
	vec2 m_LastPos {};
	int64_t m_Mask {};
	int m_TickLastIdle {};
	int m_TickCountDown {};
	bool m_Projectile {};

public:
	CPlayer* GetPlayer() const;
	CEntityPathNavigator(CGameWorld* pGameWorld, int ClientID, bool StartByCreating, vec2 SearchPos, int WorldID, bool Projectile, int64_t Mask = -1);

	void Tick() override;
	void Snap(int SnappingClient) override;
	void TickDeferred() override;

private:
	void Move(CPlayer* pPlayer);
};

#endif
