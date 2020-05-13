#include "stdafx.h"
#include "ShareData.h"
#include "misc.h"
#include "D2Hack1.11b.h"

void DbgPrint(const char *format, ...)
{
#ifdef _DEBUG
	char str[128] = "D2HACK_TOOL:";
	va_list ap;
	va_start(ap, format);
	vsprintf_s(str + strlen(str), 128 - strlen(str), format, ap);
	va_end(ap);
	OutputDebugString(str);
#endif
}

void DbgPrint(const WCHAR *format, ...)
{
#ifdef _DEBUG
	WCHAR str[128] = L"D2HACK_TOOL:";
	va_list ap;
	va_start(ap, format);
	vswprintf_s(str + wcslen(str), 128 - wcslen(str), format, ap);
	va_end(ap);
	OutputDebugStringW(str);
#endif
}

void ReadLocalBYTES(LPVOID pAddress, LPVOID buf, int len)
{
	ReadProcessMemory(GetCurrentProcess(), pAddress, buf, len, 0);
}

void WriteLocalBYTES(LPVOID pAddress, void *buf, int len)
{
	DWORD oldprot;
	if (VirtualProtect(pAddress, len, PAGE_EXECUTE_READWRITE, &oldprot))
	{
		DWORD dummy;
		WriteProcessMemory(GetCurrentProcess(), pAddress, buf, len, 0);
		VirtualProtect(pAddress, len, oldprot, &dummy);
	}
}

DWORD VirtualProtect(DWORD pAddress, DWORD len, DWORD prot)
{
	DWORD oldprot = 0;
	VirtualProtect((void *)pAddress, len, prot, &oldprot);
	return oldprot;
}
void WriteLocalBYTES(DWORD pAddress, void *buf, int len)
{
	DWORD oldprot = VirtualProtect(pAddress, len, PAGE_EXECUTE_READWRITE);
	WriteProcessMemory(GetCurrentProcess(), (void *)pAddress, buf, len, 0);
	VirtualProtect(pAddress, len, oldprot);
}

void FillLocalBYTES(DWORD pAddress, BYTE ch, int len)
{
	BYTE *buf1 = new BYTE[len];
	memset(buf1, ch, len);
	WriteLocalBYTES(pAddress, buf1, len);
	delete buf1;
}

void FillLocalBYTES(LPVOID pAddress, BYTE ch, int len)
{
	BYTE *buf1 = new BYTE[len];
	memset(buf1, ch, len);
	WriteLocalBYTES(pAddress, buf1, len);
	delete buf1;
}

DWORD ReadLocalDWORD(LPVOID pAddress)
{
	DWORD val = 0;
	ReadLocalBYTES(pAddress, &val, 4);
	return val;
}

void InterceptLocalCode2(BYTE inst, LPVOID pOldCode, DWORD offset, int len)
{
	BYTE *buf1 = new BYTE[len];
	buf1[0] = inst;
	*(DWORD *)(buf1 + 1) = offset;
	memset(buf1 + 5, INST_NOP, len - 5);
	WriteLocalBYTES(pOldCode, buf1, len);
	delete buf1;
}

void InterceptLocalCode(BYTE inst, LPVOID pOldCode, LPVOID pNewCode, int len)
{
	DWORD offset = ((char*)pNewCode) - ((char*)pOldCode) -  5;
	InterceptLocalCode2(inst, pOldCode, offset, len);
}

void PatchCALL(LPVOID addr, LPVOID param, DWORD len)
{
	InterceptLocalCode(INST_CALL, addr, param, len);
}

void PatchFILL(LPVOID addr, LPVOID param, DWORD len)
{
	FillLocalBYTES(addr, (BYTE)(DWORD)param, len);
}

void PatchJMP(LPVOID addr, LPVOID param, DWORD len)
{
	InterceptLocalCode(INST_JMP, addr, param, len);
}

void PatchVALUE(LPVOID addr, LPVOID param, DWORD len)
{
	WriteLocalBYTES(addr, &param, len);
}

