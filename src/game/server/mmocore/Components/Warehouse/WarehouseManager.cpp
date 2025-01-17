﻿/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "WarehouseManager.h"

#include <game/server/gamecontext.h>

#include <game/server/mmocore/Components/Inventory/InventoryManager.h>

constexpr auto TW_WAREHOUSE_TABLE = "tw_warehouses";
constexpr auto TW_WAREHOUSE_ITEMS_TABLE = "tw_warehouse_items";

// Optimized
void CWarehouseManager::OnInit()
{
	// init warehouses
	ResultPtr pRes = Database->Execute<DB::SELECT>("*", TW_WAREHOUSE_TABLE);
	std::unordered_map< int, CWarehouse::ContainerTradingSlots > TradesSlots;
	while(pRes->next())
	{
		WarehouseIdentifier ID = pRes->getInt("ID");
		std::string Name = pRes->getString("Name").c_str();
		vec2 Pos = vec2((float)pRes->getInt("PosX"), (float)pRes->getInt("PosY"));
		int Currency = pRes->getInt("Currency");
		int WorldID = pRes->getInt("WorldID");

		// init by server
		CWarehouse(ID).Init(Name, Pos, Currency, WorldID);
	}

	// init trades slots
	ResultPtr pResStore = Database->Execute<DB::SELECT>("*", TW_WAREHOUSE_ITEMS_TABLE);
	while(pResStore->next())
	{
		TradeIdentifier ID = pResStore->getInt("ID");
		ItemIdentifier ItemID = pResStore->getInt("ItemID");
		int ItemValue = pResStore->getInt("ItemValue");
		ItemIdentifier RequiredItemID = pResStore->getInt("RequiredItemID");
		int Price = pResStore->getInt("Price");
		int Enchant = pResStore->getInt("Enchant");
		int WarehouseID = pResStore->getInt("WarehouseID");

		// init by server
		CTradingSlot TradeSlot(ID);
		std::shared_ptr<CItem> pItem = std::make_shared<CItem>(CItem(ItemID, ItemValue, Enchant));
		TradeSlot.Init(pItem, &CItemDescription::Data()[RequiredItemID], Price);
		TradesSlots[WarehouseID].push_back(TradeSlot);
	}

	// init trades slots for warehouses
	for(auto& [WarehouseID, DataContainer] : TradesSlots)
		CWarehouse::Data()[WarehouseID].m_aTradingSlots = DataContainer;
}

bool CWarehouseManager::OnHandleTile(CCharacter* pChr, int IndexCollision)
{
	CPlayer* pPlayer = pChr->GetPlayer();
	const int ClientID = pPlayer->GetCID();

	// shop zone
	if(pChr->GetHelper()->TileEnter(IndexCollision, TILE_SHOP_ZONE))
	{
		_DEF_TILE_ENTER_ZONE_SEND_MSG_INFO(pPlayer);
		GS()->UpdateVotes(ClientID, pPlayer->m_CurrentVoteMenu);
		return true;
	}
	else if(pChr->GetHelper()->TileExit(IndexCollision, TILE_SHOP_ZONE))
	{
		_DEF_TILE_EXIT_ZONE_SEND_MSG_INFO(pPlayer);
		GS()->UpdateVotes(ClientID, pPlayer->m_CurrentVoteMenu);
		return true;
	}

	return false;
}

bool CWarehouseManager::OnHandleMenulist(CPlayer* pPlayer, int Menulist, bool ReplaceMenu)
{
	const int ClientID = pPlayer->GetCID();
	CCharacter* pChr = pPlayer->GetCharacter();
	if(ReplaceMenu && pChr && pChr->IsAlive())
	{
		if(pChr->GetHelper()->BoolIndex(TILE_SHOP_ZONE))
		{
			if(CWarehouse* pWarehouse = GetWarehouse(pChr->m_Core.m_Pos))
			{
				ShowWarehouseMenu(pChr->GetPlayer(), pWarehouse);
			}
			else
			{
				GS()->AV(ClientID, "null", "Warehouse don't work");
			}

			return true;
		}
	}

	return false;
}

bool CWarehouseManager::OnHandleVoteCommands(CPlayer* pPlayer, const char* CMD, const int VoteID, const int VoteID2, int Get, const char* GetText)
{
	const int ClientID = pPlayer->GetCID();
	if(PPSTR(CMD, "REPAIR_ITEMS") == 0)
	{
		Job()->Item()->RepairDurabilityItems(pPlayer);
		GS()->Chat(ClientID, "You repaired all items.");
		return true;
	}

	if(PPSTR(CMD, "SHOP_BUY") == 0)
	{
		if(BuyItem(pPlayer, VoteID, VoteID2))
			GS()->UpdateVotes(ClientID, MenuList::MENU_MAIN);
		return true;
	}

	return false;
}

