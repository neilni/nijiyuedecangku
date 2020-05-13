// D2Hack1.11b.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "ShareData.h"
#include "misc.h"
#include "D2PtrHookHeader.h"
#include "D2Hack1.11b.h"

#include "A1Bot.cxx"
#include "D2HackBot.cxx"

//#define _91D2_BATTLENET
#define IMPK_BATTLENET

#define RESOLUTIONX 800
#define RESOLUTIONY 600
const short MAX_DIST_FOR_SINGLE_TP = 27;	//单次TP的最大距离

HK_GAME_INFO g_Game_Data;
HK_Game_Config g_Game_Config;
HANDLE g_hRunThreadHandle = NULL;

static BYTE ActLevels[] = { 1, 40, 75, 103, 109, 133 };

void ResetHackGlobalObj()
{
	memset(&g_Game_Data, 0, sizeof(g_Game_Data));
}

//返回当前游戏的Act序号，返回值为 1-5
DWORD GetCurrentAct(DrlgLevel* currlvl)
{
	if (!currlvl) return 0;

	DWORD l = currlvl->nLevelNo;
	DWORD act = 0;
	do {} while (l >= ActLevels[++act]);
	return act;
}

DrlgLevel *GetUnitDrlgLevel(UnitAny *unit)
{
	__try
	{
		if (unit && unit->pPos && unit->pPos->pRoom1) {
			return unit->pPos->pRoom1->pRoom2->pDrlgLevel;
		}
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		SendHackMessage(D2RPY_LOG, "Mem Exception in GetUnitDrlgLevel..., Close Current Game !");
		return NULL;
	}
	return NULL;
}

DrlgLevel* __fastcall GetDrlgLevel(DrlgMisc *drlgmisc, DWORD levelno)
{
	for (DrlgLevel* pDrlgLevel = drlgmisc->pLevelFirst; pDrlgLevel; pDrlgLevel = pDrlgLevel->pLevelNext)
	{
		if (pDrlgLevel->nLevelNo == levelno)
			return pDrlgLevel;
	}
	return D2COMMON_GetDrlgLevel(drlgmisc, levelno);
}

void SelectSkill(int skill, BYTE hand)
{
	BYTE packet[9] = { 0x3c };
	*(WORD*)&packet[1] = (WORD)skill;
	if (hand == 1)	//左手
		*(WORD*)&packet[3] = 0x8000;
	else
		*(WORD*)&packet[3] = 0;
	*(DWORD*)&packet[5] = 0xffffffff;
	D2NET_SendPacket(sizeof(packet), 0, packet);
}

void WaitToBeReadyToCastSkill(int MaxWaitTime)
{
	DWORD dw;
	WAIT_UNTIL(50, MaxWaitTime / 50, D2CLIENT_ReadyToCastSkill(D2Client_pPlayUnit, &dw), FALSE);
}

void CastDefendSkill()
{
	//cast defend skill
	if (!g_Game_Config.DefendSkill)
		return;

	SelectSkill(g_Game_Config.DefendSkill);
	Sleep(100);
	BYTE packet[9];
	packet[0] = 0xc;
	*(WORD*)&packet[1] = (WORD)(D2Client_pPlayUnit->pPos->xpos1 >> 16);
	*(WORD*)&packet[3] = (WORD)(D2Client_pPlayUnit->pPos->ypos1 >> 16);
	DWORD dw;
	if(D2CLIENT_ReadyToCastSkill(D2Client_pPlayUnit, &dw))
		D2NET_SendPacket(5, 0, packet);
	Sleep(100);
}

void CastSkillToMonster(WORD x, WORD y, BYTE hand)
{
	if (hand == 1)	//左手
		hand = 0x5;
	else
		hand = 0xc;
	DWORD dw;
	if (D2CLIENT_ReadyToCastSkill(D2Client_pPlayUnit, &dw))
	{
		BYTE packet[5] = { hand };
		*(WORD*)&packet[1] = x;
		*(WORD*)&packet[3] = y;
		D2NET_SendPacket(sizeof(packet), 0, packet);
	}
}

void CastSkillToMonster(DWORD id, BYTE hand)
{
	if (!IsMonsterKilled(id))
	{
		if (hand == 1) //左手
			hand = 0x6;
		else
			hand = 0xd;

		DWORD dw;
		if (D2CLIENT_ReadyToCastSkill(D2Client_pPlayUnit, &dw))
		{
			BYTE packet[9] = { hand };
			*(DWORD*)&packet[1] = 1;
			*(DWORD*)&packet[5] = id;
			D2NET_SendPacket(sizeof(packet), 0, packet);
		}
	}
}

//整理背包，把背包里的药水放回腰带
void CleanupInventory()
{
	UnitAny* item;
	BYTE packet[5];
	DWORD n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
	BOOL bInvChanged;
	while (n)
	{
		bInvChanged = FALSE;
		item = D2COMMON_GetUnitFromItem(n);
		if (item && !item->pItemData->nLocation2 && !item->pItemData->nLocation1)	// 身上的物品
		{
			switch (item->nTxtFileNo)
			{
			case 0x24f:
			case 0x24e:
				//血瓶
			case 0x254:
			case 0x253:
				//蓝瓶
			case 0x204:
			case 0x203:
				//大紫，小紫
			case 0x211:
				//回城卷轴
			case 0x212:
				//辨识卷轴
				//以上物品扔到地面
				packet[0] = 0x19;
				*(DWORD*)&packet[1] = item->nUnitId;
				D2NET_SendPacket(5, 0, packet);
				Sleep(300);

				packet[0] = 0x17;
				*(DWORD*)&packet[1] = item->nUnitId;
				D2NET_SendPacket(5, 0, packet);
				Sleep(300);

				//inventory has changes , re-iterate
				n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
				bInvChanged = TRUE;
				break;			
			}
		}
		if(!bInvChanged)
			n = D2COMMON_GetNextItemFromInventory(n);
	}

	SearchItemsWantedNearby(D2Client_pPlayUnit->pPos->pRoom1);
	PickItem();
	Sleep(300);
}

void CheckCorpseAndCleanupInventory()
{
	if (g_Game_Data.bPickupCorpse)
	{
		//pick up corpse
		BYTE packet[9] = { 0x13 };
		*(DWORD*)&packet[1] = UNITNO_PLAYER;
		*(DWORD*)&packet[5] = g_Game_Data.CorpseGUID;
		D2NET_SendPacket(9, 0, packet);
		Sleep(200);
	}
	CleanupInventory();
}

DWORD LoopCurrentLevelRoom2(WORD nLevelNo, std::function<DWORD(DrlgRoom2*)> pFuncProcRoom2)
{
	//InitCurrentLevelDrlgRoom()bvklc在GameLoopPatch中调用，用以同步，其他地方不要调用
	//InitCurrentLevelDrlgRoom();
	g_Game_Data.bLevelToBeInitialized = TRUE;
	WAIT_UNTIL(50, 100, !g_Game_Data.bLevelToBeInitialized, TRUE);

	DWORD bRet = FALSE;
	DrlgLevel *pDrlgLevel = GetDrlgLevel(D2CLIENT_pDrlgAct->pDrlgMisc, nLevelNo);
	if (!pDrlgLevel->pRoom2First)
		D2COMMON_InitDrlgLevel(pDrlgLevel);

	DrlgRoom2 *curroom = pDrlgLevel->pRoom2First;
	while (curroom)
	{
		if (bRet = pFuncProcRoom2(curroom))
			return bRet;
			
		curroom = curroom->pNext;
	}

	return bRet;
}

//用于在Room2里查找Preset Unit，比如各个场景的Waypoint等不变的unit
//与FindUnitGUIDPosFromTxtFileNo使用unit不同
BOOL GetLevelPresetObj(WORD nLevelNo, POINT& pt, DrlgRoom2** pRoom2, const char* strObjName, DWORD ObjNO)
{
	auto pFuncProcRoom2 = [&](DrlgRoom2* room2)->DWORD {
		PresetUnit *unit = room2->pPresetUnits;
		DWORD levelno = room2->pDrlgLevel->nLevelNo;

		while (unit) {
			if (unit->nUnitType == UNITNO_OBJECT)
			{
				//传送点(WayPoint)属于此类Unit。 
				//WayPoint 地址计算方法：x = (room2->xPos * 5 + unit->xPos)； y = (room2->yPos * 5 + unit->yPos)
				DWORD objno = unit->nTxtFileNo;
				if (ObjNO && (ObjNO == objno))
				{
					pt = POINT{ long(room2->xPos * 5 + unit->xPos), long(room2->yPos * 5 + unit->yPos) };
					if (pRoom2)
						*pRoom2 = room2;
					return TRUE;
				}
				else if (strObjName)
				{
					if (objno >= 574) goto _LoopRoom2Next;
					ObjectTxt *obj;
					obj = D2COMMON_GetObjectTxt(objno);
					if (obj && obj->szName && !strcmp(obj->szName, strObjName))
					{
						pt = POINT{ long(room2->xPos * 5 + unit->xPos), long(room2->yPos * 5 + unit->yPos) };
						if (pRoom2)
							*pRoom2 = room2;
						return TRUE;
					}
				}
			}
		_LoopRoom2Next:
			unit = unit->pNext;
		}
		return 0;
	};

	return LoopCurrentLevelRoom2(nLevelNo, pFuncProcRoom2);
}

//查找不同场景(Level)间的通道口，与GetEntrancePosAndRoom2不同！
BOOL FindRoom2BetweenLevels(WORD nLevelFrom, WORD nLevelTo, DrlgRoom2* &pRoomFrom, DrlgRoom2* &pRoomTo, DWORD RoomHasObjID)
{
	auto pFuncProcRoom2 = [&](DrlgRoom2* room2)->DWORD {
		for (DWORD i = 0; i < room2->nRoomsNear; ++i)
		{
			if ((room2->paRoomsNear[i]->pDrlgLevel->nLevelNo == nLevelTo) &&
				room2->pPresetUnits)
			{
				BOOL bHasTorch = FALSE;
				PresetUnit *unit = room2->pPresetUnits;
				while (unit) {
					//各个场景Level间的入口有火炬(nTxtNO = 0x25)，查找真正的入口
					if ((unit->nUnitType == UNITNO_OBJECT) && (unit->nTxtFileNo == RoomHasObjID/*0x25*/))
					{
						bHasTorch = TRUE;
						break;
					}
					unit = unit->pNext;
				}
				if (bHasTorch)
				{
					pRoomFrom = room2;
					pRoomTo = room2->paRoomsNear[i];
					return TRUE;
				}
				else
					return FALSE;
			}
		}
		return FALSE;
	};

	return LoopCurrentLevelRoom2(nLevelFrom, pFuncProcRoom2);
}

static DrlgRoom2 *GetRoomTileOtherRoom2(DrlgRoom2 *room2, DWORD roomtileno)
{
	RoomTile *roomtile = room2->pRoomTiles;
	while (roomtile) {
		if (*roomtile->nNum == roomtileno) {
			return roomtile->pRoom2;
		}
		roomtile = roomtile->pNext;
	}
	return 0;
}

BOOL GetEntrancePosAndRoom2(WORD lvfrom, WORD lvto, POINT& pt, DrlgRoom2* &retroom2)
{
	auto pFuncProcRoom2 = [&](DrlgRoom2* room2)->DWORD {
		PresetUnit *unit = room2->pPresetUnits;
		while (unit) {
			if (unit->nUnitType == UNITNO_ROOMTILE)  //不同level间的入口属于此类，如地下墓穴二层到三层的入口  by Liu
			{
				DrlgRoom2 *otherroom2 = GetRoomTileOtherRoom2(room2, unit->nTxtFileNo);
				if ((otherroom2) && (otherroom2->pDrlgLevel->nLevelNo == lvto))
				{
					pt = { long(room2->xPos * 5 + unit->xPos), long(room2->yPos * 5 + unit->yPos) };
					retroom2 = room2;
					return TRUE;
				}
			}
			unit = unit->pNext;
		}
		return FALSE;
	};

	return LoopCurrentLevelRoom2(lvfrom, pFuncProcRoom2);
}

void __fastcall PacketReceivedInterceptHook(BYTE* aPacket, DWORD aLength)
{
#ifdef _DEBUG
	if ((aPacket[0] != 0x8f) && (aPacket[0] != 0x67)&& (aPacket[0] != 0x68)&& (aPacket[0] != 0x6d) && (aPacket[0] != 0x6b)) {
		char tmp[256] = "Server Packet : ";
		for (int i = 0; i < 32; ++i) {
			sprintf_s(tmp + strlen(tmp), sizeof(tmp) - strlen(tmp), "%02x", aPacket[i]);
			if ((i + 1) % 2 == 0)
				sprintf_s(tmp + strlen(tmp), sizeof(tmp) - strlen(tmp), " ");
		}
		sprintf_s(tmp + strlen(tmp), sizeof(tmp) - strlen(tmp), "\n");
		DbgPrint(tmp);
	}

	/////////////tmp code
	//自动接受组队
	static BOOL bAccept = FALSE;
	static time_t InviteTime;
	static DWORD PlayerID;
	if ((aPacket[0] == 0x8b)&&(aPacket[5] == 2))//invite
	{
		bAccept = TRUE;
		PlayerID = *(DWORD*)&aPacket[1];
		InviteTime = clock();
	}
	if (bAccept && (clock() - InviteTime >500))
	{
		BYTE Pack[7];
		*(WORD*)&Pack[0] = 0x85e;
		*(DWORD*)&Pack[2] = PlayerID;
		D2NET_SendPacket(6, 0, Pack);
		bAccept = FALSE;
	}
	////////////
#endif


	//Back to town
	if (*(WORD*)&aPacket[0] == 0x251)
	{
		if (g_Game_Data.bBackToTown && (aLength > 6) && (*(WORD*)&aPacket[6] == 0x3B))
		{
			g_Game_Data.bBackToTown = FALSE;
			BYTE castMove[9] = { 0x13 };
			*(DWORD*)&castMove[1] = 2;
			*(DWORD*)&castMove[5] = *(DWORD*)&aPacket[2]; // Door ID
			D2NET_SendPacket(9, 0, castMove);
		}
	}
	else
		//69数据包，NPC state，判断boss是否已挂掉
		if (aPacket[0] == 0x69)
		{
			if (*(DWORD*)&aPacket[1] == g_Game_Data.MercGUID)
			{
				if (aPacket[5] == 9)
				{
					DbgPrint("佣兵挂了！退出！");
					SendHackMessage(D2RPY_LOG, "雇佣兵挂了！！！");
					/*if (g_hRunThreadHandle)
					{
						if (g_hRunThreadHandle)
						{
							//TerminateThread(g_hRunThreadHandle, -1);
							SuspendThread(g_hRunThreadHandle);
						}
						CloseGame();
						if (g_hRunThreadHandle)
						{
							ResumeThread(g_hRunThreadHandle);
						}
					}*/
					return;
				}
			}
		}
		else
			//assign merc,佣兵
			if (aPacket[0] == 0x81)
			{
				if (D2Client_pPlayUnit->nUnitId == *(DWORD*)&aPacket[4])
				{
					g_Game_Data.MercGUID = *(DWORD*)&aPacket[8];
				}
			}
			else
				//assign corpse,是不是已经挂了，捡尸体
				if (*(WORD*)&aPacket[0] == 0x0074)
				{
					if (D2Client_pPlayUnit->nUnitId == *(DWORD*)&aPacket[2])
					{
						g_Game_Data.bPickupCorpse = TRUE;
						g_Game_Data.CorpseGUID = *(DWORD*)&aPacket[6];
					}
				}
				else
					//SetState packet
					if (*(WORD*)&aPacket[0] == 0x00a8)
					{
						if ((D2Client_pPlayUnit->nUnitId == *(DWORD*)&aPacket[2]) && (aPacket[7] == 2))	//中毒
							g_Game_Data.bPlayerPoisoned = TRUE;
					}
					else
						//EndState Packet
						if (*(WORD*)&aPacket[0] == 0x00a9)
						{
							if ((D2Client_pPlayUnit->nUnitId == *(DWORD*)&aPacket[2]) && (aPacket[6] == 2))	//中毒
								g_Game_Data.bPlayerPoisoned = FALSE;
						}
						//else
							//Reassign player
							//if (*(WORD*)&aPacket[0] == 0x0015) {
							//	if (*(DWORD*)&aPacket[2] == D2Client_pPlayUnit->nUnitId)
							//		g_Game_Data.b0x15PacketReceived = TRUE;
							//}
#ifdef _DEBUG
							else if (aPacket[0] == 0x96)
							{
								WCHAR tmp[256] = L"My Pos:";
								swprintf(tmp + wcslen(tmp), SIZEOFARRAY(tmp) - wcslen(tmp), L"%d(0x%8x),%d(0x%8x)\n",
									(int)D2Client_pPlayUnit->pPos->xpos1 >> 16, (int)D2Client_pPlayUnit->pPos->xpos1,
									(int)D2Client_pPlayUnit->pPos->ypos1 >> 16, (int)D2Client_pPlayUnit->pPos->ypos1);

								D2CLIENT_PrintGameStringAtTopLeft(tmp, 4);
							}
#endif							
							else
								//9c packet
								if (*(WORD*)&aPacket[0] == 0x049c)
								{
									if (!g_Game_Data.nGambleItemID)
										g_Game_Data.nGambleItemID = *(DWORD*)&aPacket[4];
								}
}

BOOL PermShowItemsPatch()
{
	return g_Game_Config.bPermShowConfig || D2CLIENT_GetUiVar(UIVAR_SHOWITEMS);
}

void __fastcall KeydownPatch(BYTE keycode, BYTE repeat)
{
	if ((keycode == (BYTE)-1) || !keycode) return;

	if (keycode == 89) //Y
	{
		g_Game_Config.bPermShowConfig = !g_Game_Config.bPermShowConfig;
	}

	if (keycode == 69) // 'e'
	{
		if (g_hRunThreadHandle)
		{
			static bool bGamePause = true;
			if (bGamePause)
				SuspendThread(g_hRunThreadHandle);
			else
				ResumeThread(g_hRunThreadHandle);
			bGamePause = !bGamePause;
		}
	}
}

