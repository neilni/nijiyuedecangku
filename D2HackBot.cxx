#include "stdafx.h"
#include "misc.h"
#include "D2PtrHookHeader.h"
#include "D2Hack1.11b.h"

BOOL KillMonster(DWORD MonsterID, int KeepDistX, int KeepDistY, BOOL bChangePos/*是否打一下换个方向打*/, short RetryCount = 10,
	BOOL bSendMsg = TRUE, BOOL bSearchItem = TRUE)
{
	if (g_Game_Config.ExitWhenMonsterImm &&
		IsMonsterImmune(D2CLIENT_GetUnitFromId(MonsterID, UNITNO_MONSTER), (UnitStat)g_Game_Config.ExitWhenMonsterImm))
	{
		if(bSendMsg)
			SendHackMessage(D2RPY_LOG, L"【%s】法术免疫！",	D2CLIENT_GetMonsterName(D2CLIENT_GetUnitFromId(MonsterID, UNITNO_MONSTER)));
		return FALSE;
	}

	short retry;
	retry = 0;
	POINT pt;

	auto pFuncKBoss = [&]() {
		if (!IsMonsterKilled(MonsterID))
		{
			unsigned int mana = D2COMMON_GetUnitStat(D2Client_pPlayUnit, STAT_MANA, 0);
			mana >>= 8; // actual mana
			unsigned int maxmana = D2COMMON_GetUnitStat(D2Client_pPlayUnit, STAT_MAXMANA, 0);
			maxmana >>= 8;

			double percent = ((double)mana) / maxmana;
			if ((percent < 0.1) && (!g_Game_Data.Potions[4].PotionGUID))
				BackToTownToBuyPotions((WORD)GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo);
			
			int delay;
			if (g_Game_Config.AssistSkill)
			{
				SelectSkill(g_Game_Config.AssistSkill, g_Game_Config.AssistSkillHand);
				Sleep(500);
			}
			SelectSkill(g_Game_Config.PrimarySkill, g_Game_Config.PrimarySkillHand);
			Sleep(50);
			WaitToBeReadyToCastSkill(1500);
			CastSkillToMonster(MonsterID, g_Game_Config.PrimarySkillHand);

			delay = g_Game_Config.PrimarySkillDelay - 4 * g_Game_Config.SecondSkillDelay;

			if (delay <= 0)
				delay = 200;
			Sleep(delay);
			if (g_Game_Config.SecondSkill)
			{
				SelectSkill(g_Game_Config.SecondSkill, g_Game_Config.SecondSkillHand);
				Sleep(50);
				WaitToBeReadyToCastSkill(1500);
				CastSkillToMonster(MonsterID, g_Game_Config.SecondSkillHand);
				Sleep(g_Game_Config.SecondSkillDelay);
				WaitToBeReadyToCastSkill(1500);
				CastSkillToMonster(MonsterID, g_Game_Config.SecondSkillHand);
				Sleep(g_Game_Config.SecondSkillDelay);
				WaitToBeReadyToCastSkill(1500);
				CastSkillToMonster(MonsterID, g_Game_Config.SecondSkillHand);
				Sleep(g_Game_Config.SecondSkillDelay);
				WaitToBeReadyToCastSkill(1500);
				CastSkillToMonster(MonsterID, g_Game_Config.SecondSkillHand);
				Sleep(g_Game_Config.SecondSkillDelay);
			}
		}
	};
	
	while ((!IsMonsterKilled(MonsterID)) && (retry++ < RetryCount))
	{
		UpdateNPCPos(MonsterID, pt);
		if (KeepDistX)
		{
			if (!IsMonsterKilled(MonsterID) && 
				(IsMonsterNearby() ||
				(abs(int(pt.x - (D2Client_pPlayUnit->pPos->xpos1 >> 16))) > KeepDistX) || bChangePos))
				TP2Coordinate(pt.x + KeepDistX, pt.y, 5, MAX_DIST_FOR_SINGLE_TP, 4);
			pFuncKBoss();

			UpdateNPCPos(MonsterID, pt);
			if (!IsMonsterKilled(MonsterID) && 
				(IsMonsterNearby() ||
				(abs(int(pt.x - (D2Client_pPlayUnit->pPos->xpos1 >> 16))) > KeepDistX) || bChangePos))
				TP2Coordinate(pt.x - KeepDistX, pt.y, 5, MAX_DIST_FOR_SINGLE_TP, 4);
			pFuncKBoss();
		}

		if (KeepDistY)
		{
			UpdateNPCPos(MonsterID, pt);
			if (!IsMonsterKilled(MonsterID) && 
				(IsMonsterNearby() || bChangePos ||
				(abs(int(pt.y - (D2Client_pPlayUnit->pPos->ypos1 >> 16))) > KeepDistY)))
				TP2Coordinate(pt.x, pt.y + KeepDistY, 5, MAX_DIST_FOR_SINGLE_TP, 4);
			pFuncKBoss();

			UpdateNPCPos(MonsterID, pt);
			if (!IsMonsterKilled(MonsterID) && 
				(IsMonsterNearby() || bChangePos ||
				(abs(int(pt.y - (D2Client_pPlayUnit->pPos->ypos1 >> 16))) > KeepDistY)))
				TP2Coordinate(pt.x, pt.y - KeepDistY, 5, MAX_DIST_FOR_SINGLE_TP, 4);
			pFuncKBoss();
		}
	}

	if (IsMonsterKilled(MonsterID))
	{
		if (bSendMsg)
			SendHackMessage(D2RPY_LOG, L"成功消灭【%s】！",
				D2CLIENT_GetMonsterName(D2CLIENT_GetUnitFromId(MonsterID, UNITNO_MONSTER)));

		if (bSearchItem)
		{
			//Pick Items
			Sleep(150);
			SearchItemsWantedNearby(D2Client_pPlayUnit->pPos->pRoom1);
			PickItem();
			Sleep(100);
			SearchItemsWantedNearby(D2Client_pPlayUnit->pPos->pRoom1);
			PickItem();
		}
	}
	return IsMonsterKilled(MonsterID);
}