LPVOID GetDllOffset(char *dll, int offset)
{
	HMODULE hmod = GetModuleHandle(dll);
	if (!hmod)
		hmod = LoadLibrary(dll);
	if (!hmod)
		return 0;
	if (offset < 0) {
		return GetProcAddress(hmod, (LPCSTR)-offset);
	}
	return (LPVOID)(((DWORD)hmod) + offset);
}

LPVOID GetDllOffset(int num)
{
	static char *dlls[] = { "D2CLIENT.DLL", "D2COMMON.DLL", "D2GFX.DLL", "D2WIN.DLL", "D2LANG.DLL", "D2CMP.DLL", "D2MULTI.DLL", "BNCLIENT.DLL", "D2NET.DLL", "STORM.DLL", "FOG.DLL" };
	return GetDllOffset(dlls[num & 0xff], num >> 8);
}

LPVOID g_initD2Ptr(int offset)
{
	return GetDllOffset(offset);
}

void LButtonClick(HWND hwnd, WORD x, WORD y)
{
	LPARAM lparm = MAKELPARAM(x, y);
	SendMessage(hwnd, WM_LBUTTONDOWN, 0, lparm);
	SendMessage(hwnd, WM_LBUTTONUP, 0, lparm);
}

void RButtonClick(HWND hwnd, WORD x, WORD y)
{
	LPARAM lparm = MAKELPARAM(x, y);
	SendMessage(hwnd, WM_RBUTTONDOWN, 0, lparm);
	SendMessage(hwnd, WM_RBUTTONUP, 0, lparm);
}

void SendVKey(HWND hwnd, char c, short repeat)
{
	while (--repeat >= 0)
		SendMessage(hwnd, WM_KEYDOWN, c, 0);

}
void SendString(HWND hwnd, const char* str)
{
	for (DWORD i = 0; i < strlen(str); i++)
		SendMessage(hwnd, WM_CHAR, str[i], 0);
}

//#define _D2HACK_PathTreeMemLeak_Deteck
#ifdef _D2HACK_PathTreeMemLeak_Deteck
int _D2Hack_PathTree_MemAllocatedSize = 0;
int _D2Hack_PathTree_MemDeleteSize = 0;
#endif

void NewNodeAndAttachtoPathTree(MonsterPathTree *to, const MonsterPathTree& from)
{
	to->NextNum++;
	if (!to->pNextNodeList)
	{
		to->pNextNodeList = new MonsterPathTree[8];
		memset(to->pNextNodeList, 0, 8 * sizeof(MonsterPathTree));
#ifdef _D2HACK_PathTreeMemLeak_Deteck
		_D2Hack_PathTree_MemAllocatedSize += (8 * sizeof(MonsterPathTree));
#endif
	}
	to->pNextNodeList[to->NextNum - 1] = from;
	to->pNextNodeList[to->NextNum - 1].pPreNode = to;
}

//从生成树的根节点开始释放生成树的内存
void DeleteMonsterPathTree(MonsterPathTree* head)
{
	for (char i = 0; i < head->NextNum; ++i)
	{
		DeleteMonsterPathTree(&head->pNextNodeList[i]);
	}
	if (head->pNextNodeList)
	{
#ifdef _D2HACK_PathTreeMemLeak_Deteck
		_D2Hack_PathTree_MemDeleteSize += (8 * sizeof(MonsterPathTree));
#endif
		delete[]head->pNextNodeList;
		head->pNextNodeList = NULL;
		head->NextNum = 0;
	}
}

MonsterPathTree* PathTreeNodeAlreadyExist(MonsterPathTree* head, LPVOID target)
{
	MonsterPathTree *p = head, *ret;
	if (p && p->pRoom == target)
		return head;

	for (int i = 0; i < p->NextNum; ++i)
	{
		if (ret = PathTreeNodeAlreadyExist(&p->pNextNodeList[i], target))
			return ret;
	}
	return NULL;
}

