/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/gamecontext.h>

#include "achievement_data.h"

CGS* CAchievement::GS() const { return (CGS*)Instance::GameServerPlayer(m_ClientID); }
CPlayer* CAchievement::GetPlayer() const { return GS()->GetPlayer(m_ClientID); }

void CAchievementInfo::InitData(const std::string& RewardData)
{
	// parse the reward data
	mystd::json::parse(RewardData, [this](nlohmann::json& pJson)
	{
		m_RewardData = std::move(pJson);
	});

	// initialize achievement group
	m_Group = ACHIEVEMENT_GROUP_GENERAL;
	switch(m_Type)
	{
		case AchievementType::DefeatPVP:
		case AchievementType::DefeatPVE:
		case AchievementType::DefeatMob:
		case AchievementType::Death:
		case AchievementType::TotalDamage:
			m_Group = ACHIEVEMENT_GROUP_BATTLE;
			break;
		case AchievementType::Equip:
		case AchievementType::ReceiveItem:
		case AchievementType::HaveItem:
		case AchievementType::CraftItem:
			m_Group = ACHIEVEMENT_GROUP_ITEMS;
			break;
		default:
			m_Group = ACHIEVEMENT_GROUP_GENERAL;
			break;
	}
}

bool CAchievementInfo::IsCompleted(int Criteria, const CAchievement* pAchievement) const
{
	// check if the criteria are met
	if(m_Criteria > 0 && m_Criteria != Criteria)
		return false;

	// logic for checking progress by achievement type
	switch(m_Type)
	{
		// achievements that require minimal progress (just having progress)
		case AchievementType::Equip:
		case AchievementType::UnlockWorld:
			return pAchievement->m_Progress > 0;

		// achievements that require a specific number of actions or conditions to be met
		case AchievementType::CraftItem:
		case AchievementType::DefeatMob:
		case AchievementType::ReceiveItem:
		case AchievementType::HaveItem:
		case AchievementType::DefeatPVE:
		case AchievementType::DefeatPVP:
		case AchievementType::Death:
		case AchievementType::TotalDamage:
		case AchievementType::Leveling:
			return pAchievement->m_Progress >= m_Required;

		// if the type is unknown, print an error and return false
		default:
			dbg_assert(false, "Unknown achievement type");
			return false;
	}
}

bool CAchievement::UpdateProgress(int Criteria, int Progress, int ProgressType)
{
	CPlayer* pPlayer = GetPlayer();
	if(m_Completed || !pPlayer)
		return false;

	// update the achievement progress
	switch(ProgressType)
	{
		case PROGRESS_ABSOLUTE:
		{
			m_Progress = Progress;
			break;
		}
		case PROGRESS_ACCUMULATE:
		{
			m_Progress += Progress;
			break;
		}
		default:
			return false;
	}
	m_Progress = clamp(m_Progress, 0, m_pInfo->GetRequired());

	// check if the achievement is completed
	if(m_pInfo->IsCompleted(Criteria, this))
	{
		GS()->CreateHammerHit(pPlayer->m_ViewPos);
		GS()->CreatePlayerSound(m_ClientID, SOUND_CTF_CAPTURE);
		GS()->Chat(m_ClientID, "'{}' has completed the achievement '{}'!", Server()->ClientName(m_ClientID), m_pInfo->GetName());

		m_Completed = true;
		RewardPlayer();
	}
	else
	{
		NotifyPlayerProgress();
	}

	// save
	pPlayer->Account()->UpdateAchievementProgress(m_pInfo->GetID(), m_Progress, m_Completed);
	return true;
}

void CAchievement::RewardPlayer() const
{
	CPlayer* pPlayer = GetPlayer();
	const auto& JsonData = m_pInfo->GetRewardData();

	// json reward
	if(!JsonData.empty())
	{
		if(const int Experience = JsonData.value("exp", 0); Experience > 0)
		{
			pPlayer->Account()->AddExperience(Experience);
			GS()->Chat(m_ClientID, "You received {} exp!", Experience);
		}

		for(const CItemsContainer Items = CItem::FromArrayJSON(JsonData, "items"); auto & Item : Items)
		{
			pPlayer->GetItem(Item)->Add(Item.GetValue());
		}
	}

	// achievement points
	if(const int AchievementPoints = m_pInfo->GetPoint(); AchievementPoints > 0)
	{
		auto* pPlayerItem = pPlayer->GetItem(itAchievementPoint);
		pPlayerItem->Add(AchievementPoints);
		GS()->Chat(m_ClientID, "You received {}({}) achievement points!", AchievementPoints, pPlayerItem->GetValue());
	}
}

void CAchievement::NotifyPlayerProgress()
{
	const int Percent = translate_to_percent(m_pInfo->GetRequired(), m_Progress);
	if(Percent > 80 && !m_NotifiedSoonComplete)
	{
		m_NotifiedSoonComplete = true;
		GS()->Chat(m_ClientID, "Achievement '{}' will be soon completed!", m_pInfo->GetName());
	}
}