BOOL TestPlayerInTown(UnitAny *unit)
{
	DrlgLevel* curlvl = GetUnitDrlgLevel(unit);
	return curlvl ?
		curlvl->nLevelNo == 1	// act1
		|| curlvl->nLevelNo == 40	// act2
		|| curlvl->nLevelNo == 75	// act3
		|| curlvl->nLevelNo == 103	// act4
		|| curlvl->nLevelNo == 109	// act5
		: FALSE;
}

inline void ClearPickList()
{
	g_Game_Data.ItemToPickupNum = 0;
}

void AddItemToPickList(DWORD id, WORD x, WORD y, DWORD type)
{
	if (g_Game_Data.ItemToPickupNum < SIZEOFARRAY(g_Game_Data.ItemToPickup))
	{
		g_Game_Data.ItemToPickup[g_Game_Data.ItemToPickupNum].id = id;
		g_Game_Data.ItemToPickup[g_Game_Data.ItemToPickupNum].x = x;
		g_Game_Data.ItemToPickup[g_Game_Data.ItemToPickupNum].y = y;
		g_Game_Data.ItemToPickup[g_Game_Data.ItemToPickupNum].type = type;
		g_Game_Data.ItemToPickupNum++;
	}
}

BOOL IsItemPicked(DWORD id)
{
	for (int i = 0; i < g_Game_Data.ItemToPickupNum; ++i)
	{
		if (g_Game_Data.ItemToPickup[i].id == id)
		{
			return TRUE;
		}
	}
	return FALSE;
}

PickupItemsConfig::MatchResult IsItemWanted(UnitAny *item, const PickupItemsConfig& ItemConfig)
{
	int itemno = item->nTxtFileNo, quality = item->pItemData->nQuality;
	int ItemCode = GetItemCode(itemno) + 1;
	PickupItemsConfig::MatchResult MR = PickupItemsConfig::Match, tmp;
	if ((!ItemConfig.Code || (ItemConfig.Code == ItemCode)) && (!ItemConfig.Quality || (ItemConfig.Quality == quality)))
	{
		auto pItemConfigAttr = ItemConfig.pFirstAttr;
		while (pItemConfigAttr)
		{
			tmp = PickupItemsConfig::ItemConfigTable[pItemConfigAttr->index].pFuncIsItemAttrMatch(item,
				PickupItemsConfig::ItemConfigTable[pItemConfigAttr->index].Param, pItemConfigAttr);
			if (tmp == PickupItemsConfig::NotMatch)
				return PickupItemsConfig::NotMatch;
			else
			{
				if (tmp == PickupItemsConfig::NeedIdentify)
					MR = PickupItemsConfig::NeedIdentify;
			}
			pItemConfigAttr = pItemConfigAttr->pNextAttr;
		}
		return MR;
	}
	
	return PickupItemsConfig::NotMatch;
}

static BOOL IsUniqueEliteItem(UnitAny* item)
{
	int ItemCode = GetItemCode(item->nTxtFileNo) + 1;
	if (((ItemCode >= 190) && (ItemCode <= 276)) ||
		((ItemCode >= 297) && (ItemCode <= 306)) ||
		((ItemCode >= 1117) && (ItemCode <= 1162)) ||
		((ItemCode >= 1183) && (ItemCode <= 1202)))
	{
		if (item->pItemData->nQuality == 7)//暗金
			return TRUE;
	}
	return FALSE;
}

PickupItemsConfig::MatchResult IsItemWanted(UnitAny *item)
{
	int l = 0, h = g_Game_Config.ItemsConfigNum - 1, m;
	int itemno = item->nTxtFileNo;
	int ItemCode = GetItemCode(itemno) + 1;
	PickupItemsConfig::MatchResult MR = PickupItemsConfig::NotMatch;

#ifdef _91D2_BATTLENET
	if (IsUniqueEliteItem(item))
		return PickupItemsConfig::Match;
#endif

	//先匹配code=0的情形
	for (int i = 0; i < g_Game_Config.ItemsConfigNum; ++i)
	{
		if (!g_Game_Config.ItemsConfigForBot[i].Code)
		{
			PickupItemsConfig::MatchResult Tmp = IsItemWanted(item, g_Game_Config.ItemsConfigForBot[i]);
			if (Tmp == PickupItemsConfig::Match)
				return PickupItemsConfig::Match;
			else if (Tmp == PickupItemsConfig::NeedIdentify)
				MR = PickupItemsConfig::NeedIdentify;
		}
		else
			break;
	}

	while (l <= h)
	{
		m = (l + h) / 2;
		if (ItemCode < g_Game_Config.ItemsConfigForBot[m].Code)
			h = m - 1;
		else if (ItemCode > g_Game_Config.ItemsConfigForBot[m].Code)
			l = m + 1;
		else
		{
			//found one itemconfig
			while ((m >= 1) && (g_Game_Config.ItemsConfigForBot[m - 1].Code == ItemCode))
				m--;

			for (int i = m; i <= h; ++i)
			{
				PickupItemsConfig::MatchResult Tmp = IsItemWanted(item, g_Game_Config.ItemsConfigForBot[i]);
				if(Tmp == PickupItemsConfig::Match)
					return PickupItemsConfig::Match;
				else if(Tmp == PickupItemsConfig::NeedIdentify)
					MR = PickupItemsConfig::NeedIdentify;
			}
			return MR;
		}
	}

	return MR;
}

void __fastcall ItemNamePatch(wchar_t *name, UnitAny *item)
{
#ifdef _DEBUG
	static int first = 0;
	if (first != item->nUnitId)
		DbgPrint("IsItemWanted() == %d", IsItemWanted(item));
	first = item->nUnitId;
#endif

	int itemno = item->nTxtFileNo;
	int ItemCode = GetItemCode(itemno) + 1;
	//ItemTxt *itemtxt = D2COMMON_GetItemTxt(itemno);
	//if (itemtxt->dwCode == D2TXTCODE('gld ')) 
	if(itemno == 523)//gold
	{
		//FIX Gold Display
		wchar_t temp[0x40];
		// FIXME: name+0x40
		swprintf_s(temp, L"%d ", D2COMMON_GetUnitStat(item, STAT_GOLD, 0));
		wcsrcat(name, temp);
	}
	else if (item->pItemData->nItemLevel > 1)
	{
		//显示物品等级
		wchar_t temp[0x40];		
		swprintf_s(temp, L" (%d)", item->pItemData->nItemLevel);
		wcscat(name, temp);
	}
	else if ((ItemCode >= 2103)&(ItemCode <= 2135))
	{
		//显示符文序号
		wchar_t temp[0x40];
		swprintf_s(temp, L" (%d#)", ItemCode - 2102);
		wcscat(name, temp);
	}
}

void __stdcall ShakeScreenPatch(DWORD *xpos, DWORD *ypos)
{
	D2CLIENT_CalcShake(xpos, ypos);
	D2CLIENT_xMapShake = 0;
	D2CLIENT_yMapShake = 0;

	*xpos += D2CLIENT_xMapShake;
	*ypos += D2CLIENT_yMapShake;
}

void InitCurrentLevelDrlgRoom()
{
	DrlgLevel *pDrlgLevel = GetUnitDrlgLevel(D2Client_pPlayUnit);
	if (!pDrlgLevel->pRoom2First)
		D2COMMON_InitDrlgLevel(pDrlgLevel);
	DrlgRoom2 *currroom = pDrlgLevel->pRoom2First;
	while (currroom) {
		if (currroom->nPresetType == 2) {
			DWORD presetno = *currroom->nPresetType2No;
			if (presetno >= 38 && presetno <= 39) goto _InitNextRoom; //Act 1 - Swamp Fill 1&2
			if (presetno >= 401 && presetno <= 405) goto _InitNextRoom; //Act 2 - Desert Fill Bone 1&2, Desert Fill Wagon 1, Desert Fill Berms 1&2
			if (presetno == 836) goto _InitNextRoom; //Act 4 - Lava X
			if (presetno == 863) goto _InitNextRoom; //Act 5 - Town
		}
		if (!currroom->pRoom1) {
			BYTE cmdbuf[6];
			*(WORD *)(cmdbuf + 1) = (WORD)currroom->xPos;
			*(WORD *)(cmdbuf + 3) = (WORD)currroom->yPos;
			cmdbuf[5] = (BYTE)currroom->pDrlgLevel->nLevelNo;
			D2CLIENT_RecvCommand07(cmdbuf);			
			D2CLIENT_RecvCommand08(cmdbuf);
		}
		_InitNextRoom:
		currroom = currroom->pNext;
	}
}

void RevealCurrentLevelMap()
{
	if (!D2Client_pPlayUnit)
		return;

	DrlgLevel *pDrlgLevel = GetUnitDrlgLevel(D2Client_pPlayUnit);
	if (!pDrlgLevel->pRoom2First)
		D2COMMON_InitDrlgLevel(pDrlgLevel);
	DrlgRoom2 *currroom = pDrlgLevel->pRoom2First;
	while (currroom) {
		if (currroom->pRoom1) {
			D2CLIENT_RevealAutomapRoom(currroom->pRoom1, 1, D2CLIENT_pAutomapLayer);
		}
		else {
			BYTE cmdbuf[6];
			*(WORD *)(cmdbuf + 1) = (WORD)currroom->xPos;
			*(WORD *)(cmdbuf + 3) = (WORD)currroom->yPos;
			cmdbuf[5] = (BYTE)currroom->pDrlgLevel->nLevelNo;
			D2CLIENT_RecvCommand07(cmdbuf);
			D2CLIENT_RevealAutomapRoom(currroom->pRoom1, 1, D2CLIENT_pAutomapLayer);
			D2CLIENT_RecvCommand08(cmdbuf);
		}
		currroom = currroom->pNext;
	}
}

static BOOL IsCornerRoom(const DrlgRoom2* room)
{
	/*int NeighborRoomCount = 0;
	for (DWORD i = 0; i < room->nRoomsNear; ++i)
	{
		if (room->paRoomsNear[i] == room)
			continue;
		DWORD roomx = room->paRoomsNear[i]->xPos;
		DWORD roomy = room->paRoomsNear[i]->yPos;

		if ((((DWORD)abs((int)(roomx - room->xPos)) <= room->xPos1) && (roomy == room->yPos)) ||
			(((DWORD)abs((int)(roomy - room->yPos)) <= room->yPos1) && (roomx == room->xPos)))
		{
			NeighborRoomCount++;
		}
	}*/
	return room->nRoomsNear < 9;
}

BOOL GeneratePathToMonsterRoom(const DrlgRoom2 *pStart, const DrlgRoom2* pDest, MonsterPathTree& PathSource, MonsterPathTree& PathDest)
{
	////////////找路算法////////////
	//从PathDest开始向PathSource反向生成路径树，反向生成树的目的是可以立即获取一条最佳路径
	DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
	BOOL bFound = FALSE;

	MonsterPathTree* Item;
	MonsterPathTree* pTail = NULL;
	InitMonsterPathTreeNode(PathDest);
	InitMonsterPathTreeNode(PathSource);
	PathDest.pRoom = (LPVOID)pDest;
	PathDest.RoomPos.x = (pDest)->xPos;
	PathDest.RoomPos.y = (pDest)->yPos;
	PathDest.RoomPos.size.cx = (pDest)->xPos1;
	PathDest.RoomPos.size.cy = (pDest)->yPos1;

	SimplePathHeap buffer;
	buffer.PushHeapItem(&PathDest);	//PathDest是第一个node，即生成树的树根

	//tempory function (anonymous function)
	auto pFuncAddNewPathTreeNode = [](MonsterPathTree* ParentTreeNode, const DrlgRoom2* room, MonsterPathTree* &pTreeTail) {
		MonsterPathTree tmp;
		InitMonsterPathTreeNode(tmp);
		tmp.pRoom = (LPVOID)room;
		tmp.RoomPos.x = room->xPos;
		tmp.RoomPos.y = room->yPos;
		tmp.RoomPos.size.cx = room->xPos1;
		tmp.RoomPos.size.cy = room->yPos1;
		NewNodeAndAttachtoPathTree(ParentTreeNode, tmp);
		pTreeTail = &ParentTreeNode->pNextNodeList[ParentTreeNode->NextNum - 1];
	};

	DWORD roomx, roomy, itemx, itemy;
	while (!bFound && buffer.PopHeapItem(Item))
	{
		itemx = ((DrlgRoom2*)Item->pRoom)->xPos;
		itemy = ((DrlgRoom2*)Item->pRoom)->yPos;

		DrlgRoom2* pCornerRoom[8];
		int nCornerRoomCount = 0;
		for (DWORD i = 0; i < ((DrlgRoom2*)Item->pRoom)->nRoomsNear; ++i)
		{
			roomx = ((DrlgRoom2*)Item->pRoom)->paRoomsNear[i]->xPos;
			roomy = ((DrlgRoom2*)Item->pRoom)->paRoomsNear[i]->yPos;

			if ((((DWORD)abs((int)(roomx - itemx)) <= ((DrlgRoom2*)Item->pRoom)->paRoomsNear[i]->xPos1) && (roomy == itemy)) ||
				(((DWORD)abs((int)(roomy - itemy)) <= ((DrlgRoom2*)Item->pRoom)->paRoomsNear[i]->yPos1) && (roomx == itemx)))
			{
				//优先搜索中间的房间，边角房间围墙有时导致TP失效
				if(IsCornerRoom(((DrlgRoom2*)Item->pRoom)->paRoomsNear[i]))
				{
					pCornerRoom[nCornerRoomCount++] = ((DrlgRoom2*)Item->pRoom)->paRoomsNear[i];
				}
				else
				{
					if (!PathTreeNodeAlreadyExist(&PathDest, (LPVOID)((DrlgRoom2*)Item->pRoom)->paRoomsNear[i]))
					{
						pFuncAddNewPathTreeNode(Item, ((DrlgRoom2*)Item->pRoom)->paRoomsNear[i], pTail);

						if(pStart == ((DrlgRoom2*)Item->pRoom)->paRoomsNear[i])
						{
							bFound = TRUE;
							break;
						}
						else
							buffer.PushHeapItem(pTail);
					}
				}
			}
		}
		if (!bFound)
			for (int k = 0; k < nCornerRoomCount; ++k)
			{
				if (!PathTreeNodeAlreadyExist(&PathDest, (LPVOID)pCornerRoom[k]))
				{
					pFuncAddNewPathTreeNode(Item, pCornerRoom[k], pTail);
					
					if (pStart == pCornerRoom[k])
					{
						bFound = TRUE;
						break;
					}					
					else
						buffer.PushHeapItem(pTail);
				}
			}
	}

	if(pTail)
		PathSource = *pTail;
	
	return bFound;
}

void UsePotion(BOOL bUseLife, BOOL bUseMana, BOOL bNeedZiping, BOOL bPlayerOrMerc = 1)
{
	short col = -1;
	static time_t tPlayerLastDrinkLifeTime = 0;
	static time_t tPlayerLastDrinkZipingTime = 0;
	static time_t tPlayerLastDrinkManaTime = 0;
	static time_t tMercLastDrinkTime = 0;
	static time_t tMercLastDrinkZipingTime = 0;
	
	UpdateBeltPotionPos();
	if (bNeedZiping)
	{
		if (g_Game_Data.Potions[2].PotionGUID != 0)
			col = 2;
		else if (g_Game_Data.Potions[3].PotionGUID != 0)
			col = 3;
		else
			col = 0;
	}
	else if (bUseLife)
	{
		if (g_Game_Data.Potions[0].PotionGUID)
			col = 0;
	}
	
	if ((col != -1) && g_Game_Data.Potions[col].PotionGUID)
	{
		if (col == 0)
		{
			if (bPlayerOrMerc && ((clock() - tPlayerLastDrinkLifeTime) < 2500))
				return;	//dont drink too often
			else if (!bPlayerOrMerc && ((clock() - tMercLastDrinkTime) < 4500))
				return;

			if (bPlayerOrMerc)
				tPlayerLastDrinkLifeTime = clock();
			else
				tMercLastDrinkTime = clock();
		}
		else if (col == 1)
		{
			if (bPlayerOrMerc && ((clock() - tPlayerLastDrinkManaTime) < 2500))
				return;	//dont drink too often
			tPlayerLastDrinkManaTime = clock();
		}
		else
		{
			if (bPlayerOrMerc && ((clock() - tPlayerLastDrinkZipingTime) < 1500))
				return;	//dont drink too often
			else if (!bPlayerOrMerc && ((clock() - tMercLastDrinkZipingTime) < 2000))
				return;

			if (bPlayerOrMerc)
				tPlayerLastDrinkZipingTime = clock();
			else
				tMercLastDrinkZipingTime = clock();
		}
		BYTE packet[13];
		memset(packet, 0, sizeof(packet));
		packet[0] = 0x26;
		*(DWORD*)&packet[1] = g_Game_Data.Potions[col].PotionGUID;
		if (!bPlayerOrMerc)
			*(DWORD*)&packet[5] = 1;	//1给佣兵药水，0给玩家
		D2NET_SendPacket(13, 0, packet);
	}

	if (bUseMana)
	{
		if (bPlayerOrMerc && ((clock() - tPlayerLastDrinkManaTime) < 2500))
			return;	//dont drink too often
		col = g_Game_Data.Potions[1].PotionGUID ? 1 : g_Game_Data.Potions[2].PotionGUID ? 2 : g_Game_Data.Potions[3].PotionGUID ? 3 : -1;
		if (col != -1)
		{			
			tPlayerLastDrinkManaTime = clock();
			BYTE packet[13];
			memset(packet, 0, sizeof(packet));
			packet[0] = 0x26;
			*(DWORD*)&packet[1] = g_Game_Data.Potions[col].PotionGUID;
			D2NET_SendPacket(13, 0, packet);
		}
	}
}

int GetItemCountInCube(UnitAny*& pItem)
{
	UnitAny* item;
	DWORD n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
	int nCount = 0;
	while (n)
	{
		item = D2COMMON_GetUnitFromItem(n);
		if (item->pItemData->nLocation2 == 3)
		{
			pItem = item;
			nCount++;
		}
		n = D2COMMON_GetNextItemFromInventory(n);
	}
	return nCount;
}