void ClearLevel(WORD nLayerLevel)
{
	extern BOOL GeneratePathToMonsterRoom(const DrlgRoom2 *pStart, const DrlgRoom2* pDest, MonsterPathTree& PathSource, MonsterPathTree& PathDest);

	POINT TotalRooms[128];
	short RoomCleared = 0;

	auto pFuncIsRoomCleared = [&](DWORD RoomX, DWORD RoomY) {
		for (int i = 0; i < RoomCleared; ++i)
		{
			if ((TotalRooms[i].x == RoomX) && (TotalRooms[i].y == RoomY))
				return TRUE;
		}
		return FALSE;
	};

	auto pFuncClearRoomMonsters = [&](DrlgRoom1* Room){
		if (pFuncIsRoomCleared(Room->pRoom2->xPos, Room->pRoom2->yPos))
			return;

		ClearRoomMonsters(Room);

		if (RoomCleared < SIZEOFARRAY(TotalRooms))
		{
			TotalRooms[RoomCleared].x = Room->pRoom2->xPos;
			TotalRooms[RoomCleared++].y = Room->pRoom2->yPos;
		}

		for (DWORD i = 0; i < Room->pRoom2->nRoomsNear; ++i)
		{
			if (!Room->pUnitFirst && !pFuncIsRoomCleared(Room->pRoom2->paRoomsNear[i]->xPos, Room->pRoom2->paRoomsNear[i]->yPos))
			{
				if (RoomCleared < SIZEOFARRAY(TotalRooms))
				{
					TotalRooms[RoomCleared].x = Room->pRoom2->paRoomsNear[i]->xPos;
					TotalRooms[RoomCleared++].y = Room->pRoom2->paRoomsNear[i]->yPos;
				}
			}
		}
	};

	//InitCurrentLevelDrlgRoom();
	g_Game_Data.bLevelToBeInitialized = TRUE;
	WAIT_UNTIL(50, 100, !g_Game_Data.bLevelToBeInitialized, FALSE);

	DrlgLevel *pDrlgLevel = GetDrlgLevel(D2CLIENT_pDrlgAct->pDrlgMisc, nLayerLevel);
	DrlgRoom2 *curroom = pDrlgLevel->pRoom2First;
	while (curroom)
	{
		if (pFuncIsRoomCleared(curroom->xPos, curroom->yPos))
			goto _ClearNext;

		if (curroom != D2Client_pPlayUnit->pPos->pRoom1->pRoom2)
		{
			MonsterPathTree MonsterPathSource, MonsterPathDest;
			if (GeneratePathToMonsterRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, curroom, MonsterPathSource, MonsterPathDest))
			{
				//tp to lv entrance
				MonsterPathTree *path = MonsterPathSource.pPreNode;
				while (path)
				{
					if (!path->pPreNode)
					{
						pFuncClearRoomMonsters(((DrlgRoom2*)path->pRoom)->pRoom1);
						SendHackMessage(D2RPY_PING, "");
						break;
					}

					TP2Coordinate(path->RoomPos.x * 5 + path->RoomPos.size.cx * 5 / 2, path->RoomPos.y * 5 + path->RoomPos.size.cy * 5 / 2, 22, MAX_DIST_FOR_SINGLE_TP, 2);
					pFuncClearRoomMonsters(((DrlgRoom2*)path->pRoom)->pRoom1);
					SendHackMessage(D2RPY_PING, "");
					
					UpdateBeltPotionPos();
					if (!g_Game_Data.Potions[0].PotionGUID || !g_Game_Data.Potions[1].PotionGUID)
					{
						BackToTownToBuyPotions(nLayerLevel);
						curroom = D2Client_pPlayUnit->pPos->pRoom1->pRoom2;
						break;
					}
					path = path->pPreNode;
				}
				DeleteMonsterPathTree(&MonsterPathDest);
			}
		}
		else
		{
			pFuncClearRoomMonsters(curroom->pRoom1);
			SendHackMessage(D2RPY_PING, "");
			UpdateBeltPotionPos();
			if (!g_Game_Data.Potions[5].PotionGUID)
			{
				BackToTownToBuyPotions(nLayerLevel);
				curroom = D2Client_pPlayUnit->pPos->pRoom1->pRoom2;
			}
		}
		_ClearNext:
		curroom = curroom->pNext;
	}
}

