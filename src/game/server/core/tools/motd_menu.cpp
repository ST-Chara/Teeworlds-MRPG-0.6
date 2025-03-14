﻿#include "motd_menu.h"

#include <game/server/gamecontext.h>

CGS* MotdMenu::GS() const
{
	return (CGS*)Instance::GameServerPlayer(m_ClientID);
}

CPlayer* MotdMenu::GetPlayer() const
{
	return GS()->GetPlayer(m_ClientID);
}

void MotdMenu::AddImpl(int extra, int extra2, std::string_view command, const std::string& description)
{
	auto* pPlayer = GetPlayer();
	if(!pPlayer)
		return;

	Point p;
	str_copy(p.m_aDesc, description.c_str(), sizeof(p.m_aDesc));
	str_utf8_fix_truncation(p.m_aDesc);
	p.m_Command = command;
	p.m_Extra = extra;
	p.m_Extra2 = extra2;
	m_Points.push_back(p);
	pPlayer->m_MotdData.m_ScrollManager.SetMaxScrollPos(static_cast<int>(m_Points.size()));
}

void MotdMenu::AddEditField(int TextID, int64_t Flags)
{
	auto* pPlayer = GetPlayer();
	if(!pPlayer)
		return;

	// initialize variables
	auto& playerMotdData = pPlayer->m_MotdData;
	auto& currentInputField = playerMotdData.m_CurrentInputField;
	auto& vFields = playerMotdData.m_vFields;
	const char* pMarkActive = (currentInputField.Active && TextID == currentInputField.TextID) ? "✎ " : "";
	size_t lengthSide = vFields[TextID].Message.empty() ? 18 : 0;
	std::string spaces = lengthSide > 0 ? std::string(lengthSide, '-') : "";

	// flags
	std::string endText;
	vFields[TextID].Flags = Flags;
	if(vFields[TextID].Flags & MTTEXTINPUTFLAG_PASSWORD)
		endText = std::string(vFields[TextID].Message.length(), '*');
	else
		endText = vFields[TextID].Message;

	// result
	std::string result = fmt_localize(m_ClientID, "{}[-{}{}-]", pMarkActive, endText, spaces);
	AddImpl(TextID, NOPE, "TEXT_FIELD", result);
}