BOOL PutItemIntoCube(UnitAny* item, DWORD CubeID)
{
	BYTE Packet[9];
	Packet[0] = 0x19;
	*(DWORD*)&Packet[1] = item->nUnitId;
	D2NET_SendPacket(5, 0, Packet);
	WAIT_UNTIL(100, 200, item->eMode == 4, FALSE);	//等到item拿到鼠标上

	if (item->eMode != 4)
		return FALSE;

	Packet[0] = 0x2a;
	*(DWORD*)&Packet[1] = item->nUnitId;
	*(DWORD*)&Packet[5] = CubeID;
	D2NET_SendPacket(9, 0, Packet);
	WAIT_UNTIL(100, 100, item->pItemData->nLocation2 == 3, FALSE);	//等到item放进Cube里

	if (item->pItemData->nLocation2 != 3)
		return FALSE;
	return TRUE;
}

BOOL CubeGem(DWORD nCode, DWORD CubeID)
{
	UnitAny* item;
	DWORD n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
	DWORD ItemCode;
	BYTE Packet[13];
	short GemCount = 0;
	while (n)
	{
		item = D2COMMON_GetUnitFromItem(n);
		ItemCode = GetItemCode(item->nTxtFileNo) + 1;
		if ((ItemCode == nCode) &&
			(!item->pItemData->nLocation2 || (item->pItemData->nLocation2 == 4)))
		{
			PutItemIntoCube(item, CubeID);
			if (++GemCount == 3)
				break;
			n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
		}
		else
			n = D2COMMON_GetNextItemFromInventory(n);
	}

	//cube
	Packet[0] = 0x20;
	*(DWORD*)&Packet[1] = CubeID;
	*(WORD*)&Packet[5] = D2Client_pPlayUnit->pPos->xpos1 >> 16;
	*(WORD*)&Packet[7] = 0;
	*(WORD*)&Packet[9] = D2Client_pPlayUnit->pPos->ypos1 >> 16;
	*(WORD*)&Packet[11] = 0;
	D2NET_SendPacket(13, 0, Packet);

	WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_CUBE), FALSE);

	if (D2CLIENT_GetUiVar(UIVAR_CUBE))
	{
		GetItemCountInCube(item);
		DWORD ItemID = item->nUnitId;

		Packet[0] = 0x4f;
		*(DWORD*)&Packet[1] = 0x18;
		*(WORD*)&Packet[5] = 0;
		D2NET_SendPacket(7, 0, Packet);
		
		WAIT_UNTIL(400, 20, (GetItemCountInCube(item) == 1) && (ItemID != item->nUnitId) && item->pItemPath, FALSE);

		char cName[256];
		memset(cName, 0, sizeof(cName));
		GetItemName(item, cName);

		D2HackReply::ItemInfo replyInfo;
		replyInfo.Quality = (char)item->pItemData->nQuality;
		strcpy_s(replyInfo.ItemName, sizeof(replyInfo.ItemName), cName);
		strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "成功合成物品，[ %s ]。");
		SendHackMessage(D2RPY_FOUND_ITEM, replyInfo);

		WORD width, height, x, y;
		GetItemSize(item, width, height, 3);
		//SendHackMessage(D2RPY_LOG, "[ %s ]宽度：%d，高度：%d 。", cName, width, height);
		if (FindPosToStoreItemToBank(width, height, x, y))
		{
			//SendHackMessage(D2RPY_LOG, "将合成物品放回箱子...");

			BYTE packet[17];
			packet[0] = 0x19;
			*(DWORD*)&packet[1] = item->nUnitId;
			D2NET_SendPacket(5, 0, packet);
			WAIT_UNTIL(200, 30, item->eMode == 4, TRUE);	//等到item拿到鼠标上

			CloseMenu(UIVAR_CUBE);
			if (!D2CLIENT_GetUiVar(UIVAR_STASH))
				InteractObject(267);

			packet[0] = 0x18;
			*(DWORD*)&packet[1] = item->nUnitId;
			*(DWORD*)&packet[5] = x;
			*(DWORD*)&packet[9] = y;
			*(DWORD*)&packet[13] = 4;	//存储到宝箱里
			D2NET_SendPacket(17, 0, packet);
			WAIT_UNTIL(200, 30, item->pItemData->nLocation2 == 4, TRUE);	//等到item放进宝箱里

			//SendHackMessage(D2RPY_LOG, "合成物品已放回箱子。");			
			//DepositItem(pItem->nUnitId, x, y);
			//CloseMenu(UIVAR_CUBE);
			return TRUE;
		}
	}
	return FALSE;
}

void GotoCubeGem(StartLocation& s, DWORD ItemCode, DWORD CubeID)
{
	if (GotoBank(s))
	{
		InteractObject(267);
		WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_STASH), FALSE);

		if (D2CLIENT_GetUiVar(UIVAR_STASH))
		{
			while (ReadyToCubeGem(ItemCode, CubeID))
			{
				SendHackMessage(D2RPY_LOG, "开始合成宝石...");
				if (!CubeGem(ItemCode, CubeID))
					break;
				if(!D2CLIENT_GetUiVar(UIVAR_STASH))
					InteractObject(267);
			}

			SendHackMessage(D2RPY_LOG, "结束合成宝石。");
			CloseMenu(UIVAR_STASH);
			Sleep(100);
		}
	}
}

BOOL ReadyToCubeGem(DWORD& nCubeItemCode, DWORD& CubeID)
{
	/*Flawless-Amethyst----无瑕疵紫宝石----gzv--2053
	Flawless-Diamond-----无瑕疵钻石------glw--2078
	Flawless-Emerald-----无瑕疵绿宝石----glg--2068
	Flawless-Ruby--------无瑕疵红宝石----glr--2073
	Flawless-Saphire-----无瑕疵蓝宝石----glb--2063
	Flawless-Skull-------无瑕疵骷髅------skl--2093
	Flawless-Topaz-------无瑕疵黄宝石----gly--2058
	--
	Perfect-Diamond------完美钻石--------gpw--2079
	Perfect-Amethyst-----完美紫宝石------gpv--2054
	Perfect-Sapphire-----完美蓝宝石------gpb--2064
	Perfect-Topaz--------完美黄宝石------gpy--2059
	Perfect-Ruby---------完美红宝石------gpr--2074
	Perfect-Skull--------完美骷髅--------skz--2094
	Perfect-Emerald------完美绿宝石------gpg--2069
	*/
	short nAme = 0, nDia = 0, nEme = 0, nRuby = 0, nSap = 0, nSkul = 0, nTopaz = 0;
	UnitAny* item;
	DWORD n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
	DWORD ItemCode;
	BOOL bIsCubeEmpty = TRUE;
	CubeID = 0;
	while (n)
	{
		item = D2COMMON_GetUnitFromItem(n);
		ItemCode = GetItemCode(item->nTxtFileNo) + 1;
		if (!item->pItemData->nLocation2 || (item->pItemData->nLocation2 == 4))
		{
			if (ItemCode == 2053)
				nAme++;
			if (ItemCode == 2078)
				nDia++;
			if (ItemCode == 2068)
				nEme++;
			if (ItemCode == 2073)
				nRuby++;
			if (ItemCode == 2063)
				nSap++;
			if (ItemCode == 2093)
				nSkul++;
			if (ItemCode == 2058)
				nTopaz++;

			if (item->nTxtFileNo == 549)	//找到合成方块
				CubeID = item->nUnitId;
		}
		else if (item->pItemData->nLocation2 == 3)
			bIsCubeEmpty = FALSE;
		
		n = D2COMMON_GetNextItemFromInventory(n);
	}

	BOOL bol = FALSE;
	if (nAme >= 3)
		nCubeItemCode = 2053, bol = TRUE;
	if (nDia >= 3)
		nCubeItemCode = 2078, bol = TRUE;
	if (nEme >= 3)
		nCubeItemCode = 2068, bol = TRUE;
	if (nRuby >= 3)
		nCubeItemCode = 2073, bol = TRUE;
	if (nSap >= 3)
		nCubeItemCode = 2063, bol = TRUE;
	if (nSkul >= 3)
		nCubeItemCode = 2093, bol = TRUE;
	if (nTopaz >= 3)
		nCubeItemCode = 2058, bol = TRUE;

	return bol && bIsCubeEmpty && CubeID;
}

BOOL ReadyToCubeGrandCharm(DWORD& Gem1, DWORD& Gem2, DWORD& Gem3, DWORD& GrandCharm, DWORD& CubeID)
{
	UnitAny* item;
	DWORD n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
	DWORD ItemCode;
	BOOL bIsCubeEmpty = TRUE;
	GrandCharm = CubeID = Gem1 = Gem2 = Gem3 = 0;
	while (n)
	{
		item = D2COMMON_GetUnitFromItem(n);
		ItemCode = GetItemCode(item->nTxtFileNo) + 1;
		if (!item->pItemData->nLocation2 || (item->pItemData->nLocation2 == 4))
		{
			if ((ItemCode == 2054) || (ItemCode == 2079) || (ItemCode == 2069) || (ItemCode == 2074) || (ItemCode == 2064)
				|| (ItemCode == 2094) || (ItemCode == 2059))
			{
				Gem1 ? (Gem2 ? (Gem3 ? 0 : Gem3 = item->nUnitId) : Gem2 = item->nUnitId) : Gem1 = item->nUnitId;
			}
			else if (ItemCode == 2098)	//GrandCharm
			{
				if ((item->pItemData->nItemLevel >= 90) && D2COMMON_GetItemFlag(item, ITEMFLAG_IDENTIFIED, 0, "?") &&
					(IsItemWanted(item) != PickupItemsConfig::Match))
					GrandCharm = item->nUnitId;
			}

			if (item->nTxtFileNo == 549)	//找到合成方块
				CubeID = item->nUnitId;
		}
		else if (item->pItemData->nLocation2 == 3)
			bIsCubeEmpty = FALSE;

		n = D2COMMON_GetNextItemFromInventory(n);
	}
	return Gem1 && Gem2 && Gem3 && GrandCharm && bIsCubeEmpty && CubeID;
}

void GotoCubeGrandCharm(StartLocation& s)
{
	DWORD Gem1, Gem2, Gem3, GrandCharm, CubeID;
	if (GotoBank(s))
	{
		InteractObject(267);
		WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_STASH), FALSE);

		if (D2CLIENT_GetUiVar(UIVAR_STASH))
		{
			while (ReadyToCubeGrandCharm(Gem1, Gem2, Gem3, GrandCharm, CubeID))
			{
				SendHackMessage(D2RPY_LOG, "开始洗超大护身符...");
				UnitAny *pGemItem1, *pGemItem2, *pGemItem3, *pGramdCharm;
				pGemItem1 = D2CLIENT_GetUnitFromId(Gem1, UNITNO_ITEM);
				pGemItem2 = D2CLIENT_GetUnitFromId(Gem2, UNITNO_ITEM);
				pGemItem3 = D2CLIENT_GetUnitFromId(Gem3, UNITNO_ITEM);
				pGramdCharm = D2CLIENT_GetUnitFromId(GrandCharm, UNITNO_ITEM);

				PutItemIntoCube(pGemItem1, CubeID);
				PutItemIntoCube(pGemItem2, CubeID);
				PutItemIntoCube(pGemItem3, CubeID);
				PutItemIntoCube(pGramdCharm, CubeID);

				//cube
				BYTE Packet[17];
				Packet[0] = 0x20;
				*(DWORD*)&Packet[1] = CubeID;
				*(WORD*)&Packet[5] = D2Client_pPlayUnit->pPos->xpos1 >> 16;
				*(WORD*)&Packet[7] = 0;
				*(WORD*)&Packet[9] = D2Client_pPlayUnit->pPos->ypos1 >> 16;
				*(WORD*)&Packet[11] = 0;
				D2NET_SendPacket(13, 0, Packet);

				WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_CUBE), FALSE);

				if (D2CLIENT_GetUiVar(UIVAR_CUBE))
				{
					UnitAny* pItem = NULL;
					GetItemCountInCube(pItem);
					DWORD ItemID = pItem->nUnitId;

					Packet[0] = 0x4f;
					*(DWORD*)&Packet[1] = 0x18;
					*(WORD*)&Packet[5] = 0;
					D2NET_SendPacket(7, 0, Packet);
					
					WAIT_UNTIL(800, 10, (GetItemCountInCube(pItem) == 1) && (ItemID != pItem->nUnitId) && pItem->pItemPath, FALSE);

					char cName[256];
					memset(cName, 0, sizeof(cName));
					GetItemName(pItem, cName);

					D2HackReply::ItemInfo replyInfo;
					replyInfo.Quality = (char)pItem->pItemData->nQuality;
					strcpy_s(replyInfo.ItemName, sizeof(replyInfo.ItemName), cName);

					if (IsItemWanted(pItem) == PickupItemsConfig::Match)
					{						
						strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "洗超大护身符获取物品，[ %s ]。");
						SendHackMessage(D2RPY_OBTAIN_ITEM, replyInfo);
					}
					else
					{
						strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "新超大护身符，[ %s ]。");
						SendHackMessage(D2RPY_FOUND_ITEM, replyInfo);
					}

					WORD width, height, x, y;
					GetItemSize(pItem, width, height, 3);
					if (FindPosToStoreItemToBank(width, height, x, y))
					{
						BYTE packet[17];
						packet[0] = 0x19;
						*(DWORD*)&packet[1] = pItem->nUnitId;
						D2NET_SendPacket(5, 0, packet);
						WAIT_UNTIL(200, 30, pItem->eMode == 4, TRUE);	//等到item拿到鼠标上

						CloseMenu(UIVAR_CUBE);
						if (!D2CLIENT_GetUiVar(UIVAR_STASH))
							InteractObject(267);

						packet[0] = 0x18;
						*(DWORD*)&packet[1] = pItem->nUnitId;
						*(DWORD*)&packet[5] = x;
						*(DWORD*)&packet[9] = y;
						*(DWORD*)&packet[13] = 4;	//存储到宝箱里
						D2NET_SendPacket(17, 0, packet);
						WAIT_UNTIL(200, 30, pItem->pItemData->nLocation2 == 4, TRUE);	//等到item放进宝箱里
						
						//DepositItem(pItem->nUnitId, x, y);
						//CloseMenu(UIVAR_CUBE);
					}
				}

				if (!D2CLIENT_GetUiVar(UIVAR_STASH))
					InteractObject(267);
			}

			SendHackMessage(D2RPY_LOG, "结束洗超大护身符。");

			CloseMenu(UIVAR_STASH);
			Sleep(100);
		}
	}
}

void PrepareBeforeFight(StartLocation& s)
{
	BOOL bNeedCure;
	if (CheckLifeAndMana(&bNeedCure) && bNeedCure)
		GotoGetHeal(s);

	BOOL bNeedBloodPotion, bNeedManaPotion, bIdentifyItems, bNeedRepair, bBuyTownScroll;
	CheckPlayerStatus(bNeedBloodPotion, bNeedManaPotion, bIdentifyItems, bNeedRepair, bBuyTownScroll);
	if (bNeedBloodPotion || bNeedManaPotion || bIdentifyItems || bBuyTownScroll)
		GotoIdentifyAndBuyPotions(s);
	if (bNeedRepair)
		GotoRepairItems(s);

	//整理背包
	BOOL bReorgItems = FALSE;
	WORD width, height, x, y;
	if (g_Game_Config.ReorganizeItem)
	{
		for (int PosX = g_Game_Config.ReorganizeItemPosStartCol; PosX <= g_Game_Config.ReorganizeItemPosEndCol; ++PosX)
		{
			for (int PosY = 0; PosY <= 3; ++PosY)
			{
				UnitAny* pItem = GetPosUnit(PosX, PosY, 0);
				if (pItem && 
					(D2COMMON_GetItemFlag(pItem, ITEMFLAG_IDENTIFIED, 0, "?") ||(IsItemWanted(pItem) == PickupItemsConfig::Match)))
				{
					GetItemSize(pItem, width, height, 0);
					if (FindPosToStoreItemToBank(width, height, x, y))
						bReorgItems = TRUE;
				}
			}
		}
	}
	if (bReorgItems)
		GotoReorgnizeInventoryItems(s);

	if (g_Game_Config.bEnableCubeGem || g_Game_Config.bEnableCubeGrandCharm)
	{
		//如果cube里有物品，整理cube里的物品
		UnitAny *pItem;
		if (GetItemCountInCube(pItem))
		{
			WORD width, height, x, y;
			GetItemSize(pItem, width, height, 3);
			if (FindPosToStoreItemToBank(width, height, x, y))
			{
				if (GotoBank(s))
				{
					InteractObject(267);
					WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_STASH), FALSE);

					if (D2CLIENT_GetUiVar(UIVAR_STASH))
					{
						/*
						POINT pt;
						BYTE packet[17];
						packet[0] = 0x20;
						*(DWORD*)&packet[1] = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 267, UNITNO_OBJECT);;
						*(WORD*)&packet[5] = D2Client_pPlayUnit->pPos->xpos1 >> 16;
						*(WORD*)&packet[7] = 0;
						*(WORD*)&packet[9] = D2Client_pPlayUnit->pPos->ypos1 >> 16;
						*(WORD*)&packet[11] = 0;
						D2NET_SendPacket(13, 0, packet);
						WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_CUBE), FALSE);
						
						packet[0] = 0x19;
						*(DWORD*)&packet[1] = pItem->nUnitId;
						D2NET_SendPacket(5, 0, packet);
						WAIT_UNTIL(200, 30, pItem->eMode == 4, TRUE);	//等到item拿到鼠标上

						CloseMenu(UIVAR_CUBE);
						if (!D2CLIENT_GetUiVar(UIVAR_STASH))
							InteractObject(267);

						packet[0] = 0x18;
						*(DWORD*)&packet[1] = pItem->nUnitId;
						*(DWORD*)&packet[5] = x;
						*(DWORD*)&packet[9] = y;
						*(DWORD*)&packet[13] = 4;	//存储到宝箱里
						D2NET_SendPacket(17, 0, packet);
						WAIT_UNTIL(200, 30, pItem->pItemData->nLocation2 == 4, TRUE);	//等到item放进宝箱里
						*/
						DepositItem(pItem->nUnitId, x, y);
					}

					if (D2CLIENT_GetUiVar(UIVAR_STASH))
						CloseMenu(UIVAR_STASH);
				}
			}
		}
	}

	//Cube宝石
	DWORD ItemCode, CubeID;
	if (g_Game_Config.bEnableCubeGem && ReadyToCubeGem(ItemCode, CubeID))
		GotoCubeGem(s, ItemCode, CubeID);

	//Cube超大护身符
	DWORD Gem1, Gem2, Gem3, Charm;
	if (g_Game_Config.bEnableCubeGrandCharm && ReadyToCubeGrandCharm(Gem1, Gem2, Gem3, Charm, CubeID))
		GotoCubeGrandCharm(s);
}