//k地穴
void ClearPit()
{
	ClearLevel(12);	//一层
	if (GotoFindAndTakeEntrance(16))
		ClearLevel(16);	//二层
}

void KillBarr(StartLocation& s)
{
	DWORD MonstersGUID[128], NumOfMonsters;
	auto pFuncClearMonsters = [&]() {
		GetCurrentRoom1Monsters(D2Client_pPlayUnit->pPos->pRoom1, MonstersGUID, SIZEOFARRAY(MonstersGUID), NumOfMonsters);
		for (DWORD i = 0; i < NumOfMonsters; ++i)
		{
			if (D2CLIENT_GetUnitFromId(MonstersGUID[i], UNITNO_MONSTER)->nTxtFileNo != 543) //站台上的巴尔
				KillMonster(MonstersGUID[i], 1, 1, TRUE, 10, FALSE, FALSE);
		}
	};

	TP2Coordinate(0x3af6, 0x13c7, 5, MAX_DIST_FOR_SINGLE_TP, 15);
	pFuncClearMonsters();

#ifdef _DEBUG
	//////////tmp code
	//open a door
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
	/////////////////
#endif // _DEBUG

	TP2Coordinate(0x3af2, 0x1394, 5, MAX_DIST_FOR_SINGLE_TP, 15);
	pFuncClearMonsters();

	SearchItemsWantedNearby(D2Client_pPlayUnit->pPos->pRoom1);
	PickItem();

	//开始k五小队
	DWORD BossID;
	POINT BossPos;
	short Retry = 0;
	while (!(BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 62, UNITNO_MONSTER, TRUE, 61)))
	{
		TP2Coordinate(0x3af6, 0x13c7, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();

		TP2Coordinate(0x3af2, 0x1394, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();

		if (Retry++ > 15)
			return;
	}

	if (BossID)
		KillMonster(BossID, 10, 10, FALSE);

	Retry = 0;
	while (!(BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 105, UNITNO_MONSTER, TRUE, 62)))
	{
		TP2Coordinate(0x3af6, 0x13c7, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();
		
		TP2Coordinate(0x3af2, 0x1394, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();
		if (Retry++ > 15)
			return;
	}

	if (BossID)
		KillMonster(BossID, 10, 10, FALSE);

	Retry = 0;
	while (!(BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 557, UNITNO_MONSTER, TRUE, 63)))
	{
		TP2Coordinate(0x3af6, 0x13c7, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();

		TP2Coordinate(0x3af2, 0x1394, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();
		if (Retry++ > 15)
			return;
	}

	if (BossID)
		KillMonster(BossID, 10, 10, FALSE);

	Retry = 0;
	while (!(BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 558, UNITNO_MONSTER, TRUE, 64)))
	{
		TP2Coordinate(0x3af6, 0x13c7, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();

		TP2Coordinate(0x3af2, 0x1394, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();
		if (Retry++ > 15)
			return;
	}

	if (BossID)
		KillMonster(BossID, 10, 10, FALSE);

	Retry = 0;
	while (!(BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 571, UNITNO_MONSTER, TRUE, 65)))
	{
		TP2Coordinate(0x3af6, 0x13c7, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();

		TP2Coordinate(0x3af2, 0x1394, 5, MAX_DIST_FOR_SINGLE_TP, 15);
		Sleep(1000);
		pFuncClearMonsters();
		Sleep(1000);
		pFuncClearMonsters();
		if (Retry++ > 15)
			return;
	}

	if (BossID)
		KillMonster(BossID, 10, 10, FALSE);
}

void KillNil(StartLocation& s)
{
	DrlgRoom2 *pRoomTo;
	POINT pt;
	//MonsterPathTree MonsterPathSource, MonsterPathDest;
	s = From_Unknown;

	if (!GetLevelPresetObj((WORD)GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo, pt, &pRoomTo, NULL, 462))	//462老尼站的正方形地方的中心的objno
		return;

	FindPathAndTP2DestRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoomTo);

	/*if (!GeneratePathToMonsterRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoomTo, MonsterPathSource, MonsterPathDest))
	{
		DeleteMonsterPathTree(&MonsterPathDest);
		RevealCurrentLevelMap();
		if (!GeneratePathToMonsterRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoomTo, MonsterPathSource, MonsterPathDest))
		{
			DeleteMonsterPathTree(&MonsterPathDest);
			return;
		}
	}
	MonsterPathTree *path = MonsterPathSource.pPreNode;
	while (path)
	{
		//TP to room2
		TP2Coordinate(path->RoomPos.x * 5 + path->RoomPos.size.cx * 5 / 2, path->RoomPos.y * 5 + path->RoomPos.size.cy * 5 / 2, 20, MAX_DIST_FOR_SINGLE_TP, 15);
		path = path->pPreNode;
	}
	DeleteMonsterPathTree(&MonsterPathDest);*/
	TP2Coordinate(pt.x - 10, pt.y - 10, 5, MAX_DIST_FOR_SINGLE_TP, 2);

	POINT MonsterPT;
	DWORD NilID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, MonsterPT, 526, UNITNO_MONSTER);//尼拉塞克 

	if (NilID)
	{
		UpdateNPCPos(NilID, MonsterPT);
		TP2Coordinate(MonsterPT.x - 15, MonsterPT.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);
		KillMonster(NilID, 15, 15, FALSE);
	}
	s = From_Unknown;
}

void KillCountress(StartLocation& s)
{
	POINT pt;
	DWORD nCountressID;
	s = From_Unknown;
	TP2Coordinate(0x30fc, 0x2b3b, 2, MAX_DIST_FOR_SINGLE_TP, 10);

	if (!(nCountressID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x2d, UNITNO_MONSTER, TRUE)))
	{
		TP2Coordinate(0x3115, 0x2b14, 2, MAX_DIST_FOR_SINGLE_TP, 10);
		if (!(nCountressID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x2d, UNITNO_MONSTER, TRUE)))
		{
			TP2Coordinate(0x3130, 0x2b1a, 2, MAX_DIST_FOR_SINGLE_TP, 10);
			nCountressID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x2d, UNITNO_MONSTER, TRUE);
		}
	}
	if (nCountressID)
	{
		UpdateNPCPos(nCountressID, pt);
		TP2Coordinate(pt.x, pt.y + 8, 5, MAX_DIST_FOR_SINGLE_TP, 2);
		KillMonster(nCountressID, 15 ,15, TRUE);
	}
	s = From_Unknown;
}

