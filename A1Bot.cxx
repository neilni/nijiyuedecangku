#include "stdafx.h"
#include "misc.h"
#include "D2PtrHookHeader.h"
#include "D2Hack1.11b.h"

void KillAndy(StartLocation& s);
void KillA2Boss(StartLocation& s);
void KillCountress(StartLocation& s);
void KillSummoner(StartLocation& s);
void KillPindle(StartLocation& s);
void KillNil(StartLocation& s);
void SearchItemsWantedNearby(const DrlgRoom1* room);
void KillMorph(StartLocation& s);
void GotoDeositMoneyAndGamble(StartLocation& s);
#ifdef _DEBUG
void PrintCurrentLevelPresetUnit(DWORD UnitType);
void PrintMonstersAround();
BOOL PrintPresetUnit(DrlgRoom2* room2, DWORD UnitType);
void PrintCurrentActLevels();
#endif
void KillBarr(StartLocation& s);
BOOL FindRoom2BetweenLevels(WORD nLevelFrom, WORD nLevelTo, DrlgRoom2* &pRoomFrom, DrlgRoom2* &pRoomTo, DWORD ObjID);

void ClearLevel(WORD);
void ClearPit();
BOOL ReadyToCubeGem(DWORD& nCubeItemCode, DWORD& CubeID);
void GotoCubeGem(StartLocation& s, DWORD ItemCode, DWORD CubeID);
void GotoCubeGrandCharm(StartLocation& s);
BOOL ReadyToCubeGrandCharm(DWORD& Gem1, DWORD& Gem2, DWORD& Gem3, DWORD& GrandCharm, DWORD& CubeID);

DWORD WINAPI GoKAct1Boss(LPVOID lpParam)
{
	/*void PrintCurrentActLevels();
	PrintCurrentActLevels();
	return 0;*/	

	__try {

		StartLocation s = From_StartPoint;

#ifdef _DEBUG

		if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
		{
			BackToTown(s);
			PrepareBeforeFight(s);
		}
		KillPindle(s);

		if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
		{
			BackToTown(s);
			PrepareBeforeFight(s);
		}
		WORD lvlss[] = { 101, 102 };
		if (GotoActLevelFromWP(s, lvlss, SIZEOFARRAY(lvlss)))
			KillMorph(s);

		

#endif

		return 0;

		CheckCorpseAndCleanupInventory();		
		PrepareBeforeFight(s);

		DWORD curact = GetCurrentAct(GetUnitDrlgLevel(D2Client_pPlayUnit));
		if (!g_Game_Data.MercGUID && g_Game_Config.Use_Merc && !GotoResurMerc(s))
			return FALSE;
		
		{
			if (!TestPlayerInTown(D2Client_pPlayUnit) && (s != From_WP))
			{
				BackToTown(s);
				PrepareBeforeFight(s);
			}

			WORD lvlss[] = { 107 };
			GotoActLevelFromWP(s, lvlss, SIZEOFARRAY(lvlss));	//下到火焰之河小站
			
			DrlgRoom2 *pRoom1, *pRoom2;		
			if (!FindRoom2BetweenLevels(107, 108, pRoom1, pRoom2, 401) ||
				!FindPathAndTP2DestRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoom1))	//查找并TP到混沌庇护所入口
				return 0;

			//5个封印的Preset objID， 混沌大臣(395,396)(306 Boss)，西西(394) (312),  邪魔之王(392,393)(362 Boss) 
			//混沌大臣
			POINT pt;
			DrlgRoom2 *pRoomTo;
			if (!GetLevelPresetObj(108, pt, &pRoomTo, NULL, 395))//封印
				return 0;

			int Retry = 0;
			while ((pRoomTo != D2Client_pPlayUnit->pPos->pRoom1->pRoom2) && (Retry++ <= 5))
			{
				FindAndClearPathToDestRoom(D2Client_pPlayUnit->pPos->pRoom1->pRoom2, pRoomTo);				
				if (!GetLevelPresetObj(108, pt, &pRoomTo, NULL, 395))//封印
					return 0;
			}
			
			int dis;
			if (D2Client_pPlayUnit->nTxtFileNo == Player_Type_Paladin)
				dis = 1;
			else
				dis = 10;
			//开封印
			TP2Coordinate(pt.x, pt.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);
			InteractObject(395);

			ClearRoomMonsters(D2Client_pPlayUnit->pPos->pRoom1, dis);
			if(FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 396, UNITNO_OBJECT))	//找封印
			{
				TP2Coordinate(pt.x, pt.y, 5, MAX_DIST_FOR_SINGLE_TP, 2);
				InteractObject(396);
			}
			
			DWORD BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 306, UNITNO_MONSTER, TRUE);
			WAIT_UNTIL(200, 10, BossID = FindUnitGUIDPosFromTxtFileNo(D2Client_pPlayUnit->pPos->pRoom1, pt, 306, UNITNO_MONSTER, TRUE), TRUE);
			if (BossID)
			{
				BOOL KillMonster(DWORD MonsterID, int KeepDistX, int KeepDistY, BOOL bChangePos/*是否打一下换个方向打*/, short RetryCount = 10, BOOL bSendMsg = TRUE, BOOL bSearch = TRUE);
				KillMonster(BossID, dis, dis, FALSE);
				ClearRoomMonsters(D2Client_pPlayUnit->pPos->pRoom1, dis);
			}

			return 0;
		}
		
		{
			if (!TestPlayerInTown(D2Client_pPlayUnit) && (s == From_Unknown))
				BackToTown(s);
			GoTakeWP(s, 0x2e);
			if (GotoFindAndTakeEntrance((WORD)D2CLIENT_pDrlgAct->pDrlgMisc->nStaffTombLvl))
				KillA2Boss(s);
		}


		{
			if (!TestPlayerInTown(D2Client_pPlayUnit) && (s == From_Unknown))
				BackToTown(s);
			WORD lvlss[] = { 6, 20,21,22,23,24,25 };
			if (GotoActLevelFromWP(s, lvlss, SIZEOFARRAY(lvlss)))
				KillCountress(s);
		}

		{
			if (!TestPlayerInTown(D2Client_pPlayUnit) && (s == From_Unknown))
				BackToTown(s);
			WORD lvls[] = { 0x4a };
			if (GotoActLevelFromWP(s, lvls, SIZEOFARRAY(lvls)))
				KillSummoner(s);
		}

		{
			if (!TestPlayerInTown(D2Client_pPlayUnit) && (s == From_Unknown))
				BackToTown(s);
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


		return 1;
	}
	__except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		return 0;
	}
	return 0;
}

void RunAct1Boss()
{
	DWORD dwThreadID, num = 0;
	char gamename[15];

	for (int i = 0; i < 1; ++i)
	{		
		sprintf_s(gamename, sizeof(gamename), "%s%d", g_Game_Config.Game_Name, num++);
		if (!CreateGame(gamename, g_Game_Config.Game_Difficulty))
			continue;

		g_hRunThreadHandle = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			GoKAct1Boss,        // name of the thread function
			NULL,              // no thread parameters
			0,                 // default startup flags
			&dwThreadID);
		if (g_hRunThreadHandle == NULL)
			return;
		if (WaitForSingleObject(
			g_hRunThreadHandle,
			INFINITE)
			!= WAIT_OBJECT_0)
		{
			TerminateThread(g_hRunThreadHandle, 0);
		}
		CloseHandle(g_hRunThreadHandle);

	}
}