BOOL CheckLifeAndMana(BOOL *bNeedHeal)
{
	if (bNeedHeal)
		*bNeedHeal = FALSE;

	if (g_Game_Data.bPlayerPoisoned && bNeedHeal)
		*bNeedHeal = TRUE;

	unsigned int life = D2COMMON_GetUnitStat(D2Client_pPlayUnit, STAT_LIFE, 0);
	life >>= 8; // actual life
	unsigned int maxlife = D2COMMON_GetUnitStat(D2Client_pPlayUnit, STAT_MAXLIFE, 0);
	maxlife >>= 8;

	BOOL isInTown = TestPlayerInTown(D2Client_pPlayUnit);
	BOOL bNeedBlood = FALSE, bNeedMana = FALSE, bNeedZiPing = FALSE;
	double percent = ((double)life) / maxlife;

	if ((percent < 0.6) && bNeedHeal)
		*bNeedHeal = TRUE;

	if (percent < 0.3)
	{
		if (!isInTown)
		{
			if (g_hRunThreadHandle)
				SuspendThread(g_hRunThreadHandle);

			CloseGame();
			SendHackMessage(D2RPY_LOG, "血量太低，退出游戏！");
			if (g_hRunThreadHandle)
				TerminateThread(g_hRunThreadHandle, -1);
		}
	}
	else if (percent < 0.5)
	{
		if (!isInTown)
			bNeedZiPing = TRUE;		//喝紫瓶
	}
	else if (percent < 0.8)
	{
		if (!isInTown)			
			bNeedBlood = TRUE;		//喝血瓶
	}

	unsigned int mana = D2COMMON_GetUnitStat(D2Client_pPlayUnit, STAT_MANA, 0);
	mana >>= 8; // actual mana
	unsigned int maxmana = D2COMMON_GetUnitStat(D2Client_pPlayUnit, STAT_MAXMANA, 0);
	maxmana >>= 8;

	percent = ((double)mana) / maxmana;
	if ((percent < 0.5) && bNeedHeal)
		*bNeedHeal = TRUE;
	if (percent < 0.3)
	{
		if (!isInTown)
			bNeedMana = TRUE;
	}

	if (bNeedBlood || bNeedMana || bNeedZiPing)
		UsePotion(bNeedBlood, bNeedMana, bNeedZiPing, 1);
	
	//检查佣兵
	if (g_Game_Config.Use_Merc && g_Game_Data.MercGUID)
	{
		UnitAny *pMerc = D2CLIENT_GetUnitFromId(g_Game_Data.MercGUID, UNITNO_MONSTER);
		life = D2COMMON_GetUnitStat(pMerc, STAT_LIFE, 0);

		percent = ((double)life) / 0x8000;	//佣兵最大life 0x8000

		if ((percent < 0.5) && bNeedHeal)
			*bNeedHeal = TRUE;

		if (percent < 0.3)
		{
			if (!isInTown)
				UsePotion(0, 0, 1, 0);		//喝紫瓶
		}
		else if (percent < 0.5)
		{
			if (!isInTown)
				UsePotion(1, 0, 0, 0);		//喝血瓶
		}
	}
	return TRUE;
}

inline BOOL IsMonsterImmune(UnitAny* monster, UnitStat s)
{
	return (D2COMMON_GetUnitStat(monster, s, 0) >= 100);
}

inline BOOL IsMonsterKilled(DWORD MonsterID)
{
	UnitAny *monster = D2CLIENT_GetUnitFromId(MonsterID, UNITNO_MONSTER);
	return (D2COMMON_GetUnitStat(monster, STAT_LIFE, 0) == 0);
}

void CheckPlayerStatus(BOOL &bNeedBuyBlood, BOOL& bNeedBuyMana, BOOL& bHasUnidentifiedInventoryItems, BOOL& bNeedRepair, BOOL& bBuyTownScroll)
{
	short b = 0, m = 0;
	UpdateBeltPotionPos();
	for (int i = 3; i >= 0; --i)
	{
		if (!g_Game_Data.Potions[4 * i].PotionGUID)
			b++;
		if (!g_Game_Data.Potions[4 * i + 1].PotionGUID)
			m++;
	}
	bNeedBuyBlood = (b >= 2 ? TRUE : FALSE);
	bNeedBuyMana = (m >= 2 ? TRUE : FALSE);

	bHasUnidentifiedInventoryItems = FALSE;
	bNeedRepair = FALSE;
	bBuyTownScroll = FALSE;
	UnitAny* item;
	DWORD n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
	while (n)
	{
		item = D2COMMON_GetUnitFromItem(n);
		if (!item->pItemData->nLocation2 && !D2COMMON_GetItemFlag(item, ITEMFLAG_IDENTIFIED, 0, "?")
			&& (IsItemWanted(item) != PickupItemsConfig::Match))	//inventory 		
			bHasUnidentifiedInventoryItems = TRUE;
		
		if (!item->pItemData->nLocation2 && (GetItemCode(item->nTxtFileNo) + 1 == 2011))	//回城书
		{
			if (D2COMMON_GetUnitStat(item, STAT_QUANTITY, 0) <= 3)
				bBuyTownScroll = TRUE;
		}

		if ((item->pItemData->nLocation2 == 0xff) && item->pItemData->nLocation1 && !D2COMMON_GetItemFlag(item, ITEMFLAG_ETHEREAL, 0, "?")) //items on body
		{
			int nDurability = D2COMMON_GetUnitStat(item, STAT_DURABILITY, 0);
			int nMaxDurability = D2COMMON_GetUnitStat(item, STAT_MAX_DURABILITY, 0);
			if (nMaxDurability && ((double)nDurability / nMaxDurability < 0.2))
				bNeedRepair = TRUE;
		}
		n = D2COMMON_GetNextItemFromInventory(n);
	}
}

void TP2Coordinate(int destx, int desty, int HowNear, int max_dist_perstep, int maxtpnum, int randomstep)
{
	int retry = 0;
	double dx, dy;
	BYTE packet[5];

	srand((int)time(0));
	max_dist_perstep = max_dist_perstep + rand() % randomstep;
	//TP to room2
	dx = abs(destx - (int)(D2Client_pPlayUnit->pPos->xpos1 >> 16));
	dy = abs(desty - (int)(D2Client_pPlayUnit->pPos->ypos1 >> 16));
	SelectSkill(Player_Skill::TELEPORT);
	WaitToBeReadyToCastSkill(250);
	do
	{
		if (dx > max_dist_perstep)
			dx = max_dist_perstep;
		if (dy > max_dist_perstep)
			dy = max_dist_perstep;

		dx = destx > (int)((D2Client_pPlayUnit->pPos->xpos1 >> 16)) ? dx : -dx;
		dy = desty > (int)((D2Client_pPlayUnit->pPos->ypos1 >> 16)) ? dy : -dy;
		
		DWORD dw;
		if (D2CLIENT_ReadyToCastSkill(D2Client_pPlayUnit, &dw))
		{
			//right skill tp
			packet[0] = { 0xc };
			*(WORD*)&packet[1] = (WORD)((D2Client_pPlayUnit->pPos->xpos1 >> 16) + (int)dx);
			*(WORD*)&packet[3] = (WORD)((D2Client_pPlayUnit->pPos->ypos1 >> 16) + (int)dy);
			D2NET_SendPacket(5, 0, packet);

			WaitToBeReadyToCastSkill(250);
			Sleep(250);
			dx = abs(destx - (int)(D2Client_pPlayUnit->pPos->xpos1 >> 16));
			dy = abs(desty - (int)(D2Client_pPlayUnit->pPos->ypos1 >> 16));
		}
	} while (((dx > HowNear) || (dy > HowNear)) && (retry++ < maxtpnum));
}

DWORD LoopUnitNearby(const DrlgRoom1* room, std::function<DWORD(UnitAny*)> pCallback)
{
	const DrlgRoom1* pRoomSearched[32];
	DWORD Room1Num = 0;
	DWORD bRet = 0;

	if (!room)
		return 0;

	auto pFuncItemExist = [](const DWORD* pArray, DWORD ArrayNum, DWORD item) {
		for (DWORD i = 0; i < ArrayNum; ++i)
		{
			if (pArray[i] == item)
				return TRUE;
		}
		return FALSE;
	};

	auto pFuncLoopCurrentRoomUnit = [](const DrlgRoom1* room1, std::function<DWORD(UnitAny*)> pCallback) ->DWORD{
		UnitAny* item = room1->pUnitFirst;
		DWORD bRet = 0;
		while (item)
		{
			if (bRet = pCallback(item))
				return bRet;
			item = item->pNextUnit;
		}
		return 0;
	};

	if (!pFuncItemExist((const DWORD*)pRoomSearched, Room1Num, (DWORD)room))
	{
		if (bRet = pFuncLoopCurrentRoomUnit(room, pCallback))
			return bRet;
		pRoomSearched[Room1Num++] = room;
	}

	DrlgRoom1 *pRoom;
	for (unsigned int i = 0; i < room->nRoomsNear; ++i)
	{
		pRoom = room->paRoomsNear[i];
		if (!pFuncItemExist((const DWORD*)pRoomSearched, Room1Num, (DWORD)pRoom))
		{
			if (bRet = pFuncLoopCurrentRoomUnit(pRoom, pCallback))
				return bRet;
			pRoomSearched[Room1Num++] = pRoom;
		}
	}
	return 0;
}

//通过unit的pos坐标，在当前room1里查找unit的GUID
DWORD FindUnitGUIDFromPos(DrlgRoom1* room, const POINT& UnitPos)
{
	auto pFuncCheckUnit = [&](UnitAny* unit)->DWORD {
		POINT pt;

		if (unit->nUnitType == UNITNO_ITEM || unit->nUnitType == UNITNO_ROOMTILE || unit->nUnitType == UNITNO_OBJECT)
			pt = POINT{ (WORD)unit->pItemPath->xpos,(WORD)unit->pItemPath->ypos };
		else
			pt = POINT{ unit->pPos->xpos1 >> 16, unit->pPos->ypos1 >> 16 };

		if ((pt.x == UnitPos.x) && (pt.y == UnitPos.y))
			return unit->nUnitId;
		return 0;
	};

	return LoopUnitNearby(room, pFuncCheckUnit);
}

void GetItemName(UnitAny* item, char* Name)
{
	wchar_t wName[128] = { L'\0' };
	wchar_t *p;
	D2CLIENT_GetItemName(item, wName);
	while (p = wcschr(wName, L'\n'))
	{
		*p = L'-';
	}

	int w_nlen = WideCharToMultiByte(CP_ACP, 0, wName, -1, NULL, 0, NULL, false);
	WideCharToMultiByte(CP_ACP, 0, wName, -1, Name, w_nlen, NULL, false);
}

void SearchItemsWantedNearby(const DrlgRoom1* room)
{
	auto pFuncCheckItem = [&](UnitAny* item) -> DWORD {
		PickupItemsConfig::MatchResult MR;
		if ((item->nUnitType == UNITNO_ITEM) && item->pItemPath->ptRoom && (g_Game_Data.ItemToPickupNum < SIZEOFARRAY(g_Game_Data.ItemToPickup))
			&& !IsItemPicked(item->nUnitId))
		{
			MR = IsItemWanted(item);
#ifdef IMPK_BATTLENET
			//*********code for IMPK battle.net******************//
			//捡取所有的暗金精华物品，卖给NPC随机出现符文，For IMPK
			if (IsUniqueEliteItem(item) && (MR == PickupItemsConfig::NotMatch))
				MR = PickupItemsConfig::NeedIdentify;
			//*********end of code for IMPK battle.net******************//
#endif
			if ((MR == PickupItemsConfig::Match) || (MR == PickupItemsConfig::NeedIdentify))
			{
				int itemno = item->nTxtFileNo, quality = item->pItemData->nQuality;
				ItemTxt *itemtxt = D2COMMON_GetItemTxt(itemno);
				if ((itemtxt->dwCode == D2TXTCODE('rvs ')) || (itemtxt->dwCode == D2TXTCODE('rvl ')) ||
					(itemtxt->dwCode == D2TXTCODE('hp5 ')) || (itemtxt->dwCode == D2TXTCODE('mp5 ')) ||
					(itemtxt->dwCode == D2TXTCODE('hp4 ')) || (itemtxt->dwCode == D2TXTCODE('mp4 ')) ||
					(itemtxt->dwCode == D2TXTCODE('gld ')))
				{
					AddItemToPickList(item->nUnitId, (WORD)item->pItemPath->xpos, (WORD)item->pItemPath->ypos, itemtxt->dwCode);
				}
				else
				{
					AddItemToPickList(item->nUnitId, (WORD)item->pItemPath->xpos, (WORD)item->pItemPath->ypos, 0);
				}
			}
		}
		return 0;
	};

	ClearPickList();
	LoopUnitNearby(room, pFuncCheckItem);
}

BOOL inline IsLiveMonster(UnitAny* Monster)
{
	if ((Monster->nUnitType == UNITNO_MONSTER) && !IsMonsterKilled(Monster->nUnitId) &&
		(!GetPlayerMercUnit() || (Monster->nUnitId != GetPlayerMercUnit()->nUnitId)))
	{
		if ((Monster->nTxtFileNo == 351) || (Monster->nTxtFileNo == 352) || (Monster->nTxtFileNo == 353))//九头蛇，不是monster
			return FALSE;
		else
			return TRUE;
	}
	else
		return FALSE;
}

//5步之内是否有Monster
DWORD IsMonsterNearby()
{
	auto pFuncCheckMonster = [&](UnitAny* Monster) -> DWORD {
		if (IsLiveMonster(Monster) &&
			(abs(int((Monster->pPos->xpos1 >> 16) - (D2Client_pPlayUnit->pPos->xpos1 >> 16))) < 8) &&
			(abs(int((Monster->pPos->ypos1 >> 16) - (D2Client_pPlayUnit->pPos->ypos1 >> 16))) < 8))
			return Monster->nUnitId;		
		return 0;
	};
	return LoopUnitNearby(D2Client_pPlayUnit->pPos->pRoom1, pFuncCheckMonster);
}

void GetMonstersNearby(DWORD* MonsterGUIDArray, const DWORD NumOfArray, DWORD& NumOfMonster)
{
	auto pFuncCheckMonster = [&](UnitAny* Monster) -> DWORD {
		if (IsLiveMonster(Monster) && (NumOfMonster < NumOfArray))
			MonsterGUIDArray[NumOfMonster++] = Monster->nUnitId;
		return 0;
	};

	NumOfMonster = 0;
	LoopUnitNearby(D2Client_pPlayUnit->pPos->pRoom1, pFuncCheckMonster);
}

void GetCurrentRoom1Monsters(const DrlgRoom1* room1, DWORD* MonsterGUIDArray, const DWORD NumOfArray, DWORD& NumOfMonster)
{
	UnitAny* unit = room1->pUnitFirst;
	NumOfMonster = 0;
	while (unit)
	{
		if (IsLiveMonster(unit) && (NumOfMonster < NumOfArray))
			MonsterGUIDArray[NumOfMonster++] = unit->nUnitId;		
		unit = unit->pNextUnit;
	}
}

void GetCurrentRoom1UnhandledMonsters(const DrlgRoom1* room1, DWORD* MonsterGUIDArray, const DWORD NumOfArray, DWORD& NumOfMonster)
{
	UnitAny* unit = room1->pUnitFirst;
	NumOfMonster = 0;
	while (unit)
	{
		if (IsLiveMonster(unit) && 
			(!g_Game_Config.ExitWhenMonsterImm || !IsMonsterImmune(unit, (UnitStat)g_Game_Config.ExitWhenMonsterImm))
			&& (NumOfMonster < NumOfArray))
			MonsterGUIDArray[NumOfMonster++] = unit->nUnitId;
		unit = unit->pNextUnit;
	}
}

void ClearRoomMonsters(DrlgRoom1* Room, int DisFromMonster)
{
	short retry = 0;
	DWORD MonstersGUID[128], NumOfMonsters;
	GetCurrentRoom1UnhandledMonsters(Room, MonstersGUID, SIZEOFARRAY(MonstersGUID), NumOfMonsters);
	while (NumOfMonsters && (retry++ <= 4))
	{
		for (DWORD i = 0; i < NumOfMonsters; ++i)
			KillMonster(MonstersGUID[i], DisFromMonster, DisFromMonster, TRUE, 3, FALSE, FALSE);

		SearchItemsWantedNearby(D2Client_pPlayUnit->pPos->pRoom1);
		PickItem();
		Sleep(100);
		GetCurrentRoom1UnhandledMonsters(Room, MonstersGUID, SIZEOFARRAY(MonstersGUID), NumOfMonsters);
	}
}

//用于在room1里查找unit，适用于动态生成的unit，比如TP到附件才出现的各个Boss，Monster等，返回其GUID。FindUnitGUIDFromPos类似。
//与GetLevelPresetObj使用对象不同。
DWORD FindUnitGUIDPosFromTxtFileNo(DrlgRoom1* room, POINT& UnitPos, const DWORD nTxtFileNo, UnitNo unittype, BOOL bOnlyFindBoss, WORD UniqueMonsterID)
{
	auto pFuncCheckUnit = [&](UnitAny* unit)->DWORD {
		if ((unit->nTxtFileNo == nTxtFileNo) && (unit->nUnitType == unittype))
		{
			if ((!bOnlyFindBoss || ((unittype == UNITNO_MONSTER) && unit->pMonsterData->fBoss)) &&
				(!UniqueMonsterID || ((unittype == UNITNO_MONSTER) && unit->pMonsterData->uniqueno == UniqueMonsterID)))
			{
				if (unit->nUnitType == UNITNO_ITEM || unit->nUnitType == UNITNO_ROOMTILE || unit->nUnitType == UNITNO_OBJECT)
					UnitPos = POINT{ (WORD)unit->pItemPath->xpos,(WORD)unit->pItemPath->ypos };
				else
					UnitPos = POINT{ unit->pPos->xpos1 >> 16, unit->pPos->ypos1 >> 16 };
				return unit->nUnitId;
			}
		}
		return 0;
	};

	return LoopUnitNearby(room, pFuncCheckUnit);
}

//#define _D2HACK_PathTreeMemLeak_Deteck
#ifdef _D2HACK_PathTreeMemLeak_Deteck
extern int _D2Hack_PathTree_MemAllocatedSize;
extern int _D2Hack_PathTree_MemDeleteSize;
#endif