void KillAndy(StartLocation& s)
{
#ifdef _DEBUG
	/////开门  tmp code
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
#endif
	//开始杀安姐
	POINT AndyPos;
	DWORD AndyID;
	WAIT_UNTIL(200, 20, AndyID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, AndyPos, 0x9c, UNITNO_MONSTER), FALSE);
	if (!AndyID)
		return;

	UpdateNPCPos(AndyID, AndyPos);
	TP2Coordinate(AndyPos.x, AndyPos.y + 15, 5, MAX_DIST_FOR_SINGLE_TP, 4);
	KillMonster(AndyID, 0, 15, FALSE);	
	s = From_Unknown;
}

void KillSummoner(StartLocation& s)
{
	POINT MonsterPT;
	POINT DiaryPT;
	DrlgRoom2 *room;
	//MonsterPathTree MonsterPathSource, MonsterPathDest;
	s = From_Unknown;

	//找召唤者旁边的日记
	if (GetLevelPresetObj((WORD)GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo, DiaryPT, &room, NULL, 0x165))
	{
		FindPathAndTP2DestRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, room);
		TP2Coordinate(DiaryPT.x, DiaryPT.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);
	}

	s = From_Unknown;
	DWORD SummonerID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, MonsterPT, 250, UNITNO_MONSTER);

	if (SummonerID)
	{
		KillMonster(SummonerID, 15, 15, FALSE);
		if (IsMonsterKilled(SummonerID))
		{
			//顺便下到下一个场景的wp
			DWORD DiaryID = FindUnitGUIDFromPos(D2Client_pPlayUnit->pPos->pRoom1, DiaryPT);
			if (DiaryID)
			{
				TP2Coordinate(DiaryPT.x, DiaryPT.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);
				BYTE packet[9];
				packet[0] = 0x13;
				*(DWORD*)&packet[1] = 2;
				*(DWORD*)&packet[5] = DiaryID;
				D2NET_SendPacket(9, 0, packet);
				
				Sleep(500);
				SendVKey(D2GFX_GetHwnd(), VK_ESCAPE);
				if (D2CLIENT_GetUiVar(UIVAR_GAMEMENU))
					D2CLIENT_SetUiVar(UIVAR_GAMEMENU, 1, 1);				

				POINT DoorPT;
				DWORD DoorID;
				WAIT_UNTIL(300, 5, DoorID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, DoorPT, 0x3c, UNITNO_OBJECT), FALSE);
				if (!DoorID)
				{
					TP2Coordinate(DiaryPT.x, DiaryPT.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);
					packet[0] = 0x13;
					*(DWORD*)&packet[1] = 2;
					*(DWORD*)&packet[5] = DiaryID;
					D2NET_SendPacket(9, 0, packet);
					Sleep(500);
					SendVKey(D2GFX_GetHwnd(), VK_ESCAPE);
					if (D2CLIENT_GetUiVar(UIVAR_GAMEMENU))
						D2CLIENT_SetUiVar(UIVAR_GAMEMENU, 1, 1);

					WAIT_UNTIL(300, 5, DoorID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, DoorPT, 0x3c, UNITNO_OBJECT), FALSE);
				}

				if (DoorID)
				{
					TP2Coordinate(DoorPT.x, DoorPT.y, 2, MAX_DIST_FOR_SINGLE_TP, 2);
					packet[0] = 0x13;
					*(DWORD*)&packet[1] = 2;
					*(DWORD*)&packet[5] = DoorID;
					D2NET_SendPacket(9, 0, packet);

					DrlgLevel *lv = GetUnitDrlgLevel(D2Client_pPlayUnit);
					WAIT_UNTIL(400, 10, D2Client_pPlayUnit && (lv = GetUnitDrlgLevel(D2Client_pPlayUnit)) && (lv->nLevelNo == 0x2e), FALSE);
					if (lv->nLevelNo == 0x2e)
					{
						POINT WaypointPT;
						if (GetLevelPresetObj(0x2e, WaypointPT, NULL, "Waypoint", 0))
						{
							TP2Coordinate(WaypointPT.x, WaypointPT.y, 2, MAX_DIST_FOR_SINGLE_TP, 2);
							s = From_WP;
						}
					}
				}
			}
		}
	}
}

