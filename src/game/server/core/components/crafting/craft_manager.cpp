/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "craft_manager.h"

#include <game/server/gamecontext.h>

#include "../achievements/achievement_data.h"

void CCraftManager::OnPreInit()
{
	// load crafts
	ResultPtr pRes = Database->Execute<DB::SELECT>("*", "tw_crafts_list");
	while(pRes->next())
	{
		int ItemID = pRes->getInt("ItemID");
		int ItemValue = pRes->getInt("ItemValue");
		int Price = pRes->getInt("Price");
		int WorldID = pRes->getInt("WorldID");

		// initialize required ingredients
		CItemsContainer RequiredIngredients {};
		std::string JsonRequiredData = pRes->getString("RequiredItems").c_str();
		mystd::json::parse(JsonRequiredData, [&](nlohmann::json& pJson)
		{
			RequiredIngredients = CItem::FromArrayJSON(pJson, "items");
		});

		// initialize new craft element
		CraftIdentifier ID = pRes->getInt("ID");
		auto* pCraftItem = CCraftItem::CreateElement(ID);
		pCraftItem->Init(RequiredIngredients, CItem(ItemID, ItemValue), Price, WorldID);
	}

	// sort craft item's by function
	std::ranges::sort(CCraftItem::Data(), [](const CCraftItem* p1, const CCraftItem* p2)
	{
		return p1->GetItem()->Info()->GetType() > p2->GetItem()->Info()->GetType();
	});
}

void CCraftManager::OnCharacterTile(CCharacter* pChr)
{
	CPlayer* pPlayer = pChr->GetPlayer();

	HANDLE_TILE_VOTE_MENU(pPlayer, pChr, TILE_CRAFT_ZONE, MENU_CRAFTING_LIST, {}, {});
}

void CCraftManager::CraftItem(CPlayer* pPlayer, CCraftItem* pCraft) const
{
	if(!pPlayer || !pCraft)
		return;

	const int ClientID = pPlayer->GetCID();
	CPlayerItem* pPlayerCraftItem = pPlayer->GetItem(*pCraft->GetItem());

	// check if we have all the necessary items
	std::string missingItems;
	for(const auto& RequiredItem : pCraft->GetRequiredItems())
	{
		const int playerItemCount = pPlayer->GetItem(RequiredItem)->GetValue();
		const int requiredItemCount = RequiredItem.GetValue();

		if(playerItemCount < requiredItemCount)
		{
			int itemShortage = requiredItemCount - playerItemCount;
			missingItems += fmt_localize(ClientID, "{} x{} ", RequiredItem.Info()->GetName(), itemShortage);
		}
	}

	if(!missingItems.empty())
	{
		GS()->Chat(ClientID, "Item(s) left to gather: '{}'", missingItems.c_str());
		return;
	}

	// checking to see if there are enough funds for crafting
	const int craftPrice = pCraft->GetPrice(pPlayer);
	if(!pPlayer->Account()->SpendCurrency(craftPrice))
		return;

	// remove necessary items from the player's inventory
	for(const auto& RequiredItem : pCraft->GetRequiredItems())
	{
		pPlayer->GetItem(RequiredItem)->Remove(RequiredItem.GetValue());
	}

	// add the crafted item to the player's inventory
	const int craftedItemCount = pCraft->GetItem()->GetValue();
	pPlayerCraftItem->Add(craftedItemCount);

	// report a crafted item, either to everyone or only to a player, depending on its characteristics
	if(!pPlayerCraftItem->Info()->IsStackable())
	{
		GS()->Chat(-1, "'{}' crafted '[{} x{}]'.", Server()->ClientName(ClientID), pPlayerCraftItem->Info()->GetName(), craftedItemCount);
	}
	else
	{
		GS()->Chat(ClientID, "You crafted '[{} x{}]'.", pPlayerCraftItem->Info()->GetName(), craftedItemCount);
	}

	// update achievement and votes
	pPlayer->UpdateAchievement(AchievementType::CraftItem, pCraft->GetID(), craftedItemCount, PROGRESS_ACCUMULATE);
	pPlayer->m_VotesData.UpdateCurrentVotes();
}

bool CCraftManager::OnPlayerVoteCommand(CPlayer* pPlayer, const char* pCmd, const int Extra1, const int Extra2, int ReasonNumber, const char* pReason)
{
	// craft item function
	if(PPSTR(pCmd, "CRAFT") == 0)
	{
		CraftItem(pPlayer, GetCraftByID(Extra1));
		return true;
	}

	return false;
}

bool CCraftManager::OnSendMenuVotes(CPlayer* pPlayer, int Menulist)
{
	const int ClientID = pPlayer->GetCID();

	// craft list menu
	if(Menulist == MENU_CRAFTING_LIST)
	{
		// show information
		VoteWrapper VCraftInfo(ClientID, VWF_SEPARATE_OPEN | VWF_STYLE_STRICT_BOLD, "\u2692 Crafting Information");
		VCraftInfo.Add("If you will not have enough items for crafting");
		VCraftInfo.Add("You will write those and the amount that is still required");
		VCraftInfo.AddItemValue(itGold);

		// show craft tabs
		for(auto i = (int)ItemGroup::Quest; i < (int)ItemGroup::Potion; i++)
			ShowCraftList(pPlayer, (ItemGroup)i);
		return true;
	}

	// craft selected item
	if(Menulist == MENU_CRAFTING_SELECT)
	{
		pPlayer->m_VotesData.SetLastMenuID(MENU_CRAFTING_LIST);

		if(const auto CraftID = pPlayer->m_VotesData.GetExtraID())
		{
			ShowCraftItem(pPlayer, GetCraftByID(CraftID.value()));
			VoteWrapper::AddEmptyline(ClientID);
		}

		// add backpage
		VoteWrapper::AddBackpage(ClientID);
		return true;
	}

	return false;
}