BOOL FindPathAndTP2DestRoom(const DrlgRoom2 *pRoomFrom, const DrlgRoom2 *pRoomTo)
{
	if (pRoomFrom == pRoomTo)
		return TRUE;

	g_Game_Data.bLevelToBeInitialized = TRUE;
	WAIT_UNTIL(50, 100, !g_Game_Data.bLevelToBeInitialized, TRUE);

	MonsterPathTree MonsterPathSource, MonsterPathDest;
	if (!GeneratePathToMonsterRoom(pRoomFrom, pRoomTo, MonsterPathSource, MonsterPathDest))
	{
		DeleteMonsterPathTree(&MonsterPathDest);
		return FALSE;		
	}

	//tp to lv entrance
	MonsterPathTree *path = MonsterPathSource.pPreNode;
	while (path)
	{
		TP2Coordinate(path->RoomPos.x * 5 + path->RoomPos.size.cx * 5 / 2, path->RoomPos.y * 5 + path->RoomPos.size.cy * 5 / 2, 20, MAX_DIST_FOR_SINGLE_TP, 4);
		path = path->pPreNode;
	}
	DeleteMonsterPathTree(&MonsterPathDest);
	return TRUE;
}

BOOL FindAndClearPathToDestRoom(const DrlgRoom2 *pRoomFrom, const DrlgRoom2 *pRoomTo)
{
	if (pRoomFrom == pRoomTo)
		return TRUE;

	InitCurrentLevelDrlgRoom();
	MonsterPathTree MonsterPathSource, MonsterPathDest;
	if (!GeneratePathToMonsterRoom(pRoomFrom, pRoomTo, MonsterPathSource, MonsterPathDest))
	{
		DeleteMonsterPathTree(&MonsterPathDest);
		return FALSE;
	}

	//tp to lv entrance
	MonsterPathTree *path = MonsterPathSource.pPreNode;
	while (path)
	{
		TP2Coordinate(path->RoomPos.x * 5 + path->RoomPos.size.cx * 5 / 2, path->RoomPos.y * 5 + path->RoomPos.size.cy * 5 / 2, 20, MAX_DIST_FOR_SINGLE_TP, 4);

		int dis;
		if (D2Client_pPlayUnit->nTxtFileNo == Player_Type_Paladin)
			dis = 1;
		else
			dis = 10;
		ClearRoomMonsters(D2Client_pPlayUnit->pPos->pRoom1, dis);
		SearchItemsWantedNearby(D2Client_pPlayUnit->pPos->pRoom1);
		PickItem();

		UpdateBeltPotionPos();
		if (!g_Game_Data.Potions[0].PotionGUID || !g_Game_Data.Potions[1].PotionGUID)
		{
			BackToTownToBuyPotions((WORD)GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo);
			DeleteMonsterPathTree(&MonsterPathDest);
			return FALSE;
		}
		else
			path = path->pPreNode;
	}
	DeleteMonsterPathTree(&MonsterPathDest);
	return TRUE;
}

BOOL GotoFindAndTakeEntrance(WORD ToLvl)
{
	DrlgRoom2 *pRoomTo;
	POINT pt;

	WORD currlvl = (WORD)GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo;
	if (!GetEntrancePosAndRoom2(currlvl, ToLvl, pt, pRoomTo))
	{
		Sleep(800);
		//try again
		if (!GetEntrancePosAndRoom2(currlvl, ToLvl, pt, pRoomTo))
		{
			SendHackMessage(D2RPY_LOG, "未找到目的位置...");
			return FALSE;
		}		
	}

	if (!FindPathAndTP2DestRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoomTo))
	{
		Sleep(800);
		//try again
		if (!FindPathAndTP2DestRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoomTo))
		{
			SendHackMessage(D2RPY_LOG, "未找到目的路径...");
			return FALSE;
		}
	}

#ifdef _D2HACK_PathTreeMemLeak_Deteck
	if (_D2Hack_PathTree_MemAllocatedSize != _D2Hack_PathTree_MemDeleteSize)
	{
		SendHackMessage(D2RPY_LOG, "PathTree代码存在内存泄漏！");
	}
#endif

	TP2Coordinate(pt.x, pt.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);

	DWORD EntranceID = FindUnitGUIDFromPos(D2Client_pPlayUnit->pPos->pRoom1, pt);
	//Take Entrance
	if (EntranceID)
	{
		BYTE packet[9];
		packet[0] = 0x13;
		*(DWORD*)&packet[1] = UNITNO_ROOMTILE;
		*(DWORD*)&packet[5] = EntranceID;
		D2NET_SendPacket(9, 0, packet);
		DrlgLevel *lv;

		WAIT_UNTIL(50, 50, GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo == ToLvl, FALSE);

		if ((lv = GetUnitDrlgLevel(D2Client_pPlayUnit)) && (lv->nLevelNo == ToLvl))
			return TRUE;
		else
			return FALSE;
	}
	return FALSE;
}

BOOL GotoActLevelFromWP(StartLocation& s, WORD *LvlArr, short nLvlNum)
{
	//使用此函数时，player需要在城里，或者在WP边
	if ((!TestPlayerInTown(D2Client_pPlayUnit) && s != From_WP) || !nLvlNum)
		return FALSE;

	if (nLvlNum == 1)
		return GoTakeWP(s, LvlArr[0]);

	short nCurLv = 0;
	//goto first lvl
	if (!GoTakeWP(s, LvlArr[nCurLv]))
		return FALSE;

	CastDefendSkill();
	s = From_Unknown;
	while (nCurLv < (nLvlNum - 1))
	{
		if (!GotoFindAndTakeEntrance(LvlArr[nCurLv + 1]))
			return FALSE;
		nCurLv++;
	}
	return TRUE;
}

#ifdef _DEBUG

BOOL PrintPresetUnit(DrlgRoom2* room2, DWORD UnitType)
{
	D2CLIENT_RevealAutomapRoom(room2->pRoom1, 1, D2CLIENT_pAutomapLayer);
	PresetUnit *unit = room2->pPresetUnits;
	DWORD levelno = room2->pDrlgLevel->nLevelNo;	
	while (unit) {
		DWORD objno = unit->nTxtFileNo;
		if((unit->nUnitType == UnitType) &&(objno < 574))
		{
			ObjectTxt *obj;
			obj = D2COMMON_GetObjectTxt(objno);
			DbgPrint("unit objno: %d, name: %s, x=0x%x, y=0x%x", objno, obj->szName, room2->xPos * 5 + unit->xPos, room2->yPos * 5 + unit->yPos);
		}
		unit = unit->pNext;
	}
	return FALSE;
}

void PrintCurrentLevelPresetUnit(DWORD UnitType)
{
	DrlgLevel *pDrlgLevel = D2Client_pPlayUnit->pPos->pRoom1->pRoom2->pDrlgLevel;
	if (!pDrlgLevel->pRoom2First)
		D2COMMON_InitDrlgLevel(pDrlgLevel);

	DrlgRoom2 *curroom = pDrlgLevel->pRoom2First;
	while (curroom)
	{
		if (curroom->pRoom1) {
			PrintPresetUnit(curroom, UnitType);
		}
		else {
			BYTE cmdbuf[6];
			*(WORD *)(cmdbuf + 1) = (WORD)curroom->xPos;
			*(WORD *)(cmdbuf + 3) = (WORD)curroom->yPos;
			cmdbuf[5] = (BYTE)curroom->pDrlgLevel->nLevelNo;
			D2CLIENT_RecvCommand07(cmdbuf);
			PrintPresetUnit(curroom, UnitType);
			D2CLIENT_RecvCommand08(cmdbuf);			
		}
		curroom = curroom->pNext;
	}
}
void PrintCurrentActLevels()
{
	DrlgLevel* p = D2CLIENT_pDrlgAct->pDrlgMisc->pLevelFirst;
	
	while (p)
	{
		DbgPrint(L"Level NO:%d, Level name:%s", p->nLevelNo, D2COMMON_GetLevelTxt(p->nLevelNo)->szName);
		p = p->pLevelNext;
	}
}
void PrintCurrentRoomMonsters(DrlgRoom1* room)
{
	UnitAny* unit = room->pUnitFirst;
	while (unit)
	{
		if (unit->nUnitType == UNITNO_MONSTER)
		{			
			DbgPrint(L"Monster TxtNO:%d, Name:%s", unit->nTxtFileNo, D2CLIENT_GetMonsterName(unit));
		}
		unit = unit->pNextUnit;
	}
}
void PrintMonstersAround()
{
	PrintCurrentRoomMonsters(D2Client_pPlayUnit->pPos->pRoom1);
	DrlgRoom1 *pRoom;
	for (unsigned int i = 0; i < D2Client_pPlayUnit->pPos->pRoom1->nRoomsNear; ++i)
	{
		pRoom = D2Client_pPlayUnit->pPos->pRoom1->paRoomsNear[i];
		PrintCurrentRoomMonsters(pRoom);
	}
}
#endif
void PickItem()
{
	BYTE packet[13];

	//先捡物品
	for (int i = 0; i < g_Game_Data.ItemToPickupNum; ++i)
	{
		if (!g_Game_Data.ItemToPickup[i].type && g_Game_Data.ItemToPickup[i].id)
		{
			if (!TestPlayerInTown(D2Client_pPlayUnit) && g_Game_Data.ItemToPickup[i].x)
			{
				TP2Coordinate(g_Game_Data.ItemToPickup[i].x, (WORD)g_Game_Data.ItemToPickup[i].y, 2, MAX_DIST_FOR_SINGLE_TP, 3);
			}			
			Sleep(50);

			//捡起地上Item
			packet[0] = { 0x16 };
			*(DWORD*)&packet[1] = 4;
			*(DWORD*)&packet[5] = g_Game_Data.ItemToPickup[i].id;
			*(DWORD*)&packet[9] = 0;
			D2NET_SendPacket(13, 0, packet);

			UnitAny *item = NULL;
			Sleep(300);
			WAIT_UNTIL(50, 4, (item = D2CLIENT_GetUnitFromId(g_Game_Data.ItemToPickup[i].id, UNITNO_ITEM))
				&& !item->pItemPath->ptRoom, FALSE);

			//if (!item->pItemPath->ptRoom && !item->pItemData->nLocation2)	//物品已捡取到身上
			{
				D2HackReply::ItemInfo replyInfo;
				int quality = item->pItemData->nQuality;
				replyInfo.Quality = quality;
				char str[256];
				memset(str, 0, sizeof(str));
				GetItemName(item, str);
				strcpy_s(replyInfo.ItemName, sizeof(replyInfo.ItemName), str);

				PickupItemsConfig::MatchResult MR = IsItemWanted(item);
				if (MR == PickupItemsConfig::Match)
				{
					strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "获取物品，[ %s ]。");
					SendHackMessage(D2RPY_OBTAIN_ITEM, replyInfo);
				}
				else if (MR == PickupItemsConfig::NeedIdentify)
				{
					strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "发现物品：[%s]，有待鉴别。");
					SendHackMessage(D2RPY_FOUND_ITEM, replyInfo);
				}
				g_Game_Data.ItemToPickup[i].id = 0;
			}			
		}
	}
	for (int i = 0; i < g_Game_Data.ItemToPickupNum; ++i)
	{
		if (!g_Game_Data.ItemToPickup[i].type && g_Game_Data.ItemToPickup[i].id)
		{
			if (!TestPlayerInTown(D2Client_pPlayUnit) && g_Game_Data.ItemToPickup[i].x)
			{
				TP2Coordinate(g_Game_Data.ItemToPickup[i].x, (WORD)g_Game_Data.ItemToPickup[i].y, 2, MAX_DIST_FOR_SINGLE_TP, 3);
			}			
			Sleep(250);

			//捡起地上Item
			packet[0] = 0x16;
			*(DWORD*)&packet[1] = UNITNO_ITEM;
			*(DWORD*)&packet[5] = g_Game_Data.ItemToPickup[i].id;
			*(DWORD*)&packet[9] = 0;
			D2NET_SendPacket(13, 0, packet);
			g_Game_Data.ItemToPickup[i].id = 0;
			Sleep(200);
		}
		else if (g_Game_Data.ItemToPickup[i].type && g_Game_Data.ItemToPickup[i].id )
		{
			DWORD dwCode = g_Game_Data.ItemToPickup[i].type;
			short col, pos = -1;
			if (dwCode == D2TXTCODE('gld '))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && g_Game_Data.ItemToPickup[i].x)
				{
					TP2Coordinate(g_Game_Data.ItemToPickup[i].x, (WORD)g_Game_Data.ItemToPickup[i].y, 5, MAX_DIST_FOR_SINGLE_TP, 3);
				}
				Sleep(200);

				packet[0] = 0x16;
				*(DWORD*)&packet[1] = UNITNO_ITEM;
				*(DWORD*)&packet[5] = g_Game_Data.ItemToPickup[i].id;
				*(DWORD*)&packet[9] = 0;
				D2NET_SendPacket(13, 0, packet);
				Sleep(100);
				continue;
			}
			UpdateBeltPotionPos();
			//捡紫瓶
			if ((dwCode == D2TXTCODE('rvs ')) || (dwCode == D2TXTCODE('rvl ')))
			{
				//紫瓶
				if (!g_Game_Data.Potions[14].PotionGUID)
					col = 2;
				else if (!g_Game_Data.Potions[15].PotionGUID)
					col = 3;
				else
					col = -1;
			}
			else if ((dwCode == D2TXTCODE('hp5 ')) || (dwCode == D2TXTCODE('hp4 ')))	//血瓶
				col = 0;
			else if ((dwCode == D2TXTCODE('mp5 ')) || (dwCode == D2TXTCODE('mp4 ')))
				col = 1;
			if (col != -1)
			{
				for (int i = 0; i <= 2; ++i)
					if (g_Game_Data.Potions[col + 4 * i].PotionGUID)
					{
						if (!g_Game_Data.Potions[col + 4 * i + 4].PotionGUID)
						{
							pos = col + 4 * i + 4;
							break;
						}
					}
					else
					{
						pos = col + 4 * i;
						break;
					}
			}

			if (pos != -1)
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && g_Game_Data.ItemToPickup[i].x)
				{
					TP2Coordinate(g_Game_Data.ItemToPickup[i].x, (WORD)g_Game_Data.ItemToPickup[i].y, 5, MAX_DIST_FOR_SINGLE_TP, 3);
				}
				Sleep(200);
								
				//捡起地上Item
				packet[0] = 0x16;
				*(DWORD*)&packet[1] = UNITNO_ITEM;
				*(DWORD*)&packet[5] = g_Game_Data.ItemToPickup[i].id;
				*(DWORD*)&packet[9] = 1;
				D2NET_SendPacket(13, 0, packet);

				UnitAny *pPotion;
				//wait until药水拿到鼠标上
				WAIT_UNTIL(80, 20, ((pPotion = D2CLIENT_GetUnitFromId(g_Game_Data.ItemToPickup[i].id, UNITNO_ITEM)) &&\
					(pPotion->eMode == 4)), FALSE);

				if ((pPotion = D2CLIENT_GetUnitFromId(g_Game_Data.ItemToPickup[i].id, UNITNO_ITEM)) && 
					(pPotion->eMode == 4))
				{
					packet[0] = 0x23;
					*(DWORD*)&packet[1] = g_Game_Data.ItemToPickup[i].id;
					*(DWORD*)&packet[5] = pos;
					D2NET_SendPacket(9, 0, packet);

					//Wait Until药水装备到腰带
					WAIT_UNTIL(80, 20, ((pPotion = D2CLIENT_GetUnitFromId(g_Game_Data.ItemToPickup[i].id, UNITNO_ITEM)) && \
						(pPotion->eMode == 2)), FALSE);

					if ((pPotion = D2CLIENT_GetUnitFromId(g_Game_Data.ItemToPickup[i].id, UNITNO_ITEM)) &&
						(pPotion->eMode == 4))	//药水还在鼠标上
					{
						packet[0] = 0x17;
						*(DWORD*)&packet[1] = g_Game_Data.ItemToPickup[i].id;
						D2NET_SendPacket(5, 0, packet);
						Sleep(100);
					}
				}
			}
		}
	}
}

void GotoPosRelative(HWND h, int x, int y)
{
	int PlayerX = D2Client_pPlayUnit->pPos->xpos2 - D2CLIENT_GetPlayerXOffset();
	int PlayerY = D2Client_pPlayUnit->pPos->ypos2 - D2CLIENT_GetPlayerYOffset();

	x = PlayerX + x, y = PlayerY + y;
	if (x < 0)	x = 0;
	if (x > RESOLUTIONX)	x = RESOLUTIONX - 10;
	if (y < 0)	y = 0;
	if (y > 540)	y = 540;

	LButtonClick(h, x, y);
}