void KillA2Boss(StartLocation& s)
{
	DrlgRoom2 *pRoomTo;
	POINT pt;
	//MonsterPathTree MonsterPathSource, MonsterPathDest;
	s = From_Unknown;
	if (!GetLevelPresetObj((int)D2CLIENT_pDrlgAct->pDrlgMisc->nStaffTombLvl, pt, &pRoomTo, NULL, 152))//orifice
		return;

	FindPathAndTP2DestRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoomTo);
	
	TP2Coordinate(pt.x, pt.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);

	//查找洞口id和坐标
	DWORD DoorID;

	WAIT_UNTIL(200, 20, DoorID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x64, UNITNO_OBJECT), FALSE);

	if (!DoorID)
		return;

	//进入虫子洞口
	if (D2Client_pPlayUnit->nTxtFileNo == Player_Type_Sorceress)
	{
		SelectSkill(TELEKINESIS);
		WaitToBeReadyToCastSkill(1500);
		BYTE aPacket[9] = { 0x0d };
		*(DWORD*)&aPacket[1] = 0x2;
		*(DWORD*)&aPacket[5] = DoorID;
		D2NET_SendPacket(9, 0, aPacket);
	}
	else
	{
		TP2Coordinate(pt.x, pt.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);
		BYTE packet[9];
		packet[0] = 0x13;
		*(DWORD*)&packet[1] = 2;
		*(DWORD*)&packet[5] = DoorID;
		D2NET_SendPacket(9, 0, packet);
	}

	POINT DurPos;
	DWORD DurID;
	WAIT_UNTIL(200, 20, DurID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, DurPos, 0xd3, UNITNO_MONSTER), TRUE);

	KillMonster(DurID, 15, 15, FALSE);	
}

void KillMorph(StartLocation& s)
{
	s = From_Unknown;
	TP2Coordinate(0x44b5, 0x1f86, 8, MAX_DIST_FOR_SINGLE_TP, 10);
	POINT BossPos;
	DWORD BossID;
	WAIT_UNTIL(200, 20, BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 242, UNITNO_MONSTER), FALSE);
	if (!BossID)
		return;

	UpdateNPCPos(BossID, BossPos);
	TP2Coordinate(BossPos.x + 10, BossPos.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);

	KillMonster(BossID, 25, 25, FALSE);

	if (g_Game_Config.bClearMonsterNearby)
	{
		//杀墨菲斯托旁边的三个议会成员
		TP2Coordinate(0x44b7, 0x1faf, 8, MAX_DIST_FOR_SINGLE_TP, 3);
		WAIT_UNTIL(200, 6, BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 347, UNITNO_MONSTER, TRUE, 31), FALSE);
		if (BossID)
		{
			UpdateNPCPos(BossID, BossPos);
			TP2Coordinate(BossPos.x + 10, BossPos.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);
			KillMonster(BossID, 15 ,15, FALSE);
		}
		TP2Coordinate(0x44e2, 0x1f85, 8, MAX_DIST_FOR_SINGLE_TP, 3);
		WAIT_UNTIL(200, 6, BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 347, UNITNO_MONSTER, TRUE, 28), FALSE);
		if (BossID)
		{
			UpdateNPCPos(BossID, BossPos);
			TP2Coordinate(BossPos.x - 10, BossPos.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);

			KillMonster(BossID, 15, 15, FALSE);
		}
		TP2Coordinate(0x44cb, 0x1f5a, 8, MAX_DIST_FOR_SINGLE_TP, 3);
		WAIT_UNTIL(200, 6, BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 346, UNITNO_MONSTER, TRUE), FALSE);
		if (BossID)
		{
			UpdateNPCPos(BossID, BossPos);
			TP2Coordinate(BossPos.x + 10, BossPos.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);

			KillMonster(BossID, 15, 15, FALSE);
		}
	}
	s = From_Unknown;
}