void MotdMenu::Tick()
{
	// check is has points
	if(m_Points.empty())
		return;

	// check valid character
	auto* pPlayer = GetPlayer();
	if(!pPlayer || !pPlayer->GetCharacter())
		return;

	// initialize variables
	constexpr int lineSizeY = 20;
	constexpr int startLineY = -284;
	auto* pServer = Instance::Server();
	const auto* pChar = pPlayer->GetCharacter();
	const int targetX = pChar->m_Core.m_Input.m_TargetX;
	const int targetY = pChar->m_Core.m_Input.m_TargetY;

	// prepare the buffer for the MOTD
	int linePos = 2;
	std::string buffer = Instance::Localize(m_ClientID, "* Use 'Self kill' to close the motd!\n\n");
	auto addScrollBar = [&](int index)
	{
		// initialize variables
		const auto totalItems = m_Points.size();
		const auto currentScrollPos = pPlayer->m_MotdData.m_ScrollManager.GetScrollPos();
		const auto visibleItems = pPlayer->m_MotdData.m_ScrollManager.GetMaxVisibleItems();

		const auto visibleProportion = static_cast<float>(visibleItems) / static_cast<float>(totalItems);
		auto scrollBarHeight = round_to_int(visibleProportion * visibleItems);
		if(scrollBarHeight < 1)
			scrollBarHeight = 1;

		// calculate position
		const auto progress = static_cast<float>(currentScrollPos) / static_cast<float>(totalItems - visibleItems);
		const auto scrollBarPosition = static_cast<int>(progress * (visibleItems - scrollBarHeight));

		// get current index
		const auto currentBar = index - currentScrollPos;
		const char* pSymbol = currentBar >= scrollBarPosition && currentBar < scrollBarPosition + scrollBarHeight ? "\u258D" : "\u258F";
		buffer.append(pSymbol);
	};

	auto addLineToBuffer = [&](const std::string& line, bool isSelected)
	{
		buffer.append(isSelected ? "\u279C " : "\u257E ");
		buffer.append(line);
		buffer.append("\n");
	};

	// key events and freeze input is hovered worked area
	const int startWorkedAreaY = startLineY;
	const int endWorkedAreaY = startLineY + 24 * lineSizeY;
	if((targetX > -196 && targetX < 196 && targetY >= startWorkedAreaY && targetY < endWorkedAreaY))
	{
		// handle scrolling with key events
		if(pServer->Input()->IsKeyClicked(m_ClientID, KEY_EVENT_NEXT_WEAPON))
		{
			pPlayer->m_MotdData.m_ScrollManager.ScrollUp();
			m_ResendMotdTick = pServer->Tick() + 5;
		}
		else if(pServer->Input()->IsKeyClicked(m_ClientID, KEY_EVENT_PREV_WEAPON))
		{
			pPlayer->m_MotdData.m_ScrollManager.ScrollDown();
			m_ResendMotdTick = pServer->Tick() + 5;
		}

		pServer->Input()->BlockInputGroup(m_ClientID, BLOCK_INPUT_FIRE | BLOCK_INPUT_FREEZE_HAMMER);
	}

	// add menu items to buffer
	int i = pPlayer->m_MotdData.m_ScrollManager.GetScrollPos();
	for(; i < pPlayer->m_MotdData.m_ScrollManager.GetEndScrollPos() && i < static_cast<int>(m_Points.size()); ++i, ++linePos)
	{
		// add scrollbar symbol
		addScrollBar(i);

		// empty command only text
		const auto& command = m_Points[i].m_Command;
		if(command == "NULL")
		{
			buffer.append(m_Points[i].m_aDesc).append("\n");
			continue;
		}

		// initialize variables
		bool updatedMotd = false;
		const int checkYStart = startLineY + linePos * lineSizeY;
		const int checkYEnd = startLineY + (linePos + 1) * lineSizeY;
		const bool isSelected = (targetX > -196 && targetX < 196 && targetY >= checkYStart && targetY < checkYEnd);
		const bool isClicked = pServer->Input()->IsKeyClicked(m_ClientID, KEY_EVENT_FIRE);

		// is clicked with active text field editor
		if(isClicked && pPlayer->m_MotdData.m_CurrentInputField.Active)
		{
			GS()->Chat(m_ClientID, "[⇄] Editing a field is canceled!");
			pPlayer->m_MotdData.m_CurrentInputField.Active = false;
			updatedMotd = true;
		}
		// is hovered and clicked
		else if(isSelected && pServer->Input()->IsKeyClicked(m_ClientID, KEY_EVENT_FIRE))
		{
			const auto& extra = m_Points[i].m_Extra;

			if(command == "TEXT_FIELD")
			{
				pPlayer->m_MotdData.m_CurrentInputField.Active = true;
				pPlayer->m_MotdData.m_CurrentInputField.TextID = extra;
				GS()->Chat(m_ClientID, "[⇄] Editing a field (use chat)!");
				updatedMotd = true;
			}

			if(command == "CLOSE")
			{
				ClearMotd();
				return;
			}

			if(command == "MENU")
			{
				updatedMotd = true;
				m_Menulist = extra;
				m_MenuExtra = m_Points[i].m_Extra2 <= NOPE ? std::nullopt : std::make_optional<int>(m_Points[i].m_Extra2);
				pPlayer->m_MotdData.m_vFields.clear();
				pPlayer->m_MotdData.m_ScrollManager.Reset();

			}

			if(command == "BACKPAGE")
			{
				updatedMotd = true;
				m_Menulist = m_LastMenulist;
				pPlayer->m_MotdData.m_vFields.clear();
				pPlayer->m_MotdData.m_ScrollManager.Reset();
			}

			if(GS()->OnClientMotdCommand(m_ClientID, command.c_str(), extra))
			{
				if(m_Flags & MTFLAG_CLOSE_ON_SELECT)
				{
					ClearMotd();
					return;
				}

				updatedMotd = true;
			}
		}

		if(updatedMotd)
		{
			UpdateMotd();
			return;
		}

		addLineToBuffer(m_Points[i].m_aDesc, isSelected);
	}
	buffer += "\n" + m_Description;

	// update buffer if it has changed
	if(m_LastBuffer != buffer)
	{
		m_LastBuffer = buffer;
		m_ResendMotdTick = pServer->Tick() + pServer->TickSpeed();
		pServer->SendMotd(m_ClientID, buffer.c_str());
	}
	else if(pServer->Tick() >= m_ResendMotdTick)
	{
		m_ResendMotdTick = pServer->Tick() + pServer->TickSpeed();
		UpdateMotd();
	}
}