BOOL GotoCoordinate(int DestX, int DestY, short MaxStepCount, BOOL bBackToTown)
{
	const int ratio = 10;
	srand((int)time(0));
	int dlta = 200 + rand() % 50;//单次移动的像素值，随机值避免每次走路坐标都一样
	int delay = 400 + rand() % 50;
	char StepNum = 0;

	int dX = (ratio*int(DestX - (D2Client_pPlayUnit->pPos->xpos1 >> 16)));
	int dY = (ratio*int(DestY - (D2Client_pPlayUnit->pPos->ypos1 >> 16)));
	int stepX, stepY, transXPos, transYPos;
	DWORD NearestX = -1, NearestY = -1;
	char stepNearest = 0;
	int StuckCount = 0;

	HWND hD2win = D2GFX_GetHwnd();
	while ((abs(dX) > 50) || (abs(dY) >50))
	{
		dX = (int)(ratio*int(DestX - (D2Client_pPlayUnit->pPos->xpos1 >> 16)));
		dY = (int)(ratio*int(DestY - (D2Client_pPlayUnit->pPos->ypos1 >> 16)));
		//DbgPrint("WP相对坐标:%d,%d", (int)dX, (int)dY);

		if (((DWORD)abs(dX) < NearestX) || ((DWORD)abs(dY) < NearestY))
			stepNearest = StepNum;
		NearestX = min(NearestX, (DWORD)abs(dX));
		NearestY = min(NearestY, (DWORD)abs(dY));
		if ((abs(dX) < 100) && (abs(dY) < 100))
		{
			dlta = 50;
			delay = 100;
		}
		stepX = (abs(dX) < dlta) ? (int)abs(dX) : dlta;
		stepY = (abs(dY) < dlta) ? (int)abs(dY) : dlta;
		stepX = (dX < 0) ? -stepX : stepX;
		stepY = (dY < 0) ? -stepY : stepY;

		transXPos = int(0.71*(stepX - stepY));
		transYPos = int(0.71*(stepX + stepY));
		if (((DWORD)abs(dX) >= NearestX) && ((DWORD)abs(dY) >= NearestY) && (StepNum - stepNearest)>3)
		{
			DbgPrint("Player卡住了，调整位置！");
			stepNearest = StepNum;
			if (StuckCount++ % 2)
			{
				transXPos = int(transXPos * 0.4);
				transYPos = int(transYPos * 0.4);
				/*dX = (int)(ratio*int(DestX - (D2Client_pPlayUnit->pPos->xpos1 >> 16)));
				dY = (int)(ratio*int(DestY - (D2Client_pPlayUnit->pPos->ypos1 >> 16)));
				GotoCoordinate((D2Client_pPlayUnit->pPos->xpos1 >> 16) + dY, (D2Client_pPlayUnit->pPos->ypos1 >> 16) - dX, 3);
				stepNearest = StepNum;
				NearestX = -1, NearestY = -1;*/
			}
			else
			{
				transXPos = transXPos * 2;
				transYPos = transYPos * 2;
				delay = delay + delay / 2;
				/*dX = (int)(ratio*int(DestX - (D2Client_pPlayUnit->pPos->xpos1 >> 16)));
				dY = (int)(ratio*int(DestY - (D2Client_pPlayUnit->pPos->ypos1 >> 16)));
				GotoCoordinate((D2Client_pPlayUnit->pPos->xpos1 >> 16) - dY, (D2Client_pPlayUnit->pPos->ypos1 >> 16) + dX, 3);
				stepNearest = StepNum;
				NearestX = -1, NearestY = -1;*/
			}
		}
		GotoPosRelative(hD2win, transXPos, transYPos);
		Sleep(delay);

		//如果点到了传送门，回城继续
		if (bBackToTown && !TestPlayerInTown(D2Client_pPlayUnit))
		{
			StartLocation s;
			BackToTown(s);
		}

		if (++StepNum >= MaxStepCount)
			return FALSE;
	}

	//wait for player to stop runing
	//WAIT_UNTIL(100, 10, ((D2Client_pPlayUnit->pPos->xpos1 & 0xffff) == 0x8000) && ((D2Client_pPlayUnit->pPos->ypos1 & 0xffff) == 0x8000), FALSE);
	return TRUE;
}

void InteractObject(DWORD nTxtFileNO)
{
	POINT pt;
	DWORD id = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, nTxtFileNO, UNITNO_OBJECT);
	if (id)
	{
		BYTE castOpen[9] = { 0x13 };
		*(DWORD*)&castOpen[1] = UNITNO_OBJECT;
		*(DWORD*)&castOpen[5] = id; // Door ID
		D2NET_SendPacket(9, 0, castOpen);
		Sleep(300);
	}
}

void BackToTown(StartLocation& s)
{
	if (TestPlayerInTown(D2Client_pPlayUnit)) return;

	short retry = 0;
	g_Game_Data.bBackToTown = 1;
	BYTE castTP1[9] = { 0x3C };
	*(DWORD*)&castTP1[1] = 0xDC;
	*(DWORD*)&castTP1[5] = 0xFFFFFFFF;
	D2NET_SendPacket(sizeof(castTP1), 0, castTP1);
	Sleep(200);

	BYTE castTP2[5] = { 0x0C };
	*(WORD*)&castTP2[1] = D2Client_pPlayUnit->pPos->xpos1 >> 16;
	*(WORD*)&castTP2[3] = D2Client_pPlayUnit->pPos->ypos1 >> 16;
	D2NET_SendPacket(sizeof(castTP2), 0, castTP2);	
	Sleep(1000);

	while (!TestPlayerInTown(D2Client_pPlayUnit) && (retry++ < 3))
		Sleep(1000);

	if (retry >= 3)
		KillMe();

	s = From_Door;
}

void BackToTownToBuyPotions(WORD nFromLevel)
{
	StartLocation s;
	BackToTown(s);
	GotoIdentifyAndBuyPotions(s);

	//back to fight
	GotoDoor(s);
	POINT pt;
	DWORD id = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x3b, UNITNO_OBJECT);	//find my door

	if (id)
	{
		BYTE castMove[9] = { 0x13 };
		*(DWORD*)&castMove[1] = 2;
		*(DWORD*)&castMove[5] = id; // Door ID
		D2NET_SendPacket(9, 0, castMove);
		Sleep(300);
	}
	WAIT_UNTIL(300, 10, GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo == nFromLevel, TRUE);
}

static void ShowCredit()
{
	if (!g_Game_Data.bBOT_TitleDisplay)
	{
		static int count = 0;
		static clock_t t = 0;
		if (!t)
			t = clock();
		switch (count)
		{
		case 0:
			if (clock() - t >= 2500)
			{
				D2CLIENT_PrintGameStringAtTopLeft(L"【Leon's HackBOT】: Leon's HackBot for DiabloII 1.11B Installed.", 8);
				t = clock();
				count++;
			}
			break;
		case 1:
			if (clock() - t >= 2500)
			{
				D2CLIENT_PrintGameStringAtTopLeft(L"【Leon's HackBOT】: 任何意见和建议可以与我联系（hust_liuy@163.com）.", 8);
				t = clock();
				count++;
			}
			break;
		case 2:
			if (clock() - t >= 2500)
			{
				D2CLIENT_PrintGameStringAtTopLeft(L"【Leon's HackBOT】: 使用BOT工具在某些战网可能会导致账号被封，使用此程序风险自负！", 8);
				D2CLIENT_PrintGameStringAtTopLeft(L"【Leon's HackBOT】: WARNING: Using BOT tools may cause your account BANED!!Use it at your own risk!", 8);
				t = clock();
				count++;
			}
			break;
		case 3:
			if (clock() - t >= 2500)
			{
				D2CLIENT_PrintGameStringAtTopLeft(L"【Leon's HackBOT】: 注意，此工具不可用于商业用途！ Just enjoy it!!!!", 8);
				t = 0;
				count = 0;
				g_Game_Data.bBOT_TitleDisplay = TRUE;
			}
			break;
		}
	}
}

void GameLoopPatch()
{
	ShowCredit();
	CheckLifeAndMana(NULL);

	if (g_Game_Data.bLevelToBeInitialized)
	{
		InitCurrentLevelDrlgRoom();
		g_Game_Data.bLevelToBeInitialized = FALSE;
	}

	if (g_Game_Data.bCloseGameFlag)
	{
		if ((GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit)) >= 1))
		{
			D2CLIENT_fExitAppFlag = 0;
			SendMessage(D2GFX_GetHwnd(), WM_CLOSE, 0, 0);
		}
	}
}

void __declspec(naked) GameLoopPatch_ASM()
{
	__asm {
		pop eax
		sub esp, 0x20
		mov[esp + 0x0c], ecx
		push eax
		jmp GameLoopPatch
	}
}

void GameEndPatch()
{
	ResetHackGlobalObj();
}

void __declspec(naked) GameEndPatch_ASM()
{
	__asm {
		call GameEndPatch
		//original code
		mov ecx, 0xb4
		ret
	}
}

D2_Game_Location WhereAmI( HWND hD2win)
{
	HDC dc = GetDC(hD2win);
	COLORREF  col = GetPixel(dc, 70, 51), col2;
	
	D2_Game_Location l;
	switch (col)
	{
	case 526344:
		col = GetPixel(dc, 464, 343);
		if (col == 263172)
			l = D2_LOC_ACC_INPUT;
		else
			l = D2_LOC_LOGIN;
		break;
	case 263172:
		l = D2_LOC_CHAR_SEL;
		break;
	case 2903140:
		col = GetPixel(dc, 553, 369);
		col2 = GetPixel(dc, 414, 267);
		if ((col == 2105376 || col == 263172) && (col2 == 263172))
			l = D2_LOC_LOBBY;
		else
			l = D2_LOC_UNKNOWN;
		break;
	case 0:
		l = D2_LOC_PENDING;
		break;
	default:
		l = D2_LOC_UNKNOWN;
	}
	ReleaseDC(hD2win, dc);

	DbgPrint("目前位置在：%d", l);

	return l;
}

BOOL Login()
{
	HWND hD2win;

	WAIT_UNTIL(500, 10, (WhereAmI(hD2win = D2GFX_GetHwnd()) == D2_LOC_LOGIN)|| (WhereAmI(hD2win) == D2_LOC_ACC_INPUT), FALSE);
	
	if ((WhereAmI(hD2win) != D2_LOC_LOGIN) && (WhereAmI(hD2win) != D2_LOC_ACC_INPUT))
		return FALSE;

	LButtonClick(hD2win, 400, 351);

	WAIT_UNTIL(500, 8, WhereAmI(hD2win) == D2_LOC_ACC_INPUT, FALSE);

	if (WhereAmI(hD2win) != D2_LOC_ACC_INPUT)
		return FALSE;

	LButtonClick(hD2win, 466, 333);
	SendVKey(hD2win, VK_BACK, 15);
	
	SendString(hD2win, g_Game_Config.Acc_Name);

	LButtonClick(hD2win, 469, 387);
	SendString(hD2win, g_Game_Config.Acc_Pass);
	LButtonClick(hD2win, 398, 467);

	WAIT_UNTIL(500, 8, WhereAmI(hD2win) == D2_LOC_CHAR_SEL, FALSE);

	if (WhereAmI(hD2win) != D2_LOC_CHAR_SEL)
		return FALSE;

	Sleep(1000);
	if ((g_Game_Config.Acc_Pos % 2) == 0)
		SendVKey(hD2win, VK_RIGHT);

	SendVKey(hD2win, VK_DOWN, (g_Game_Config.Acc_Pos - 1) / 2);
	SendVKey(hD2win, VK_RETURN);

	WAIT_UNTIL(500, 8, WhereAmI(hD2win) == D2_LOC_LOBBY, FALSE);
	if (WhereAmI(hD2win) != D2_LOC_LOBBY)
		return FALSE;

	return TRUE;
}

BOOL CreateGame(const char* sGameName, int Difficulty)
{
	HWND hD2win = D2GFX_GetHwnd();
	short retry = 0;
	int x;
	while ((GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit)) <= 0) && (retry++ < 7))
	{
		LButtonClick(hD2win, 591, 459);
		SendVKey(hD2win, VK_BACK, 15);
		SendString(hD2win, sGameName);
		switch (Difficulty)
		{
		case 1:
			x = 438; break;
		case 2:
			x = 562; break;
		case 3:
			x = 706; break;
		}
		LButtonClick(hD2win, x, 375);
		LButtonClick(hD2win, 676, 418);
		Sleep(2500);
	}

	if (GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit)) <= 0)
		return FALSE;

	SendHackMessage(D2RPY_SUCC_CREATEGAME, "成功创建游戏 %s.", sGameName);
	return TRUE;
}

inline void CloseGame()
{
	g_Game_Data.bCloseGameFlag = TRUE;
}

void UpdateNPCPos(DWORD NPCGUID, POINT& pt)
{
	UnitAny *pNPC = D2CLIENT_GetUnitFromId(NPCGUID, UNITNO_MONSTER);
	if (pNPC)
	{
		pt.x = pNPC->pPos->xpos1 >> 16;
		pt.y = pNPC->pPos->ypos1 >> 16;
	}
}

inline DWORD GetNPCID_Pos(DWORD NPC_TxtNO, POINT& pt)
{
	return FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, NPC_TxtNO, UNITNO_MONSTER);
}

DWORD GetHealNPCID_Pos(POINT& pt)
{
	DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
	switch (curact)
	{
	case 1:
		return GetNPCID_Pos(0x94, pt);
	case 2:		
		return GetNPCID_Pos(0xb2, pt);
	case 3:
		return GetNPCID_Pos(255, pt);
	case 4:
		return GetNPCID_Pos(405, pt);
	case 5:
		return GetNPCID_Pos(0x201, pt);
	}
	return 0;
}

DWORD GetTradeNPCID_Pos(POINT& pt)
{
	DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
	switch (curact)
	{	
	case 2:
		return GetNPCID_Pos(0xb1, pt);
	case 1:
	case 3:
	case 4:
	case 5:
		return GetHealNPCID_Pos(pt);
	}
	return 0;
}

BOOL GoTakeWP(StartLocation& s, DWORD DestWP)
{
	if (GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo == DestWP)
		return TRUE;

	DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
	GotoWaypoint(s);
	POINT pt;
	
	//find waypoint in current level
	DWORD WayPointID = 0;
	if (GetLevelPresetObj((WORD)GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo, pt, NULL, "Waypoint", 0))
	{
		WayPointID = FindUnitGUIDFromPos(D2Client_pPlayUnit->pPos->pRoom1, pt);
	}
	if (WayPointID)
	{
		TakeWP(WayPointID, DestWP, DestWP);
		return TRUE;
	}
	else
		return FALSE;
}

void GotoWaypoint(StartLocation& s)
{
	if (s == From_WP)
		return;
	DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));

	POINT WPPos;
	GetLevelPresetObj(ActLevels[curact - 1], WPPos, NULL, "Waypoint", 0);

	switch (curact)
	{
	case 1:
		switch (s)
		{
		case From_StartPoint:
			GotoCoordinate(D2Client_pPlayUnit->pPos->xpos1 >> 16, (D2Client_pPlayUnit->pPos->ypos1 >> 16) - 9);
			break;
		case From_TradeNPC:
		case From_HealerNPC:
			GotoCoordinate((D2Client_pPlayUnit->pPos->xpos1 >> 16) - 23, (D2Client_pPlayUnit->pPos->ypos1 >> 16) + 9);
			break;
		}
		break;
	case 2:
		switch (s)
		{
		case From_TradeNPC:
			GotoCoordinate(0x13e5, 0x13e6);
			break;
		case From_HealerNPC:
			GotoCoordinate(0x13f7, 0x13e9);
			GotoCoordinate(0x13de, 0x13e9);
			break;
		case From_StartPoint:
			GotoCoordinate(0x1416, 0x1440);
			GotoCoordinate(0x1407, 0x143d);
			GotoCoordinate(0x1407, 0x13f4);
			GotoCoordinate(0x13eb, 0x13e2);
			break;
		case From_Door:
			GotoCoordinate(0x13e6, 0x13c3);
			GotoCoordinate(0x13e8, 0x13e1);
			break;
		}
		break;
	case 3:
		switch (s)
		{
		case From_StartPoint:
			GotoCoordinate(0x140c, 0x142f);
			GotoCoordinate(0x140c, 0x13e5);
			GotoCoordinate(0x141c, 0x13e4);
			GotoCoordinate(0x141c, 0x13c5);
			break;
		case From_TradeNPC:
		case From_HealerNPC:
			GotoCoordinate(0x141c, 0x13e4);
			GotoCoordinate(0x141c, 0x13c5);
			break;
		}
		break;
	case 4:
		switch (s)
		{
		case From_TradeNPC:
		case From_HealerNPC:
			GotoCoordinate(0x13d5, 0x13b6);
			GotoCoordinate(0x13b7, 0x13ae);
			break;
		}
		break;
	case 5:
		switch (s)
		{
		case From_StartPoint:
			GotoCoordinate(0x13f3, 0x13c0);
			break;
		case From_TradeNPC:
		case From_HealerNPC:
			GotoCoordinate(0x13dd, 0x13aa);
			GotoCoordinate(0x13ea, 0x139c);
			GotoCoordinate(0x13f3, 0x13c0);
			break;
		}
		break;
	}

	if (!GotoCoordinate(WPPos.x, WPPos.y))
		KillMe();

	s = From_WP;
}

void CloseMenu(UIVar ui)
{
	
	LPARAM lparm = MAKELPARAM(800, 600);
	SendMessage(D2GFX_GetHwnd(), WM_MOUSEMOVE, 0, lparm);
	D2CLIENT_SetUiVar(ui, 1, 1);
	/*SendVKey(D2GFX_GetHwnd(), VK_ESCAPE);

	if(D2CLIENT_GetUiVar(UIVAR_GAMEMENU))
		SendVKey(D2GFX_GetHwnd(), VK_ESCAPE);*/
}

BOOL GotoRepairItems(StartLocation& s)
{
	// goto act2 to repair 
	GoTakeWP(s, 0x28);
	if (!GotoHealNPC(s))
		return FALSE;

	POINT NPCPos;
	DWORD ID = GetHealNPCID_Pos(NPCPos);

	BYTE packet[17];
	packet[0] = 0x13;
	*(DWORD*)&packet[1] = UNITNO_MONSTER;
	*(DWORD*)&packet[5] = ID;
	D2NET_SendPacket(9, 0, packet);
	WAIT_UNTIL(300, 10, D2CLIENT_GetUiVar(UIVAR_INTERACT), FALSE);

	packet[0] = 0x38;
	*(DWORD*)&packet[1] = UNITNO_MONSTER;
	*(DWORD*)&packet[5] = ID;
	*(DWORD*)&packet[9] = 0;
	D2NET_SendPacket(13, 0, packet);
	Sleep(200);

	packet[0] = 0x35;
	*(DWORD*)&packet[1] = ID;
	*(DWORD*)&packet[5] = 0;
	*(DWORD*)&packet[9] = 0;
	*(DWORD*)&packet[13] = 0x80000000;
	D2NET_SendPacket(17, 0, packet);
	Sleep(100);

	//close NPC menu
	if (D2CLIENT_GetUiVar(UIVAR_INTERACT))
	{
		CloseMenu(UIVAR_INTERACT);		
	}
	return TRUE;
}

