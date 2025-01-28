#ifndef GAME_SERVER_CORE_COMMAND_PROCESSOR_H
#define GAME_SERVER_CORE_COMMAND_PROCESSOR_H

#include <game/commands.h>

class CCommandProcessor
{
	class CGS* m_pGS;
	class CGS* GS() const { return m_pGS; }
	CCommandManager m_CommandManager;

	static void ConChatLogin(IConsole::IResult* pResult, void* pUserData);
	static void ConChatRegister(IConsole::IResult* pResult, void* pUserData);
	static void ConChatGuild(IConsole::IResult* pResult, void* pUserData);
	static void ConChatHouse(IConsole::IResult* pResult, void* pUserData);
	static void ConChatSound(IConsole::IResult* pResult, void* pUserData);
	static void ConChatGiveEffect(IConsole::IResult* pResult, void* pUserData);
	static void ConGroup(IConsole::IResult* pResult, void* pUser);
	static void ConChatUseItem(IConsole::IResult* pResult, void* pUserData);
	static void ConChatUseSkill(IConsole::IResult* pResult, void* pUserData);
	static void ConChatCmdList(IConsole::IResult* pResult, void* pUserData);
	static void ConChatRules(IConsole::IResult* pResult, void* pUserData);
	static void ConChatVoucher(IConsole::IResult* pResult, void* pUserData);
	static void ConChatWiki(IConsole::IResult* pResult, void* pUserData);

public:
	CCommandProcessor(CGS* pGS);
	~CCommandProcessor();

	void ChatCmd(const char* pMessage, class CPlayer *pPlayer);

private:
	void AddCommand(const char* pName, const char* pParams, IConsole::FCommandCallback pfnFunc, void* pUser, const char* pHelp);
};

#endif