void CCraftManager::ShowCraftItem(CPlayer* pPlayer, CCraftItem* pCraft) const
{
	const int ClientID = pPlayer->GetCID();
	if(!pCraft || pCraft->GetWorldID() != GS()->GetWorldID())
		return;

	// detail information
	VoteWrapper VCraftItem(ClientID, VWF_SEPARATE_OPEN | VWF_STYLE_STRICT_BOLD, "\u2692 Detail information");
	CItemDescription* pCraftItemInfo = pCraft->GetItem()->Info();
	VCraftItem.Add("Crafting: {} x{}", pCraftItemInfo->GetName(), pCraft->GetItem()->GetValue());
	VCraftItem.Add("{}", pCraftItemInfo->GetDescription());
	if(pCraftItemInfo->HasAttributes())
	{
		VCraftItem.Add(pCraftItemInfo->GetStringAttributesInfo(pPlayer, 0).c_str());
	}
	VoteWrapper::AddEmptyline(ClientID);

	// add craft reciepts
	VoteWrapper VCraftRequired(ClientID, VWF_SEPARATE_OPEN | VWF_STYLE_STRICT, "Required items", pCraftItemInfo->GetName());
	{
		VCraftRequired.BeginDepth();

		// required gold
		int CraftPrice = pCraft->GetPrice(pPlayer);
		if(CraftPrice > 0)
		{
			auto playerGold = pPlayer->Account()->GetTotalGold();
			bool hasEnoughGold = pPlayer->Account()->GetTotalGold() >= CraftPrice;
			VCraftRequired.MarkList().Add("{} Gold x{$} ({$})", hasEnoughGold ? "\u2714" : "\u2718", CraftPrice, playerGold);
		}

		// requied items
		for(const auto& pRequiredItem : pCraft->GetRequiredItems())
		{
			CPlayerItem* pPlayerItem = pPlayer->GetItem(pRequiredItem);
			bool hasEnoughItems = pPlayerItem->GetValue() >= pRequiredItem.GetValue();
			VCraftRequired.MarkList().Add("{} {} x{} ({})", hasEnoughItems ? "\u2714" : "\u2718",
				pRequiredItem.Info()->GetName(), pRequiredItem.GetValue(), pPlayerItem->GetValue());
		}
		VCraftRequired.EndDepth();
	}
	VoteWrapper::AddEmptyline(ClientID);

	// add craft button
	bool hasCraftItem = pPlayer->GetItem(pCraft->GetItem()->GetID())->HasItem();
	if(!pCraftItemInfo->IsStackable() && hasCraftItem)
	{
		VoteWrapper(ClientID).Add("- You already have the item", pCraft->GetPrice(pPlayer));
	}
	else
	{
		VoteWrapper(ClientID).AddOption("CRAFT", pCraft->GetID(), "\u2699 Craft ({} gold)", pCraft->GetPrice(pPlayer));
	}

}

void CCraftManager::ShowCraftList(CPlayer* pPlayer, ItemGroup Group) const
{
	const int ClientID = pPlayer->GetCID();

	// order only by type
	if(std::ranges::none_of(CCraftItem::Data(), [Group](const CCraftItem* p)
	{ return p->GetItem()->Info()->GetGroup() == Group; }))
	{
		return;
	}

	// add empty line
	VoteWrapper::AddEmptyline(ClientID);

	// craft tab list
	VoteWrapper VCraftList(ClientID, VWF_SEPARATE_OPEN, GetItemGroupName(Group));
	for(const auto& pCraft : CCraftItem::Data())
	{
		CItemDescription* pCraftItemInfo = pCraft->GetItem()->Info();

		// check by type and world
		if(pCraftItemInfo->GetGroup() != Group || pCraft->GetWorldID() != GS()->GetWorldID())
			continue;

		CraftIdentifier ID = pCraft->GetID();
		ItemIdentifier ItemID = pCraft->GetItem()->GetID();
		const int Price = pCraft->GetPrice(pPlayer);

		// set title name by enchant type (or stack item, or only once)
		if(!pCraftItemInfo->IsStackable())
		{
			VCraftList.AddMenu(MENU_CRAFTING_SELECT, ID, "{}{} - {} gold", 
				(pPlayer->GetItem(ItemID)->GetValue() ? "✔ " : "\0"), pCraftItemInfo->GetName(), Price);
		}
		else
		{
			VCraftList.AddMenu(MENU_CRAFTING_SELECT, ID, "[{}]{} x{} - {} gold",
				pPlayer->GetItem(ItemID)->GetValue(), pCraftItemInfo->GetName(), pCraft->GetItem()->GetValue(), Price);
		}
	}

	// add line
	VoteWrapper::AddLine(ClientID);
}

CCraftItem* CCraftManager::GetCraftByID(CraftIdentifier ID) const
{
	auto iter = std::ranges::find_if(CCraftItem::Data(), [ID](const CCraftItem* p){ return p->GetID() == ID; });
	return iter != CCraftItem::Data().end() ? *iter : nullptr;
}