BOOL GotoGetHeal(StartLocation& s)
{
	//goto healer pos
	if (!GotoHealNPC(s))
		return FALSE;

	POINT NPCPos;
	DWORD ID = GetHealNPCID_Pos(NPCPos);

	BYTE packet[17] = { 0x59 };
	/**(DWORD*)&packet[1] = 1;
	*(DWORD*)&packet[5] = ID;
	*(WORD*)&packet[9] = (D2Client_pPlayUnit->pPos->xpos1 >> 16);
	*(WORD*)&packet[11] = 0;
	*(WORD*)&packet[13] = (D2Client_pPlayUnit->pPos->ypos1 >> 16);
	*(WORD*)&packet[15] = 0;
	D2NET_SendPacket(17, 0, packet);
	Sleep(200);*/

	packet[0] = 0x13;
	*(DWORD*)&packet[1] = UNITNO_MONSTER;
	*(DWORD*)&packet[5] = ID;
	D2NET_SendPacket(9, 0, packet);

	//不要发送2f数据包
	/*packet[0] = 0x2f;
	*(DWORD*)&packet[1] = 1;
	*(DWORD*)&packet[5] = g_Game_ACT[curact - 1].NPCList.HealerNPC.NPC_UID;
	D2NET_SendPacket(9, 0, packet);*/
	WAIT_UNTIL(300, 10, D2CLIENT_GetUiVar(UIVAR_INTERACT), FALSE);

	//close NPC menu
	if (D2CLIENT_GetUiVar(UIVAR_INTERACT))
	{
		CloseMenu(UIVAR_INTERACT);
	}

	s = From_HealerNPC;
	return TRUE;
}

void UpdateBeltPotionPos()
{
	memset(g_Game_Data.Potions, 0, sizeof(g_Game_Data.Potions));
	DWORD n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
	while (n) 
	{
		UnitAny *pItem = D2COMMON_GetUnitFromItem(n);
		if ((pItem->pItemData->nLocation2 == 0xff) && !pItem->pItemData->nLocation1)	//腰带
		{
			g_Game_Data.Potions[pItem->pItemPath->xpos] = { pItem->nUnitId };
		}
		n = D2COMMON_GetNextItemFromInventory(n);
	}
}

BOOL GotoHealNPC(StartLocation& s)
{
	//goto healer pos
	DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
	switch (curact)
	{
	case 1:
	case 3:
	case 4:
	case 5:
		GotoTradeNPC(s);
		break;
	case 2:
		switch (s)
		{
		case From_StartPoint:
			GotoCoordinate(0x1416, 0x1440);
			GotoCoordinate(0x1407, 0x143d);
			GotoCoordinate(0x1407, 0x13f4);
			break;
		case From_WP:
			GotoCoordinate(0x13de, 0x13e9);
			GotoCoordinate(0x13f7, 0x13e9);
			break;
		case From_Door:
			GotoCoordinate(0x13e6, 0x13c3);
			GotoCoordinate(0x13e8, 0x13e1);
			GotoCoordinate(0x13f7, 0x13e9);
			break;
		}
		break;
	}

	POINT NPCPos;
	DWORD ID = GetHealNPCID_Pos(NPCPos);
	WAIT_UNTIL(400, 5, ID = GetHealNPCID_Pos(NPCPos), FALSE);

	if (!ID ||
		!GotoCoordinate(NPCPos.x, NPCPos.y))
		return FALSE;

	UpdateNPCPos(ID, NPCPos);
	if (!GotoCoordinate(NPCPos.x, NPCPos.y))
		return FALSE;
	s = From_HealerNPC;
	return TRUE;
}

BOOL GotoTradeNPC(StartLocation& s)
{
	//go to NPC pos
	DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
	switch (curact)
	{
	case 1:
		switch (s)
		{
		case From_StartPoint:
			GotoCoordinate((D2Client_pPlayUnit->pPos->xpos1 >> 16) + 25, (D2Client_pPlayUnit->pPos->ypos1 >> 16) - 5);
			GotoCoordinate((D2Client_pPlayUnit->pPos->xpos1 >> 16) + 5, (D2Client_pPlayUnit->pPos->ypos1 >> 16) - 5);
			break;
		case From_Door:	
			GotoCoordinate((D2Client_pPlayUnit->pPos->xpos1 >> 16) + 15, (D2Client_pPlayUnit->pPos->ypos1 >> 16) - 53);
			break;
		}
		break;
	case 2:
		switch (s)
		{
		case From_StartPoint:
			GotoCoordinate(0x1403, 0x1425);
			GotoCoordinate(0x13e8, 0x141e);
			GotoCoordinate(0x13e8, 0x13ae);
			break;
		case From_HealerNPC:
			GotoCoordinate(0x13fa, 0x13ea);
			GotoCoordinate(0x13e8, 0x13e7);
			GotoCoordinate(0x13e9, 0x13b7);
			break;
		case From_Door:
			GotoCoordinate(0x13e6, 0x13c3);
			break;
		case From_WP:
			GotoCoordinate(0x13e8, 0x13df);
			GotoCoordinate(0x13e8, 0x13ae);
			break;
		}
		break;
	case 3:
		switch (s)
		{
		case From_StartPoint:
			GotoCoordinate(0x140c, 0x142f);
			GotoCoordinate(0x140c, 0x13e5);
			break;
		case From_Door:
			GotoCoordinate(0x141c, 0x13e4);
			GotoCoordinate(0x140c, 0x13e5);
			break;
		}
		break;
	case 4:
		switch (s)
		{
		case From_StartPoint:
		case From_Door:
			GotoCoordinate(0x13dc, 0x13b7);
			break;
		}
		break;
	case 5:
		switch (s)
		{
		case From_Door:
		case From_StartPoint:
			GotoCoordinate(0x13e0, 0x13a8);
			GotoCoordinate(0x13d6, 0x13a8);
			break;
		case From_HealerNPC:
			break;
		}
		break;
	}

	POINT NPCPos;
	DWORD ID = GetTradeNPCID_Pos(NPCPos);
	WAIT_UNTIL(200, 15, ID = GetTradeNPCID_Pos(NPCPos), FALSE);

	//走到NPC 附近
	if (!GotoCoordinate(NPCPos.x, NPCPos.y))
		return FALSE;

	UpdateNPCPos(ID, NPCPos);
	//走到NPC 附近
	if (!GotoCoordinate(NPCPos.x, NPCPos.y))
		return FALSE;

	s = From_TradeNPC;
	return TRUE;
}

BOOL GotoDoor(StartLocation& s)
{
	if (s == From_TradeNPC)
	{
		DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
		switch (curact)
		{
		case 1:
			GotoCoordinate((D2Client_pPlayUnit->pPos->xpos1 >> 16) - 20, (D2Client_pPlayUnit->pPos->ypos1 >> 16) + 10);
			GotoCoordinate(D2Client_pPlayUnit->pPos->xpos1 >> 16, (D2Client_pPlayUnit->pPos->ypos1 >> 16) + 20);
			break;
		case 2:		
			GotoCoordinate(0x13e6, 0x13c3);
			GotoCoordinate(0x1428, 0x13c3);
			break;
		case 3:
			GotoCoordinate(0x140c, 0x13e5);
			GotoCoordinate(0x141c, 0x13e4);
			break;

		case 4:
			GotoCoordinate(0x13dc, 0x13b7);
			break;
		case 5:
			GotoCoordinate(0x13d6, 0x13a8);
			GotoCoordinate(0x13e0, 0x13a8);			
			break;
		}

		POINT pt;
		if (FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x3b, UNITNO_OBJECT))	//find diment door in town
		{
			GotoCoordinate(pt.x + 4 , pt.y - 4);
			s = From_Door;
			return TRUE;
		}		
	}

	return FALSE;
}

BOOL GotoBank(StartLocation& s)
{
	// goto act4 bank, should not be stucked
	if (GoTakeWP(s, 103))
	{
		POINT BankPos;//储物箱	
		DWORD bankID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BankPos, 267, UNITNO_OBJECT);
		if (!bankID)
			return FALSE;
		GotoCoordinate(BankPos.x, BankPos.y);
		s = From_StartPoint;
		return TRUE;
	}
	return FALSE;
}

static inline BOOL IsGambleItem(UnitAny* item)
{
	short code = GetItemCode(item->nTxtFileNo) + 1;
	for (int i = 0; i < g_Game_Config.GambleItemsNum; ++i)
	{
		if (code == g_Game_Config.GambleItems[i])
			return TRUE;
	}
	return FALSE;
}

void GotoDeositMoneyAndGamble(StartLocation& s)
{
	UnitAny *player = D2Client_pPlayUnit;
	int gold = D2COMMON_GetUnitStat(player, STAT_GOLD, 0);
	int level = D2COMMON_GetUnitStat(player, STAT_LEVEL, 0);

	if ((gold >= 8000 * level) && GotoBank(s))
	{
		InteractObject(267);
		WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_STASH), FALSE);

		if (D2CLIENT_GetUiVar(UIVAR_STASH))
		{
			BYTE packet[9] = { 0x13 };
			packet[0] = 0x4f;
			*(WORD*)&packet[1] = 0x14;	//deposit money ButtonID
			*(WORD*)&packet[3] = (gold & 0xffff0000) >> 16;
			*(WORD*)&packet[5] = gold & 0xffff;
			D2NET_SendPacket(7, 0, packet);
			Sleep(500);		

			CloseMenu(UIVAR_STASH);
			Sleep(100);
		}
	}

	gold = D2COMMON_GetUnitStat(player, STAT_GOLD, 0);
	DWORD goldInBank = D2COMMON_GetUnitStat(player, STAT_GOLD_IN_BANK, 0);
	if (g_Game_Config.MinimumMoneyToStartGamble && (goldInBank + gold >= g_Game_Config.MinimumMoneyToStartGamble))
	{
		//开始Gamble
		//去act5安雅Gamble
		GoTakeWP(s, 109);	//goto act5
		BOOL GotoRedDoor(StartLocation& s, int RelativeX, int RelativeY);
		GotoRedDoor(s, -5, 10);

		DWORD AnyaID;
		POINT AnyaPos;
		AnyaID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, AnyaPos, 512, UNITNO_MONSTER);
		if (AnyaID)
		{
			GotoCoordinate(AnyaPos.x, AnyaPos.y);

			UnitAny *UnitAnya = D2CLIENT_GetUnitFromId(AnyaID, UNITNO_MONSTER), *pItem;
			DWORD NPCTxtFileID = UnitAnya->nTxtFileNo;
			while (goldInBank > g_Game_Config.KeepMoneyAfterGamble)
			{
				BYTE packet[17];
				if (!D2CLIENT_GetUiVar(UIVAR_INTERACT))
				{
					packet[0] = 0x13;
					*(DWORD*)&packet[1] = UNITNO_MONSTER;
					*(DWORD*)&packet[5] = AnyaID;
					D2NET_SendPacket(9, 0, packet);
					WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_INTERACT), TRUE);
				}
				packet[0] = { 0x38 };
				*(DWORD*)&packet[1] = 2;	//action ID
				*(DWORD*)&packet[5] = AnyaID;
				*(DWORD*)&packet[9] = 0;
				D2NET_SendPacket(13, 0, packet);

				//wait for items to show up
				DWORD n;
				Sleep(1000);
				n = D2COMMON_GetItemFromInventory(UnitAnya->pInventory);
				while (n) {
					pItem = D2COMMON_GetUnitFromItem(n);
					if (IsGambleItem(pItem))
					{
						//buy item(gamble)
						packet[0] = 0x32;
						*(DWORD*)&packet[1] = AnyaID;
						*(DWORD*)&packet[5] = pItem->nUnitId;
						*(DWORD*)&packet[9] = 2;	//tab
						*(DWORD*)&packet[13] = D2COMMON_GetItemValue(D2Client_pPlayUnit, pItem,
							D2CLIENT_GetDifficulty(), D2CLIENT_GetQuestInfo(), NPCTxtFileID, 2);

						goldInBank = D2COMMON_GetUnitStat(player, STAT_GOLD_IN_BANK, 0);
						if (goldInBank <= g_Game_Config.KeepMoneyAfterGamble)
							break;

						DWORD GambleItemGUID;
						g_Game_Data.nGambleItemID = 0;
						D2NET_SendPacket(17, 0, packet);

						//Wait for 0x9c packet to get item GUID
						WAIT_UNTIL(500, 30, GambleItemGUID = g_Game_Data.nGambleItemID, FALSE);
			
						if (GambleItemGUID)
						{
							UnitAny* pGambleItem;
							
							WAIT_UNTIL(300, 10, (pGambleItem = D2CLIENT_GetUnitFromId(GambleItemGUID, UNITNO_ITEM)) && 
								pGambleItem->pItemData && D2COMMON_GetItemFlag(pGambleItem, ITEMFLAG_IDENTIFIED, 0, "?"),
								TRUE);

							char cName[256];
							memset(cName, 0, sizeof(cName));
							GetItemName(pGambleItem, cName);

							D2HackReply::ItemInfo replyInfo;
							replyInfo.Quality = (char)pGambleItem->pItemData->nQuality;
							strcpy_s(replyInfo.ItemName, sizeof(replyInfo.ItemName), cName);

							if (IsItemWanted(pGambleItem) == PickupItemsConfig::Match)
							{
								strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "赌博获取物品，[ %s ]。");
								SendHackMessage(D2RPY_OBTAIN_ITEM, replyInfo);
							}
							else
							{
								//sell garbage gamble items
								packet[0] = 0x19;
								*(DWORD*)&packet[1] = GambleItemGUID;
								D2NET_SendPacket(5, 0, packet);
								Sleep(100);

								packet[0] = 0x33;
								*(DWORD*)&packet[1] = AnyaID;
								*(DWORD*)&packet[5] = GambleItemGUID;
								*(DWORD*)&packet[9] = 4;//？
								*(DWORD*)&packet[13] = D2COMMON_GetItemValue(D2Client_pPlayUnit, pGambleItem, D2CLIENT_GetDifficulty(), D2CLIENT_GetQuestInfo(),
									NPCTxtFileID, 1);
								D2NET_SendPacket(17, 0, packet);
								Sleep(100);
								strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "赌博物品，[%s]卖给NPC。");
								SendHackMessage(D2RPY_FOUND_ITEM, replyInfo);
							}
						}
						n = D2COMMON_GetItemFromInventory(UnitAnya->pInventory);
					}
					else
						n = D2COMMON_GetNextItemFromInventory(n);
				}

				if (D2CLIENT_GetUiVar(UIVAR_INTERACT))
				{
					CloseMenu(UIVAR_INTERACT);
				}
			}
		}
		//End current game after gamble
		KillMe();
	}
}

static void BuyItem(DWORD ItemID, DWORD NPCID, DWORD Tab)
{
	UnitAny *player = D2Client_pPlayUnit;
	DWORD gold = D2COMMON_GetUnitStat(player, STAT_GOLD, 0);
	DWORD goldInBank = D2COMMON_GetUnitStat(player, STAT_GOLD_IN_BANK, 0);
	DWORD ItemValue = D2COMMON_GetItemValue(D2Client_pPlayUnit, D2CLIENT_GetUnitFromId(ItemID, UNITNO_ITEM),
		D2CLIENT_GetDifficulty(), D2CLIENT_GetQuestInfo(),
		D2CLIENT_GetUnitFromId(NPCID, UNITNO_MONSTER)->nTxtFileNo, 0);
	//向NPC买东西时物品价格与卖价格以及赌博的价格不一样，最后的参数不同！comment by Leon

	if ((gold + goldInBank) >= ItemValue)
	{
		BYTE packet[17];
		packet[0] = 0x32;
		*(DWORD*)&packet[1] = NPCID;
		*(DWORD*)&packet[5] = ItemID;
		*(DWORD*)&packet[9] = Tab;
		*(DWORD*)&packet[13] = ItemValue;

		D2NET_SendPacket(17, 0, packet);
	}
	else
	{
		SendHackMessage(D2RPY_LOG, "没有足够的金币用于购买物品！！");
	}
}