void MotdMenu::Send(int Menulist)
{
	// add close button if necessary
	if(m_Flags & MTFLAG_CLOSE_BUTTON)
	{
		AddImpl(NOPE, NOPE, "CLOSE", "Close");
	}

	// reinitialize player motd menu
	const auto* pGS = (CGS*)Instance::GameServerPlayer(m_ClientID);
	if(auto* pPlayer = pGS->GetPlayer(m_ClientID))
	{
		m_Menulist = Menulist;
		if(pPlayer->m_pMotdMenu)
		{
			m_LastMenulist = (pPlayer->m_pMotdMenu->m_Menulist != Menulist) ? pPlayer->m_pMotdMenu->m_Menulist : pPlayer->m_pMotdMenu->m_LastMenulist;
		}
		pPlayer->m_pMotdMenu = std::make_unique<MotdMenu>(*this);
	}
}

bool MotdMenu::ApplyFieldEdit(const std::string& Message)
{
	auto* pPlayer = GetPlayer();
	if(!pPlayer)
		return false;

	auto& textFieldEdit = pPlayer->m_MotdData.m_CurrentInputField;
	auto& vFields = pPlayer->m_MotdData.m_vFields;

	if(!textFieldEdit.Active)
		return false;

	// check flag ony numeric values
	auto& filedData = vFields[textFieldEdit.TextID];
	if(filedData.Flags & MTTEXTINPUTFLAG_ONLY_NUMERIC && !std::all_of(filedData.Message.begin(), filedData.Message.end(), isdigit))
	{
		GS()->Chat(m_ClientID, "[⇄] Only numeric values will be allowed to be entered.");
		return true;
	}

	textFieldEdit.Active = false;
	filedData.Message = Message;
	m_ResendMotdTick = 0;
	GS()->Chat(m_ClientID, "[⇄] Field is been updated!");
	return true;
}

void MotdMenu::ClearMotd()
{
	auto* pServer = Instance::Server();
	auto* pPlayer = GetPlayer();

	m_Points.clear();
	pServer->SendMotd(m_ClientID, "");
	if(pPlayer && pPlayer->m_pMotdMenu)
	{
		pPlayer->m_MotdData.m_ScrollManager.Reset();
		pPlayer->m_MotdData.m_vFields.clear();
		pPlayer->m_pMotdMenu.reset();
	}
}

void MotdMenu::UpdateMotd()
{
	auto* pServer = Instance::Server();
	auto* pPlayer = GetPlayer();

	if(pPlayer)
	{
		const auto oldMenuExtra = m_MenuExtra;
		m_ResendMotdTick = pServer->Tick() + pServer->TickSpeed();
		GS()->SendMenuMotd(pPlayer, m_Menulist);
		pPlayer->m_pMotdMenu->m_MenuExtra = oldMenuExtra;
	}
}
