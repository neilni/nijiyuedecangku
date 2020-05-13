#pragma once

#define D2CLIENT_CMD TEXT("Global\\D2CLIENT_CMD_EVENT")
#define D2HACK_REPLY TEXT("Global\\D2HACK_REPLY_EVENT")
#define D2HACK_MSG TEXT("Global\\D2HACK_MSG_EVENT")
#define D2CLIENT_MSG_CONFIRM TEXT("Global\\D2CLIENT_MSG_CONFIRM_EVENT")
#define D2HACK_SHARED_BUF TEXT("Global\\D2HACK_SHARED_BUFName")
#define D2HACK_REPLY_BUFF_SIZE 256
#define D2HACK_SHARED_BUF_SIZE 1024

enum D2HackCommandID
{
	D2CMD_Login,
	D2CMD_CreateGame,
	D2CMD_RunAct1Boss,
	D2CMD_RunBot,
	D2CMD_PauseBot,
	D2CMD_ResumeBot,
	D2CMD_ReloadConfig,
	D2CMD_RevealMap,
	D2CMD_PermShow,
	D2CMD_WeatherOff,
};

enum D2HackReplyMsgType
{
	D2RPY_LOG,
	D2RPY_FOUND_ITEM,
	D2RPY_OBTAIN_ITEM,
	D2RPY_SUCC_CREATEGAME,
	D2RPY_ERROR_NEED_RESTART,
	D2RPY_PING,
};

struct D2HackReply
{
	struct ItemInfo
	{
		char Quality;
		char ItemName[128];
		char Msg[D2HACK_REPLY_BUFF_SIZE - 128 - 1];
	};
	D2HackReplyMsgType msgType;
	union
	{
		char Msg[D2HACK_REPLY_BUFF_SIZE];
		ItemInfo ItemMsg;
	};
	
};

struct D2HackCommandBuffer
{
	struct
	{
		WORD nCmdID;
		char sConfigFilePath[32];
	}ClientCommand;

	D2HackReply ReplyInfo;
};