BOOL GotoIdentifyAndBuyPotions(StartLocation& s)
{
	//go to NPC pos
	DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
	if (!GotoTradeNPC(s))
		return FALSE;

	DWORD BloodPotionID = 0, ManaPotionID = 0, TownScrollID = 0, IdentifyScrollID = 0;
	BYTE packet[17];	
	short retry = 0;
	//buy potions and sell garbage
	//与NPC对话，接收物品ID
	
	POINT NPCPos;
	DWORD ID = GetTradeNPCID_Pos(NPCPos);
	if (!ID)
		return FALSE;

	packet[0] = { 0x13 };
	*(DWORD*)&packet[1] = UNITNO_MONSTER;
	*(DWORD*)&packet[5] = ID;
	D2NET_SendPacket(9, 0, packet);

	WAIT_UNTIL(300, 10, D2CLIENT_GetUiVar(UIVAR_INTERACT), TRUE);
	packet[0] = { 0x38 };
	*(DWORD*)&packet[1] = 1;
	*(DWORD*)&packet[5] = ID;
	*(DWORD*)&packet[9] = 0;
	D2NET_SendPacket(13, 0, packet);
	while (!BloodPotionID || !ManaPotionID)
	{
		//wait for items to show up, so we can receive potions ID
		DWORD n;
		UnitAny *pNPC = D2CLIENT_GetUnitFromId(ID, UNITNO_MONSTER), *pPotionUnit;
		WAIT_UNTIL(400, 8, pNPC->pInventory , FALSE);
		n = D2COMMON_GetItemFromInventory(pNPC->pInventory);
		while (n) {
			pPotionUnit = D2COMMON_GetUnitFromItem(n);
			switch (pPotionUnit->nTxtFileNo)
			{
			case 0x24f:
			case 0x24e:
				BloodPotionID = pPotionUnit->nUnitId;
				break;
			case 0x254:
			case 0x253:
				ManaPotionID = pPotionUnit->nUnitId;
				break;
			case 0x211:
				TownScrollID = pPotionUnit->nUnitId;
				break;
			case 0x212:
				IdentifyScrollID = pPotionUnit->nUnitId;
				break;
			}
			n = D2COMMON_GetNextItemFromInventory(n);
		}
		
		if (retry++ >= 2)
			break;
	}

	//辨识未识别的物品
	//Identifiy items
	//Get GUID of Identify scroll in body first
	UnitAny* item;
	DWORD n = D2COMMON_GetItemFromInventory(D2Client_pPlayUnit->pInventory);
	DWORD IdentifyBookID;
	BOOL bBuyIDScroll, bBuyTownScroll;
	DWORD UnIdentifiedItemsID[200], UnIdentifiedItemsNum = 0;

	while (n)
	{
		item = D2COMMON_GetUnitFromItem(n);
		if (item && !item->pItemData->nLocation2 && (GetItemCode(item->nTxtFileNo) + 1 == 2012))	//辨识书
		{
			IdentifyBookID = item->nUnitId;
			if (D2COMMON_GetUnitStat(item, STAT_QUANTITY, 0) < 20)
				bBuyIDScroll = TRUE;
			else
				bBuyIDScroll = FALSE;
		}
		else if (item && !item->pItemData->nLocation2 && (GetItemCode(item->nTxtFileNo) + 1 == 2011))	//回城书
		{
			if (D2COMMON_GetUnitStat(item, STAT_QUANTITY, 0) < 20)
				bBuyTownScroll = TRUE;
			else
				bBuyTownScroll = FALSE;
		}
		if (item && !item->pItemData->nLocation2 && !D2COMMON_GetItemFlag(item, ITEMFLAG_IDENTIFIED, 0, "?") &&
			(IsItemWanted(item) != PickupItemsConfig::Match))	//未辨识物品
			UnIdentifiedItemsID[UnIdentifiedItemsNum++] = item->nUnitId;
		n = D2COMMON_GetNextItemFromInventory(n);
	}

	if (IdentifyBookID && UnIdentifiedItemsNum)
	{
		for (DWORD i = 0; i < UnIdentifiedItemsNum; ++i)
		{
			packet[0] = 0x20;
			*(DWORD*)&packet[1] = IdentifyBookID;
			*(WORD*)&packet[5] = (D2Client_pPlayUnit->pPos->xpos1 >> 16);
			*(WORD*)&packet[7] = 0;
			*(WORD*)&packet[9] = (D2Client_pPlayUnit->pPos->ypos1 >> 16);
			*(WORD*)&packet[11] = 0;
			D2NET_SendPacket(13, 0, packet);
			Sleep(100);

			//Identify items
			packet[0] = 0x27;
			*(DWORD*)&packet[1] = UnIdentifiedItemsID[i];
			*(DWORD*)&packet[5] = IdentifyBookID;
			D2NET_SendPacket(9, 0, packet);
			
			WAIT_UNTIL(400, 20, (item=D2CLIENT_GetUnitFromId(UnIdentifiedItemsID[i], UNITNO_ITEM)) && 
				item->pItemData &&	D2COMMON_GetItemFlag(item, ITEMFLAG_IDENTIFIED, 0, "?"), FALSE);

			item = D2CLIENT_GetUnitFromId(UnIdentifiedItemsID[i], UNITNO_ITEM);

			char cName[256];
			memset(cName, 0, sizeof(cName));
			GetItemName(item, cName);

			D2HackReply::ItemInfo replyInfo;
			replyInfo.Quality = (char)item->pItemData->nQuality;
			strcpy_s(replyInfo.ItemName, sizeof(replyInfo.ItemName), cName);

			//sell garbage
			if (IsItemWanted(item) == PickupItemsConfig::NotMatch)
			{
				packet[0] = 0x19;
				*(DWORD*)&packet[1] = UnIdentifiedItemsID[i];
				D2NET_SendPacket(5, 0, packet);
				Sleep(100);

				packet[0] = 0x33;
				*(DWORD*)&packet[1] = ID;
				*(DWORD*)&packet[5] = UnIdentifiedItemsID[i];
				*(DWORD*)&packet[9] = 4;//？
				*(DWORD*)&packet[13] = D2COMMON_GetItemValue(D2Client_pPlayUnit, item, D2CLIENT_GetDifficulty(), D2CLIENT_GetQuestInfo(), 
					D2CLIENT_GetUnitFromId(ID, UNITNO_MONSTER)->nTxtFileNo, 1);
				D2NET_SendPacket(17, 0, packet);
				Sleep(100);
				
				strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "鉴别物品，[%s]卖给NPC。");				
				SendHackMessage(D2RPY_FOUND_ITEM, replyInfo);
			}
			else
			{
				strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "获取物品，[ %s ]。");
				SendHackMessage(D2RPY_OBTAIN_ITEM, replyInfo);
			}
		}
	}

#ifdef IMPK_BATTLENET
	//*********code for IMPK battle.net******************//
	Sleep(800);
	UnitAny *pNPC = D2CLIENT_GetUnitFromId(ID, UNITNO_MONSTER), *pItem;
	n = D2COMMON_GetItemFromInventory(pNPC->pInventory);
	while (n) {
		pItem = D2COMMON_GetUnitFromItem(n);
		//1#----33#
		if ((pItem->nTxtFileNo >= 0x262) && (pItem->nTxtFileNo <= 0x282) && (IsItemWanted(pItem) == PickupItemsConfig::Match))
		{
			BuyItem(pItem->nUnitId, ID, 0);

			char cName[256];
			memset(cName, 0, sizeof(cName));
			GetItemName(pItem, cName);

			D2HackReply::ItemInfo replyInfo;
			replyInfo.Quality = (char)pItem->pItemData->nQuality;
			strcpy_s(replyInfo.ItemName, sizeof(replyInfo.ItemName), cName);
			strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "卖出精华暗金物品获取符文，[ %s ]。");
			SendHackMessage(D2RPY_OBTAIN_ITEM, replyInfo);
		}
		n = D2COMMON_GetNextItemFromInventory(n);
	}
	//*********end of code for IMPK battle.net******************//
#endif

	//buy potions
	UpdateBeltPotionPos();
	for (int i = 0; i <= 3; ++i)
	{
		if (!g_Game_Data.Potions[4 * i].PotionGUID && BloodPotionID)
		{
			BuyItem(BloodPotionID, ID, 0);
			Sleep(100);
		}
		if (!g_Game_Data.Potions[4 * i + 1].PotionGUID && ManaPotionID)
		{
			BuyItem(ManaPotionID, ID, 0);
			Sleep(100);
		}
	}

	//buy scroll
	if (bBuyTownScroll && TownScrollID)
	{
		BuyItem(TownScrollID, ID, 0x80000000);
		Sleep(100);
	}
	if (bBuyIDScroll && IdentifyScrollID)
	{
		BuyItem(IdentifyScrollID, ID, 0x80000000);
		Sleep(100);
	}

	//close NPC menu
	if (D2CLIENT_GetUiVar(UIVAR_INTERACT))
	{
		CloseMenu(UIVAR_INTERACT);		
	}

	return TRUE;
}

void ShopItem(DWORD NPCID)
{
	BYTE packet[17];
	if (!D2CLIENT_GetUiVar(UIVAR_INTERACT))
	{
		packet[0] = 0x13;
		*(DWORD*)&packet[1] = UNITNO_MONSTER;
		*(DWORD*)&packet[5] = NPCID;
		D2NET_SendPacket(9, 0, packet);
		WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_INTERACT), TRUE);
	}
	packet[0] = { 0x38 };
	*(DWORD*)&packet[1] = 1;	//action ID
	*(DWORD*)&packet[5] = NPCID;
	*(DWORD*)&packet[9] = 0;
	D2NET_SendPacket(13, 0, packet);

	//wait for items to sshow up
	DWORD n;
	Sleep(1000);
	UnitAny *pNPC = D2CLIENT_GetUnitFromId(NPCID, UNITNO_MONSTER);
	n = D2COMMON_GetItemFromInventory(pNPC->pInventory);
	while (n) {
		UnitAny *pItem = D2COMMON_GetUnitFromItem(n);
		if (IsItemWanted(pItem) == PickupItemsConfig::Match)
		{
			char cName[256];
			memset(cName, 0, sizeof(cName));
			GetItemName(pItem, cName);
			D2HackReply::ItemInfo replyInfo;
			replyInfo.Quality = (char)pItem->pItemData->nQuality;
			strcpy_s(replyInfo.ItemName, sizeof(replyInfo.ItemName), cName);

			strcpy_s(replyInfo.Msg, sizeof(replyInfo.Msg), "刷商店获取物品，[ %s ]。");
			SendHackMessage(D2RPY_OBTAIN_ITEM, replyInfo);

			BuyItem(pItem->nUnitId, NPCID, 0);
		}
		n = D2COMMON_GetNextItemFromInventory(n);
	}

	if (D2CLIENT_GetUiVar(UIVAR_INTERACT))
	{
		CloseMenu(UIVAR_INTERACT);
	}
}

BOOL GotoResurMerc(StartLocation& s)
{
	UnitAny *player = D2Client_pPlayUnit;
	DWORD gold = D2COMMON_GetUnitStat(player, STAT_GOLD, 0);
	DWORD goldInBank = D2COMMON_GetUnitStat(player, STAT_GOLD_IN_BANK, 0);
	//金币小于50000
	if ((gold + goldInBank) < 50000)
		return FALSE;

	DWORD act = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));

	//double check if merc is alive
	Sleep(1000);
	if (GetPlayerMercUnit())
		return TRUE;

	//go to act1
	if (!GoTakeWP(s, 1))
		return FALSE;

	POINT NPCPos;
	DWORD ID = GetNPCID_Pos(0x96, NPCPos);	//A1 卡夏复活NPC
	WAIT_UNTIL(300, 10, ID = GetNPCID_Pos(0x96, NPCPos), TRUE);
	
	if (!GotoCoordinate(NPCPos.x, NPCPos.y))
		return FALSE;

	UpdateNPCPos(ID, NPCPos);
	if (!GotoCoordinate(NPCPos.x, NPCPos.y))
		return FALSE;

	//check if merc is alive again!!!!! IMPORTANT!!!!
	if (g_Game_Data.MercGUID || GetPlayerMercUnit())
		return TRUE;

	//与NPC对话
	BYTE packet[17] = { 0x59 };
	*(DWORD*)&packet[1] = UNITNO_MONSTER;
	*(DWORD*)&packet[5] = ID;
	*(WORD*)&packet[9] = (D2Client_pPlayUnit->pPos->xpos1 >> 16);
	*(WORD*)&packet[11] = 0;
	*(WORD*)&packet[13] = (D2Client_pPlayUnit->pPos->ypos1 >> 16);
	*(WORD*)&packet[15] = 0;
	D2NET_SendPacket(17, 0, packet);
	Sleep(400);

	packet[0] = 0x13;
	*(DWORD*)&packet[1] = UNITNO_MONSTER;
	*(DWORD*)&packet[5] = ID;
	D2NET_SendPacket(9, 0, packet);

	WAIT_UNTIL(300, 10, D2CLIENT_GetUiVar(UIVAR_INTERACT), FALSE);
	
	packet[0] = { 0x38 };
	*(DWORD*)&packet[1] = 3;	//action
	*(DWORD*)&packet[5] = ID;
	*(DWORD*)&packet[9] = 1;
	D2NET_SendPacket(13, 0, packet);
	Sleep(100);

	packet[0] = 0x62;
	*(DWORD*)&packet[1] = ID;
	D2NET_SendPacket(5, 0, packet);

	WAIT_UNTIL(300, 10, g_Game_Data.MercGUID, FALSE);

	//close NPC menu
	if (D2CLIENT_GetUiVar(UIVAR_INTERACT))
	{
		CloseMenu(UIVAR_INTERACT);
	}

	s = From_StartPoint;

	return TRUE;
}

BOOL TakeWP(DWORD wpID, DWORD destWPID, DWORD wpLvl)
{
	BYTE packet[9];

	if (!D2CLIENT_GetUiVar(UIVAR_WAYPOINT))
	{
		//Take WP
		packet[0] = 0x13;
		*(DWORD*)&packet[1] = UNITNO_OBJECT;
		*(DWORD*)&packet[5] = wpID;
		D2NET_SendPacket(9, 0, packet);
		WAIT_UNTIL(200, 5, D2CLIENT_GetUiVar(UIVAR_WAYPOINT), FALSE);
	}
	if (!D2CLIENT_GetUiVar(UIVAR_WAYPOINT))
	{
		//fix a2 last waypoint prob
		packet[0] = 0x13;
		*(DWORD*)&packet[1] = UNITNO_OBJECT;
		*(DWORD*)&packet[5] = wpID;
		D2NET_SendPacket(9, 0, packet);
		WAIT_UNTIL(200, 25, D2CLIENT_GetUiVar(UIVAR_WAYPOINT), TRUE);
	}

	memset(packet, 0, sizeof(packet));
	packet[0] = 0x49;
	*(DWORD*)&packet[1] = wpID;
	*(DWORD*)&packet[5] = destWPID;
	D2NET_SendPacket(9, 0, packet);

	DrlgLevel *lv;
	WAIT_UNTIL(50, 50, (lv = GetUnitDrlgLevel(D2Client_pPlayUnit)) && (lv->nLevelNo == wpLvl), TRUE);

	//close WayPoint menu
	if (D2CLIENT_GetUiVar(UIVAR_WAYPOINT))
	{
		CloseMenu(UIVAR_WAYPOINT);
	}

	return TRUE;
}

//获取宝箱里或者Inventory里物品的宽度和高度
//nLocation = 0：身上，4：宝箱, 3：方块里
void GetItemSize(UnitAny* item, WORD& width, WORD& height, DWORD nLocation)
{
	int x = item->pItemPath->xpos;
	int y = item->pItemPath->ypos;
	int x2, y2 = y + 1;

	BYTE Max_Width, Max_Height;
	GetCabinetSize(D2Client_pPlayUnit, nLocation, Max_Width, Max_Height);

	for (; y2 < Max_Height; ++y2)
	{
		if (GetPosUnit(x, y2, nLocation) != item)
			break;
	}

	x2 = x + 1;
	for (; x2 < Max_Width; ++x2)
	{
		if (GetPosUnit(x2, y, nLocation) != item)
			break;
	}

	width = x2 - x;
	height = y2 - y;
}

//在宝箱里查找容纳width和height大小的空间的位置
BOOL FindPosToStoreItemToBank(WORD width, WORD height, WORD& PosX, WORD& PosY)
{
	//nLocation = 0：身上，4：宝箱, 3：方块里
	DWORD nLocation = 4;
	BYTE Max_Width, Max_Height;
	GetCabinetSize(D2Client_pPlayUnit, nLocation, Max_Width, Max_Height);

	int x, y;
	int x2 = 0, y2 = 0;
	BOOL bol;
	for (y = 0; y < Max_Height - height + 1; ++y)
	{
		for (x = 0; x < Max_Width - width + 1; ++x)
		{
			bol = TRUE;
			for (x2 = x; (x2 < x + width) && bol; ++x2)
				for (y2 = y; (y2 < y + height) && bol; ++y2)
				{
					if (GetPosUnit(x2, y2, 4))
						bol = FALSE;
				}
			if (bol)
			{
				PosX = x;
				PosY = y;
				return TRUE;
			}
		}
	}
	return FALSE;
}

//将Item放到宝箱的x,y位置
void DepositItem(DWORD ItemID, WORD ToPosX, WORD ToPosY)
{
	UnitAny *pItem = D2CLIENT_GetUnitFromId(ItemID, UNITNO_ITEM);
	BYTE packet[17];
	packet[0] = 0x19;
	*(DWORD*)&packet[1] = ItemID;
	D2NET_SendPacket(5, 0, packet);
	WAIT_UNTIL(100, 100, pItem->eMode == 4, TRUE);	//等到item拿到鼠标上

	packet[0] = 0x18;
	*(DWORD*)&packet[1] = ItemID;
	*(DWORD*)&packet[5] = ToPosX;
	*(DWORD*)&packet[9] = ToPosY;
	*(DWORD*)&packet[13] = 4;	//存储到宝箱里
	D2NET_SendPacket(17, 0, packet);
	WAIT_UNTIL(100, 100, pItem->pItemData->nLocation2 == 4, TRUE);	//等到item放进宝箱里
}

void GotoReorgnizeInventoryItems(StartLocation& s)
{
	if (GotoBank(s))
	{
		POINT BankPos;//储物箱	
		DWORD bankID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BankPos, 267, UNITNO_OBJECT);
		if (!bankID)
			return;
		BYTE packet[9] = { 0x13 };
		*(DWORD*)&packet[1] = UNITNO_OBJECT;
		*(DWORD*)&packet[5] = bankID;
		D2NET_SendPacket(9, 0, packet);
		WAIT_UNTIL(200, 10, D2CLIENT_GetUiVar(UIVAR_STASH), FALSE);

		if (D2CLIENT_GetUiVar(UIVAR_STASH))
		{
			WORD width, height;
			WORD x, y;
			for (int PosX = g_Game_Config.ReorganizeItemPosStartCol; PosX <= g_Game_Config.ReorganizeItemPosEndCol; ++PosX)
			{
				for (int PosY = 0; PosY <= 3; ++PosY)
				{
					UnitAny* pItem = GetPosUnit(PosX, PosY, 0);
					if (pItem)
					{
						GetItemSize(pItem, width, height, 0);
						if (FindPosToStoreItemToBank(width, height, x, y))
						{
							DepositItem(pItem->nUnitId, x, y);
							Sleep(500);
						}
					}
				}
			}

			CloseMenu(UIVAR_STASH);
			Sleep(100);
		}
	}
}

void RunBot()
{
	if (!Login())
	{
		SendHackMessage(D2RPY_ERROR_NEED_RESTART, "登录服务器出错！");
		return;
	}

	StartRunBot();
}

#define CMD_PROCESS(ID, FUN)	case ID: FUN;break;
DWORD WINAPI CommandProcess(LPVOID lpParam)
{
	D2HackCommandBuffer* cmd = (D2HackCommandBuffer*)lpParam;
	switch (cmd->ClientCommand.nCmdID)
	{
		CMD_PROCESS(D2HackCommandID::D2CMD_Login, Login())
		CMD_PROCESS(D2HackCommandID::D2CMD_RunAct1Boss, RunAct1Boss())
		CMD_PROCESS(D2HackCommandID::D2CMD_RunBot, RunBot())
		CMD_PROCESS(D2HackCommandID::D2CMD_PauseBot, if (g_hRunThreadHandle) SuspendThread(g_hRunThreadHandle);)
		CMD_PROCESS(D2HackCommandID::D2CMD_ResumeBot, if (g_hRunThreadHandle) ResumeThread(g_hRunThreadHandle);)
		CMD_PROCESS(D2HackCommandID::D2CMD_ReloadConfig, ReadConfigFile();)
		CMD_PROCESS(D2HackCommandID::D2CMD_RevealMap, RevealCurrentLevelMap();)
		CMD_PROCESS(D2HackCommandID::D2CMD_PermShow, g_Game_Config.bPermShowConfig = !g_Game_Config.bPermShowConfig; )
		CMD_PROCESS(D2HackCommandID::D2CMD_WeatherOff, g_Game_Config.bWeatherEffect = !g_Game_Config.bWeatherEffect; )
	}
	return 0;
}