void KillAoCao(StartLocation& s)
{
	POINT BossPos;
	DWORD BossID;
	WAIT_UNTIL(200, 20, BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, BossPos, 0x01BB, UNITNO_MONSTER), FALSE);
	if (!BossID)
		return;

	UpdateNPCPos(BossID, BossPos);
	TP2Coordinate(BossPos.x, BossPos.y + 15, 5, MAX_DIST_FOR_SINGLE_TP, 4);
	KillMonster(BossID, 10, 0, FALSE);
	s = From_Unknown;
}

BOOL GotoRedDoor(StartLocation& s, int RelativeX = 0, int RelativeY = 0)
{
	switch (s)
	{
	case From_Door:
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
	GotoCoordinate(0x1407, 0x1400);

	//到红门附近才会有红门的数据
	POINT pt;
	if (!FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x3c, UNITNO_OBJECT) || !GotoCoordinate(pt.x + RelativeX, pt.y + RelativeY))
		return FALSE;
	return TRUE;
}

void KillPindle(StartLocation& s)
{
	GoTakeWP(s, 0x6d);
	GotoRedDoor(s);
	//进红门
	s = From_Unknown;
	short retry = 0;
	DrlgLevel *lv;
	while ((lv = GetUnitDrlgLevel(D2Client_pPlayUnit)) && (lv->nLevelNo != 0x79))
	{
		POINT pt;
		DWORD redDoorID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x3c, UNITNO_OBJECT);
		if (redDoorID)
		{
			BYTE packet[9] = { 0x13 };
			*(DWORD*)&packet[1] = 2;
			*(DWORD*)&packet[5] = redDoorID;
			D2NET_SendPacket(9, 0, packet);
			WAIT_UNTIL(400, 10, (lv = GetUnitDrlgLevel(D2Client_pPlayUnit)) && (lv->nLevelNo == 0x79), FALSE);
		}
		if (retry++ > 1)
			break;
	}

	lv = GetUnitDrlgLevel(D2Client_pPlayUnit);
	if (lv->nLevelNo != 0x79)
		return;

	SelectSkill(g_Game_Config.DefendSkill);
	Sleep(100);
	WaitToBeReadyToCastSkill(1500);
	BYTE packet[5] = { 0xc };
	*(WORD*)&packet[1] = (WORD)(D2Client_pPlayUnit->pPos->xpos1 >> 16);
	*(WORD*)&packet[3] = (WORD)(D2Client_pPlayUnit->pPos->ypos1 >> 16);
	D2NET_SendPacket(5, 0, packet);
	Sleep(300);

	TP2Coordinate(0x274f, 0x33c0, 5, MAX_DIST_FOR_SINGLE_TP, 10);

	DWORD PindleID;
	POINT PindlePos;
	WAIT_UNTIL(250, 10, PindleID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, PindlePos, 0x1b8, UNITNO_MONSTER, TRUE), FALSE);
		
	if(!PindleID)
		return;

	UpdateNPCPos(PindleID, PindlePos);
	TP2Coordinate(PindlePos.x, PindlePos.y + 20, 2, MAX_DIST_FOR_SINGLE_TP, 5);

	DWORD MonstersGUID[32], NumOfMonsters;
	if (g_Game_Config.bClearMonsterNearby)
	GetCurrentRoom1Monsters(D2CLIENT_GetUnitFromId(PindleID, UNITNO_MONSTER)->pPos->pRoom1,
		MonstersGUID, SIZEOFARRAY(MonstersGUID), NumOfMonsters);
	//开始kp
	KillMonster(PindleID, 0, 20, FALSE, 10, TRUE, FALSE);
	if (IsMonsterKilled(PindleID))
	{
		if (g_Game_Config.bClearMonsterNearby)
		{
			for (DWORD i = 0; i < NumOfMonsters; ++i)
			{
				if (!IsMonsterKilled(MonstersGUID[i]))
					KillMonster(MonstersGUID[i], 0, 15, FALSE, 10, FALSE, FALSE);
			}
		}

		SearchItemsWantedNearby(D2Client_pPlayUnit->pPos->pRoom1);
		PickItem();
		Sleep(200);
		SearchItemsWantedNearby(D2Client_pPlayUnit->pPos->pRoom1);
		PickItem();
	}
	s = From_Unknown;
}