extern LPVOID g_D2HackSharedBuf;
extern HANDLE g_hD2ClientMsgConfirmEvt;
extern HANDLE g_hD2HackMsgEvt;

void SendHackMessage(D2HackReplyMsgType type, const D2HackReply::ItemInfo& r)
{
	char str[128];
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf_s(str, sizeof(str), "%4d-%02d-%02d %02d:%02d:%02d ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	strcpy_s(str + strlen(str), sizeof(str) - strlen(str), r.Msg);

	D2HackCommandBuffer *cmd = (D2HackCommandBuffer*)g_D2HackSharedBuf;
	cmd->ReplyInfo.ItemMsg = r;
	memcpy(cmd->ReplyInfo.ItemMsg.Msg, str, sizeof(str));	
	cmd->ReplyInfo.msgType = type;

	SetEvent(g_hD2HackMsgEvt);
	WaitForSingleObject(g_hD2ClientMsgConfirmEvt, 1000);
}

void SendHackMessage(D2HackReplyMsgType type, const char *format, ...)
{
	char str[128];
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf_s(str, sizeof(str), "%4d-%02d-%02d %02d:%02d:%02d ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	va_list ap;
	va_start(ap, format);
	vsprintf_s(str + strlen(str), 128 - strlen(str), format, ap);
	va_end(ap);

	D2HackCommandBuffer *cmd = (D2HackCommandBuffer*)g_D2HackSharedBuf;
	cmd->ReplyInfo.msgType = type;
	CopyMemory(cmd->ReplyInfo.Msg, str, sizeof(str));
	
	SetEvent(g_hD2HackMsgEvt);
	WaitForSingleObject(g_hD2ClientMsgConfirmEvt, 1000);
}

void SendHackMessage(D2HackReplyMsgType type, const wchar_t *format, ...)
{
	wchar_t str[64];
	SYSTEMTIME st;
	GetLocalTime(&st);
	swprintf_s(str, SIZEOFARRAY(str), L"%4d-%02d-%02d %02d:%02d:%02d ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	va_list ap;
	va_start(ap, format);
	vswprintf_s(str + wcslen(str), SIZEOFARRAY(str) - wcslen(str), format, ap);
	va_end(ap);

	int w_nlen = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, false);
	char *str2 = new char[w_nlen];
	memset(str2, 0, w_nlen);
	WideCharToMultiByte(CP_ACP, 0, str, -1, str2, w_nlen, NULL, false);

	D2HackCommandBuffer *cmd = (D2HackCommandBuffer*)g_D2HackSharedBuf;
	cmd->ReplyInfo.msgType = type;
	memcpy(cmd->ReplyInfo.Msg, str2, sizeof(cmd->ReplyInfo.Msg));
	delete[]str2;

	SetEvent(g_hD2HackMsgEvt);
	WaitForSingleObject(g_hD2ClientMsgConfirmEvt, 1000);
}

void ltrim(char *s)
{
	char *p;
	p = s;
	while (*p == ' ' || *p == '\t') { p++; }
	strcpy_s(s, strlen(s) + 1, p);
}

void rtrim(char *s)
{
	int i;

	i = strlen(s) - 1;
	while ((s[i] == ' ' || s[i] == '\t') && i >= 0) { i--; };
	s[i + 1] = '\0';
}

void trim(char *s)
{
	ltrim(s);
	rtrim(s);
}

static void SortItemConfig()
{
	int i, j;
	for (i = 0; i < g_Game_Config.ItemsConfigNum; ++i)
		for (j = g_Game_Config.ItemsConfigNum - 1; j >= i + 1; --j)
		{
			if (g_Game_Config.ItemsConfigForBot[j].Code < g_Game_Config.ItemsConfigForBot[j - 1].Code)
				PickupItemsConfig::swap(g_Game_Config.ItemsConfigForBot[j], g_Game_Config.ItemsConfigForBot[j - 1]);
		}
}

const PickupItemsConfig::ItemConfigDiary PickupItemsConfig::ItemConfigTable[] = {
	{ "addallskills", STAT_ALL_SKILLS, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "fcr", STAT_FCR, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "fbr", STAT_FBR, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "fhr", STAT_FHR, PickupItemsConfig::_pFuncIsUnitStatMatch },

	{ "ias", STAT_IAS, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "fr", STAT_FIRE_RESIST, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "lr", STAT_LIGHTING_RESIST, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "cr", STAT_COLD_RESIST, PickupItemsConfig::_pFuncIsUnitStatMatch },

	{ "pr", STAT_POSION_RESIST, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "str", STAT_STRENGTH, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "dex", STAT_DEX, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "life", STAT_MAXLIFE, PickupItemsConfig::_pFuncIsUnitLifeManaMatch },
	{ "mana", STAT_MAXMANA, PickupItemsConfig::_pFuncIsUnitLifeManaMatch },
	{ "ll", STAT_LIFE_LEECH, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "lm", STAT_MANA_LEECH, PickupItemsConfig::_pFuncIsUnitStatMatch },

	{ "mf", STAT_MAGIC_FIND, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "eth", ITEMFLAG_ETHEREAL, PickupItemsConfig::_pFuncIsUnitFlagMatch },
	{ "toblock", STAT_TOBLOCK, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "numsockets", STAT_NUMSOCKETS, PickupItemsConfig::_pFuncIsUnitStatMatch },

	{ "level", 0, PickupItemsConfig::_pFuncIsItemLevelMatch },
	{ "defence1", STAT_DEFENCE, PickupItemsConfig::_pFuncIsUnitStatMatch2 },
	{ "defence2", STAT_DEFENCE, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "enhanceddamage", STAT_ENHANCED_DAMAGE, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "goodgold", STAT_GOLD, PickupItemsConfig::_pFuncIsUnitStatMatch },

	{ "coldmaster", STAT_COLD_MASTER, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "coldpierce", STAT_COLD_PIERCE, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "firemaster", STAT_FIRE_MASTER, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "firepierce", STAT_FIRE_PIERCE, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "poisonmaster", STAT_POISON_MASTER, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "poisonpierce", STAT_POISON_PIERCE, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "lightmaster", STAT_LIGHT_MASTER, PickupItemsConfig::_pFuncIsUnitStatMatch },
	{ "lightpierce", STAT_LIGHT_PIERCE, PickupItemsConfig::_pFuncIsUnitStatMatch },

	{ "addsingleskill", STAT_ADD_SINGLE_SKILL, PickupItemsConfig::_pFuncIsUnitSingleSkillMatch },

	{ "sorcoldskills", 0x0a, PickupItemsConfig::_pFuncIsUnitSkillTabMatch },
	{ "sorfireskills", 0x08, PickupItemsConfig::_pFuncIsUnitSkillTabMatch },
	{ "sorlightskills", 0x09, PickupItemsConfig::_pFuncIsUnitSkillTabMatch },
	{ "barbattlecryskills", 0x22, PickupItemsConfig::_pFuncIsUnitSkillTabMatch },
	{ "palcombatskills", 0x18, PickupItemsConfig::_pFuncIsUnitSkillTabMatch },
	{ "necpoisonskills", 0x11, PickupItemsConfig::_pFuncIsUnitSkillTabMatch },
	{ "necsummonskills", 0x12, PickupItemsConfig::_pFuncIsUnitSkillTabMatch },
	{ "amajavskills", 0x2, PickupItemsConfig::_pFuncIsUnitSkillTabMatch },
	{ "asntrapskills", 0x30, PickupItemsConfig::_pFuncIsUnitSkillTabMatch },

	{ "sorskills", 1, PickupItemsConfig::_pFuncIsUnitClassSkillMatch },
	{ "amaskills", 0, PickupItemsConfig::_pFuncIsUnitClassSkillMatch },
	{ "necskills", 2, PickupItemsConfig::_pFuncIsUnitClassSkillMatch },
	{ "palskills", 3, PickupItemsConfig::_pFuncIsUnitClassSkillMatch },
	{ "barskills", 4, PickupItemsConfig::_pFuncIsUnitClassSkillMatch },
	{ "druskills", 5, PickupItemsConfig::_pFuncIsUnitClassSkillMatch },
	{ "assskills", 6, PickupItemsConfig::_pFuncIsUnitClassSkillMatch },	
};

inline BOOL IsSeperator(char c)
{
	return c == ',';
}

inline BOOL IsValueSeperator(char c)
{
	return c == ':';
}

inline BOOL IsSpace(char c)
{
	return (c == ' ') || (c == '\t');
}

inline BOOL IsDigit(char c)
{
	return (c >= '0') && (c <= '9');
}

inline BOOL IsLetter(char c)
{
	return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'));
}

BOOL ParseNumber(const std::string& pStr, size_t StrStartPos, size_t& NumberEndPos, std::string& strNumber)
{
	int Pos = StrStartPos;
	int S1;
	while (pStr[Pos])
	{
		if (IsSpace(pStr[Pos]))
			Pos++;
		else
		{
			S1 = Pos;
			if (IsDigit(pStr[Pos]))
			{
				Pos++;
				while (pStr[Pos])
					if (IsDigit(pStr[Pos]))
						Pos++;
					else
						break;
				strNumber = pStr.substr(S1, Pos - S1);
				NumberEndPos = Pos;
				return TRUE;
			}
			else
			{
				NumberEndPos = Pos;
				return FALSE;
			}
		}
	}
	strNumber.clear();
	NumberEndPos = Pos;
	return FALSE;
}

BOOL ParseConfigName(const std::string& pStr, size_t StrStartPos, size_t& NameEndPos, std::string& strName)
{
	int Pos = StrStartPos;
	int S1;
	while (pStr[Pos])
	{
		if (IsSpace(pStr[Pos]))
			Pos++;
		else
		{
			S1 = Pos;
			if (IsLetter(pStr[Pos]))
			{
				Pos++;
				while (pStr[Pos])
					if (IsLetter(pStr[Pos]) || IsDigit(pStr[Pos]))
						Pos++;
					else
						break;
				strName = pStr.substr(S1, Pos - S1);
				NameEndPos = Pos;
				return TRUE;
			}
			else
			{
				NameEndPos = Pos;
				return FALSE;
			}
		}
	}
	strName.clear();
	NameEndPos = Pos;
	return TRUE;
}

BOOL ParseOperator(const std::string& pStr, size_t StrStartPos, size_t& StrEndPos, PickupItemsConfig::Operator& strOperator)
{
	int Pos = StrStartPos;
	strOperator = PickupItemsConfig::Operator::Undefined;
	while (pStr[Pos])
	{
		if (IsSpace(pStr[Pos]))
			Pos++;
		else
		{
			switch (pStr[Pos])
			{
			case '>':
				Pos++;
				if (pStr[Pos] == '=')
				{
					strOperator = PickupItemsConfig::Operator::GreaterOrEqual;
					Pos++;
				}
				else
					strOperator = PickupItemsConfig::Operator::Greater;
				StrEndPos = Pos;
				return TRUE;
			case '<':
				Pos++;
				if (pStr[Pos] == '=')
				{
					strOperator = PickupItemsConfig::Operator::LessOrEqual;
					Pos++;
				}
				else
					strOperator = PickupItemsConfig::Operator::Less;
				StrEndPos = Pos;
				return TRUE;
			case '=':
				Pos++;
				strOperator = PickupItemsConfig::Operator::Equal;
				StrEndPos = Pos;
				return TRUE;
			default:
				StrEndPos = Pos;
				return FALSE;
			}
		}
	}
	StrEndPos = Pos;
	return FALSE;	
}

BOOL ParseConfigValue(const std::string& pStr, size_t StrStartPos, size_t& StrEndPos, std::string& strValue)
{
	return ParseNumber(pStr, StrStartPos, StrEndPos, strValue);
}

BOOL ParseConfigValue2(const std::string& pStr, size_t StrStartPos, size_t& StrEndPos, std::string& strValue1, std::string& strValue2)
{
	if (ParseNumber(pStr, StrStartPos, StrEndPos, strValue1))
	{
		int Pos = StrEndPos;
		while (pStr[Pos])
		{
			if (IsSpace(pStr[Pos]))
				Pos++;
			else
				break;
		}

		if (IsValueSeperator(pStr[Pos]))
		{
			Pos++;
			if (ParseNumber(pStr, Pos, StrEndPos, strValue2))
				return TRUE;
		}
	}
	return FALSE;
}

BOOL ParseSeperator(const std::string& pStr, size_t StrStartPos, size_t& StrEndPos)
{
	int Pos = StrStartPos;

	while (pStr[Pos])
	{
		if (IsSpace(pStr[Pos]))
			Pos++;
		else
		{
			if (IsSeperator(pStr[Pos]))
			{
				Pos++;
				StrEndPos = Pos;
				return TRUE;
			}
			else
			{
				StrEndPos = Pos;
				return FALSE;
			}
		}
	}

	StrEndPos = Pos;
	return TRUE;
}

BOOL ParseItemConfig(std::string& pConfigStr)
{
	size_t Pos;
	if ((Pos = pConfigStr.find("//")) != std::string::npos)
		pConfigStr = pConfigStr.substr(0, Pos);
	
	if (pConfigStr.empty())
		return TRUE;

	Pos = 0;
	std::string strName, strValue,  strValue2;
	PickupItemsConfig::Operator Operator;
	char index;

	while (Pos < pConfigStr.length())
	{
		// parse config string
		if (!ParseConfigName(pConfigStr, Pos, Pos, strName))
			return FALSE;
		if (!strName.empty())
		{
			if (!ParseOperator(pConfigStr, Pos, Pos, Operator))
				return FALSE;
			if (strName != "addsingleskill")
			{
				if (!ParseConfigValue(pConfigStr, Pos, Pos, strValue))
					return FALSE;
			}
			else
			{
				if (!ParseConfigValue2(pConfigStr, Pos, Pos, strValue, strValue2))
					return FALSE;
			}
			if (pConfigStr[Pos] && !ParseSeperator(pConfigStr, Pos, Pos))
				return FALSE;

			//generate config item
			if (strName == "code")
				g_Game_Config.ItemsConfigForBot[g_Game_Config.ItemsConfigNum].Code = atoi(strValue.c_str());
			else if (strName == "quality")
				g_Game_Config.ItemsConfigForBot[g_Game_Config.ItemsConfigNum].Quality = atoi(strValue.c_str());
			else if ((index = PickupItemsConfig::FindAttr(strName.c_str())) != -1)
			{
				auto pAttr = new PickupItemsConfig::ItemConfigAttr();
				pAttr->index = index;
				pAttr->op = (char)Operator;
				if (strName == "addsingleskill")
				{
					pAttr->value = (atoi(strValue.c_str()) << 16) + atoi(strValue2.c_str());
				}
				else
					pAttr->value = atoi(strValue.c_str());
				pAttr->pNextAttr = g_Game_Config.ItemsConfigForBot[g_Game_Config.ItemsConfigNum].pFirstAttr;
				g_Game_Config.ItemsConfigForBot[g_Game_Config.ItemsConfigNum].pFirstAttr = pAttr;
			}
			else
			{
				SendHackMessage(D2RPY_LOG, "拾取物品配置文件格式有错误！");
				g_Game_Config.ItemsConfigForBot[g_Game_Config.ItemsConfigNum].erase();
				return FALSE;
			}
		}
	}
	//必须有code字段
	/*if (g_Game_Config.ItemsConfigForBot[g_Game_Config.ItemsConfigNum].Code)
		g_Game_Config.ItemsConfigNum++;
	else if(g_Game_Config.ItemsConfigForBot[g_Game_Config.ItemsConfigNum].Quality ||
		g_Game_Config.ItemsConfigForBot[g_Game_Config.ItemsConfigNum].pFirstAttr)
	{
		g_Game_Config.ItemsConfigForBot[g_Game_Config.ItemsConfigNum].erase();
		SendHackMessage(D2RPY_LOG, "拾取物品配置文件格式有错误！");
		return FALSE;
	}*/
	g_Game_Config.ItemsConfigNum++;

	return TRUE;
}

BOOL ReadItemsConfig(const char* itemsConfigFile)
{
	for (int i = 0; i < SIZEOFARRAY(g_Game_Config.ItemsConfigForBot); ++i)
		g_Game_Config.ItemsConfigForBot[i].erase();
	g_Game_Config.ItemsConfigNum = 0;

	std::ifstream ifs(itemsConfigFile);
	std::string line, tmp, name ,value;
	size_t pos;
	BOOL ret = TRUE;

	while (getline(ifs, line, '\n'))
	{
		if (g_Game_Config.ItemsConfigNum >= SIZEOFARRAY(g_Game_Config.ItemsConfigForBot))
			return FALSE;
		
		if ((pos = line.find("//")) != std::string::npos)
		{
			line = line.substr(0, pos);
		}
		
		if (!ParseItemConfig(line))
			ret = FALSE;
	}
	SortItemConfig();
	return ret;
}

int GetItemCode(int ItemNo)
{
	extern int *p_D2COMMON_nArmorTxt;
	extern int *p_D2COMMON_nWeaponsTxt;
	return (ItemNo < *p_D2COMMON_nWeaponsTxt) ? ItemNo :
		((ItemNo -= *p_D2COMMON_nWeaponsTxt) < *p_D2COMMON_nArmorTxt) ? ItemNo + 1000 :
		ItemNo - *p_D2COMMON_nArmorTxt + 1000 + 1000;		//为了与maphack的编码保持一致
}

void ReadConfigFile()
{
	memset(&g_Game_Config, 0, sizeof(g_Game_Config));
	char strTemp[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, strTemp);
	sprintf_s(strTemp + strlen(strTemp), sizeof(strTemp) - strlen(strTemp), "%s", "\\config.ini");
	GetPrivateProfileString("Account", "ACC", "", g_Game_Config.Acc_Name , sizeof(g_Game_Config.Acc_Name), strTemp);
	char *pCommentOut = strstr(g_Game_Config.Acc_Name, "//");
	if (pCommentOut)
		*pCommentOut = '\0';
	trim(g_Game_Config.Acc_Name);

	GetPrivateProfileString("Account", "Pass", "", g_Game_Config.Acc_Pass, sizeof(g_Game_Config.Acc_Pass), strTemp);
	pCommentOut = strstr(g_Game_Config.Acc_Pass, "//");
	if (pCommentOut)
		*pCommentOut = '\0';
	trim(g_Game_Config.Acc_Pass);

	g_Game_Config.Acc_Pos = GetPrivateProfileInt("Account", "Pos", 2, strTemp);
	g_Game_Config.PrimarySkill = GetPrivateProfileInt("Account", "PrimarySkill", 0, strTemp);
	g_Game_Config.SecondSkill = GetPrivateProfileInt("Account", "SecondSkill", 0, strTemp);
	g_Game_Config.DefendSkill = GetPrivateProfileInt("Account", "DefendSkill", 0, strTemp);
	g_Game_Config.AssistSkill = GetPrivateProfileInt("Account", "AssistSkill", 0, strTemp);
	g_Game_Config.PrimarySkillHand = GetPrivateProfileInt("Account", "PrimarySkillHand", 2, strTemp);
	g_Game_Config.SecondSkillHand = GetPrivateProfileInt("Account", "SecondSkillHand", 2, strTemp);
	g_Game_Config.AssistSkillHand = GetPrivateProfileInt("Account", "AssistSkillHand", 2, strTemp);
	g_Game_Config.PrimarySkillDelay = GetPrivateProfileInt("Account", "PrimarySkillDelay", 1500, strTemp);
	g_Game_Config.SecondSkillDelay = GetPrivateProfileInt("Account", "SecondSkillDelay", 250, strTemp);

	g_Game_Config.Use_Merc = GetPrivateProfileInt("Account", "UseMerc", 0, strTemp);
	g_Game_Config.ExitWhenMonsterImm = GetPrivateProfileInt("Account", "ExitWhenMonsterImmune", 0, strTemp);
	g_Game_Config.ReorganizeItem = GetPrivateProfileInt("Account", "ReorganizeItem", 0, strTemp);
	g_Game_Config.ReorganizeItemPosStartCol = GetPrivateProfileInt("Account", "ReorganizeItemPosStartCol", 0, strTemp);
	g_Game_Config.ReorganizeItemPosEndCol = GetPrivateProfileInt("Account", "ReorganizeItemPosEndCol", 0, strTemp);

	g_Game_Config.bEnableCubeGem = GetPrivateProfileInt("Cube", "EnableCubeGem", 0, strTemp);
	g_Game_Config.bEnableCubeGrandCharm = GetPrivateProfileInt("Cube", "EnableCubeGrandCharm", 0, strTemp);

	g_Game_Config.RandomGameName = GetPrivateProfileInt("Game", "RandomGameName", 0, strTemp);
	GetPrivateProfileString("Game", "Name", "", g_Game_Config.Game_Name, sizeof(g_Game_Config.Game_Name), strTemp);
	pCommentOut = strstr(g_Game_Config.Game_Name, "//");
	if (pCommentOut)
		*pCommentOut = '\0';
	trim(g_Game_Config.Game_Name);

	g_Game_Config.Game_Difficulty = GetPrivateProfileInt("Game", "Difficulty", 2, strTemp);

	//Gamble
	g_Game_Config.MinimumMoneyToStartGamble = GetPrivateProfileInt("Gamble", "MinimumMoneyToGamble", 0, strTemp);
	g_Game_Config.KeepMoneyAfterGamble = GetPrivateProfileInt("Gamble", "KeepMoneyAfterGamble", 100000, strTemp);
	char GambleItemsString[100];
	GetPrivateProfileString("Gamble", "GambleItems", "", GambleItemsString, sizeof(GambleItemsString), strTemp);
	
	pCommentOut =	strstr(GambleItemsString, "//");
	if (pCommentOut)
		*pCommentOut = '\0';
	trim(GambleItemsString);

	g_Game_Config.GambleItemsNum = 0;
	const char* pItems = GambleItemsString;
	while (pItems)
	{
		g_Game_Config.GambleItems[g_Game_Config.GambleItemsNum++] = atoi(pItems);
		pItems = strchr(pItems, ',');
		if (pItems)
			pItems++;
	}
	//end of Gamble
	
	GetPrivateProfileString("Boss", "BotTarget", "", g_Game_Config.TargetBoss, sizeof(g_Game_Config.TargetBoss), strTemp);
	_strlwr_s(g_Game_Config.TargetBoss);
	pCommentOut = strstr(g_Game_Config.TargetBoss, "//");
	if (pCommentOut)
		*pCommentOut = '\0';
	trim(g_Game_Config.TargetBoss);

	g_Game_Config.bClearMonsterNearby = GetPrivateProfileInt("Boss", "ClearMonsterNearby", 0, strTemp);

	char ItemsFile[MAX_PATH];
	GetPrivateProfileString("PickItemsConfig", "ConfigFile", "", ItemsFile, MAX_PATH, strTemp);
	if (!ReadItemsConfig(ItemsFile))
		SendHackMessage(D2RPY_LOG, "Critical Error! ***********捡拾物品配置格式有错误！！**********");
	else
		SendHackMessage(D2RPY_LOG, "BOT已成功读取配置文件！！");
}