CWarehouse* CWarehouseManager::GetWarehouse(vec2 Pos) const
{
	for(auto& pItem : CWarehouse::Data())
	{
		if(distance(pItem.second.GetPos(), Pos) < 200)
			return &pItem.second;
	}
	return nullptr;
}

void CWarehouseManager::ShowWarehouseMenu(CPlayer* pPlayer, const CWarehouse* pWarehouse) const
{
	const int ClientID = pPlayer->GetCID();

	// show base shop functions
	GS()->AVH(ClientID, TAB_STORAGE, "Shop :: {STR}", pWarehouse->GetName());
	GS()->AVM(ClientID, "REPAIR_ITEMS", NOPE, TAB_STORAGE, "Repair all items - FREE");

	// show currency
	GS()->AV(ClientID, "null");
	GS()->AddVoteItemValue(ClientID, pWarehouse->GetCurrency()->GetID());
	GS()->AV(ClientID, "null");

	// show trade list
	int HideID = NUM_TAB_MENU + CItemDescription::Data().size() + 300;
	for(auto& Trade : pWarehouse->m_aTradingSlots)
	{
		int Price = Trade.GetPrice();
		const CItemDescription* pCurrencyItem = Trade.GetCurrency();
		const CItem* pItem = Trade.GetItem();

		// show trade slot actions
		if(pItem->Info()->IsEnchantable())
		{
			const bool PlayerHasItem = pPlayer->GetItem(*pItem)->HasItem();

			GS()->AVH(ClientID, HideID, "({STR}){STR} {STR} - {VAL} {STR}",
				(PlayerHasItem ? "✔" : "×"), pItem->Info()->GetName(), pItem->StringEnchantLevel().c_str(), Price, pCurrencyItem->GetName());

			char aAttributes[128];
			pItem->Info()->StrFormatAttributes(pPlayer, aAttributes, sizeof(aAttributes), pItem->GetEnchant());
			GS()->AVM(ClientID, "null", NOPE, HideID, "* {STR}", aAttributes);
		}
		else
		{
			GS()->AVH(ClientID, HideID, "({VAL}){STR}x{VAL} - {VAL} {STR}",
				pPlayer->GetItem(*pItem)->GetValue(), pItem->Info()->GetName(), pItem->GetValue(), Price, pCurrencyItem->GetName());
		}

		GS()->AVM(ClientID, "null", NOPE, HideID, "{STR}", pItem->Info()->GetDescription());
		GS()->AVD(ClientID, "SHOP_BUY", pWarehouse->GetID(), Trade.GetID(), HideID, "Buy {STR}x{VAL}", pItem->Info()->GetName(), pItem->GetValue());
		GS()->AVM(ClientID, "null", NOPE, HideID, "\0");
		HideID++;
	}

	GS()->AV(ClientID, "null");
}

bool CWarehouseManager::BuyItem(CPlayer* pPlayer, int WarehouseID, TradeIdentifier ID) const
{
	// finding a trade slot
	CWarehouse::ContainerTradingSlots& pContainer = GS()->GetWarehouse(WarehouseID)->m_aTradingSlots;
	auto Iter = std::find_if(pContainer.begin(), pContainer.end(), [ID](const CTradingSlot& p){ return p.GetID() == ID; });
	CTradingSlot* pTradeSlot = Iter != pContainer.end() ? &(*Iter) : nullptr;
	if(!pTradeSlot || !pTradeSlot->GetItem()->IsValid())
		return false;

	// check for enchantment
	const int ClientID = pPlayer->GetCID();
	CPlayerItem* pPlayerItem = pPlayer->GetItem(pTradeSlot->GetItem()->GetID());
	if(pPlayerItem->HasItem() && pPlayerItem->Info()->IsEnchantable())
	{
		GS()->Chat(ClientID, "Enchant item maximal count x1 in a backpack!");
		return false;
	}

	// purchase
	if(!pPlayer->SpendCurrency(pTradeSlot->GetPrice(), pTradeSlot->GetCurrency()->GetID()))
		return false;

	// give trade slot for player
	CItem* pTradeItem = pTradeSlot->GetItem();
	pPlayerItem->Add(pTradeItem->GetValue(), 0, pTradeItem->GetEnchant());
	GS()->Chat(ClientID, "You exchanged {STR}x{VAL} for {STR}x{VAL}.", pTradeSlot->GetCurrency()->GetName(), pTradeSlot->GetPrice(), pTradeItem->Info()->GetName(), pTradeItem->GetValue());
	return true;
}