void ShopBotAnya(StartLocation& s, WORD NPCTxtID)
{
	DWORD ShopCount = 100, CurCount = 0;
	//shop 安雅

	GoTakeWP(s, 0x6d);
	GotoRedDoor(s, -5, 10);
	s = From_Unknown;

	DWORD AnyaID;
	POINT AnyaPos;
	DrlgLevel *lv;
	POINT pt;
	DWORD redDoorID;
	BOOL FirstTime = TRUE;
	while (CurCount++ <= ShopCount)
	{
		AnyaID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, AnyaPos, NPCTxtID, UNITNO_MONSTER);
		if (!AnyaID)
			return;
		GotoCoordinate(AnyaPos.x, AnyaPos.y);
		ShopItem(AnyaID);

		redDoorID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x3c, UNITNO_OBJECT);
		if (!redDoorID)
			return;

		GotoCoordinate(pt.x, pt.y);
		//进红门	
		BYTE packet[9] = { 0x13 };
		*(DWORD*)&packet[1] = 2;
		*(DWORD*)&packet[5] = redDoorID;
		D2NET_SendPacket(9, 0, packet);

		if (FirstTime)
		{
			Sleep(2500);
			FirstTime = FALSE;
		}
		WAIT_UNTIL(200, 5, (lv = GetUnitDrlgLevel(D2Client_pPlayUnit)) && (lv->nLevelNo == 0x79), TRUE);

		redDoorID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 0x3c, UNITNO_OBJECT);
		if (!redDoorID)
			return;

		//GotoCoordinate(pt.x, pt.y);
		packet[0] = { 0x13 };
		*(DWORD*)&packet[1] = 2;
		*(DWORD*)&packet[5] = redDoorID;
		D2NET_SendPacket(9, 0, packet);
		WAIT_UNTIL(200, 5, (lv = GetUnitDrlgLevel(D2Client_pPlayUnit)) && (lv->nLevelNo == 0x6d), TRUE);
		SendHackMessage(D2RPY_PING, "");
	}
}

void ShopBotZhuo(StartLocation& s)
{
	DWORD ShopTime = 55*60, CurCount = 0;
	//shop 卓格南

	time_t tShopStartTime = time(0);

	GoTakeWP(s, 0x28);	//到act2城内的door

	DrlgRoom2 *pRoom1, *pRoom2;
	if (!FindRoom2BetweenLevels(40, 41, pRoom1, pRoom2, 285))
		return;

	if ((pRoom1->xPos != 0x400) || (pRoom1->yPos != 0x3e8))
		return; //地图不对重建游戏

	if (!GotoTradeNPC(s))
		return;

	time_t tTimeNow = time(0);
	while ((tTimeNow - tShopStartTime) < ShopTime)
	{
		POINT pt;
		ShopItem(GetTradeNPCID_Pos(pt));

		GotoCoordinate(0x13e8, 0x1395, 35, FALSE);
		GotoCoordinate(0x13e8, 0x1380, 35, FALSE);

		WAIT_UNTIL(100, 10, !TestPlayerInTown(D2Client_pPlayUnit), TRUE);

		GotoCoordinate(0x13e8, 0x1395, 35, FALSE);

		WAIT_UNTIL(100, 10, TestPlayerInTown(D2Client_pPlayUnit), TRUE);

		//走到卓格南附近
		POINT NPCPos;
		DWORD ID = GetTradeNPCID_Pos(NPCPos);
		WAIT_UNTIL(200, 15, ID = GetTradeNPCID_Pos(NPCPos), FALSE);

		//走到NPC 附近
		if (!GotoCoordinate(NPCPos.x, NPCPos.y))
			return;

		UpdateNPCPos(ID, NPCPos);
		//走到NPC 附近
		if (!GotoCoordinate(NPCPos.x, NPCPos.y))
			return;

		SendHackMessage(D2RPY_PING, "");
		tTimeNow = time(0);
	}
}

DWORD WINAPI HackBotProc(LPVOID lpParam)
{
	static WORD nErrNum = 0;
	__try {
		g_Game_Data.bLevelToBeInitialized = TRUE;
		WAIT_UNTIL(50, 100, !g_Game_Data.bLevelToBeInitialized, TRUE);

		CheckCorpseAndCleanupInventory();
		StartLocation s = From_StartPoint;
		PrepareBeforeFight(s);
		GotoDeositMoneyAndGamble(s);

		DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
		if (!g_Game_Data.MercGUID && g_Game_Config.Use_Merc)
			GotoResurMerc(s);

		char *next_token = NULL;
		char tmpTargetBoss[128];
		strcpy_s(tmpTargetBoss, sizeof(tmpTargetBoss), g_Game_Config.TargetBoss);
		char *pBoss = strtok_s(tmpTargetBoss, ",", &next_token);
		while (pBoss != NULL) {
			if (strstr(pBoss, "安达利尔"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				WORD lvls[] = { 0x23, 0x24, 0x25 };
				if (GotoActLevelFromWP(s, lvls, SIZEOFARRAY(lvls)))
					KillAndy(s);				
			}
			else if (strstr(pBoss, "督瑞尔"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				GoTakeWP(s, 0x2e);
				if (GotoFindAndTakeEntrance((WORD)D2CLIENT_pDrlgAct->pDrlgMisc->nStaffTombLvl))
					KillA2Boss(s);			
			}
			else if (strstr(pBoss, "暴躁外皮"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				KillPindle(s);
			}
			else if (strstr(pBoss, "女伯爵"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				WORD lvlss[] = { 6, 20,21,22,23,24,25 };
				if (GotoActLevelFromWP(s, lvlss, SIZEOFARRAY(lvlss)))
					KillCountress(s);				
			}
			else if (strstr(pBoss, "召唤者"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				WORD lvls[] = { 0x4a };
				if (GotoActLevelFromWP(s, lvls, SIZEOFARRAY(lvls)))
					KillSummoner(s);
			}
			else if (strstr(pBoss, "尼拉塞克"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				KillPindle(s);
				short nCurLv = 0;
				WORD lvls[] = { 0x7a, 0x7b, 0x7c };
				while (nCurLv < SIZEOFARRAY(lvls))
				{
					if (!GotoFindAndTakeEntrance(lvls[nCurLv]))
						return 0;
					nCurLv++;
				}
				KillNil(s);
			}
			else if (strstr(pBoss, "墨菲斯托"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				WORD lvlss[] = { 101, 102 };
				if (GotoActLevelFromWP(s, lvlss, SIZEOFARRAY(lvlss)))
					KillMorph(s);
			}
			else if (strstr(pBoss, "地穴"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}

				if (!GoTakeWP(s, 6))
					return 0;

				s = From_Unknown;
				CastDefendSkill();
				DrlgRoom2 *pRoom1, *pRoom2;
				if (!FindRoom2BetweenLevels(6, 7, pRoom1, pRoom2, 0x25) ||
					!FindPathAndTP2DestRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoom1))
					return 0;

				TP2Coordinate(pRoom2->xPos * 5 + pRoom2->xPos1 * 5 / 2, pRoom2->yPos * 5 + pRoom2->yPos1 * 5 / 2, 5, MAX_DIST_FOR_SINGLE_TP, 2);

				if (GetUnitDrlgLevel(D2Client_pPlayUnit)->nLevelNo != 7)	//泰摩高地
					return 0;

				//TP并进入地穴第一层 
				if (GotoFindAndTakeEntrance(12))
					ClearPit();
			}
			else if (strstr(pBoss, "古代通道"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}

				WORD lvlss[] = { 44, 65 };
				if (GotoActLevelFromWP(s, lvlss, SIZEOFARRAY(lvlss)))
					ClearLevel(65);
			}
			else if (strstr(pBoss, "剥壳凹槽"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				WORD lvlss[] = { 112 };
				if (GotoActLevelFromWP(s, lvlss, SIZEOFARRAY(lvlss)))
				{
					DrlgRoom2 *pRoomTo;
					POINT pt;
					if (GetEntrancePosAndRoom2(112, 113, pt, pRoomTo) &&
						FindPathAndTP2DestRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoomTo))
					{
						KillAoCao(s);
					}
				}
			}
			else if (strstr(pBoss, "安雅"))	//shop 安雅
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				ShopBotAnya(s, 512);
			}
			else if (strstr(pBoss, "卓格南"))	//shop 卓格南 非资料片用
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				ShopBotZhuo(s);
			}
			else if (strstr(pBoss, "毁灭的王座"))
			{
				if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
				{
					BackToTown(s);
					PrepareBeforeFight(s);
				}
				WORD lvlss[] = { 129, 130, 131 };
				if (GotoActLevelFromWP(s, lvlss, SIZEOFARRAY(lvlss)))
					KillBarr(s);
			}
			pBoss = strtok_s(NULL, ",", &next_token);
			nErrNum = 0;
		}
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		nErrNum++;
		SendHackMessage(D2RPY_LOG, "Mem Exception in HackBotProc, Close Current Game !");
		if (nErrNum >= 3)
		{
			SendHackMessage(D2RPY_ERROR_NEED_RESTART, "连续出现错误，重启客户端！");
		}
	}
	CloseGame();
	return 0;
}


void StartRunBot()
{
	DWORD dwThreadID, GameNuum = 0;
	char gamename[15];

	srand((int)time(0));
	for (int i = 0; i < 4000; ++i)
	{
		if (g_Game_Config.RandomGameName)
		{
			char RandChar[6];
			RandChar[0] = rand() % 25 + 'a';
			RandChar[1] = rand() % 25 + 'a';
			RandChar[2] = rand() % 25 + 'a';
			RandChar[3] = rand() % 25 + 'a';
			RandChar[4] = rand() % 25 + 'a';
			RandChar[5] = '\0';
			sprintf_s(gamename, sizeof(gamename), "%s", RandChar);
		}
		else
			sprintf_s(gamename, sizeof(gamename), "%s%d", g_Game_Config.Game_Name, GameNuum++);
		if (!CreateGame(gamename, g_Game_Config.Game_Difficulty))
			continue;

		g_hRunThreadHandle = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			HackBotProc,        // name of the thread function
			NULL,              // no thread parameters
			0,                 // default startup flags
			&dwThreadID);
		if (g_hRunThreadHandle == NULL)
			return;
		WaitForSingleObject(
			g_hRunThreadHandle,
			INFINITE);
		CloseHandle(g_hRunThreadHandle);

		WAIT_UNTIL(1000, 30, WhereAmI(D2GFX_GetHwnd()) == D2_LOC_LOBBY, FALSE);
	}
}