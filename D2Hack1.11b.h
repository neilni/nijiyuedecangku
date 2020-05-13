#pragma once

#include "ShareData.h"
#include "D2Struct.h"

#ifdef D2HACK111B_EXPORTS
#define D2HACK111B_API __declspec(dllexport)
#else
#define D2HACK111B_API __declspec(dllimport)
#endif

void SendPacketHook_ASM();
void PacketReceivedInterceptHook_ASM();
void GameEndPatch_ASM();
void GameLoopPatch_ASM();
void PermShowItemsPatch_ASM();
void PermShowItemsPatch2_ASM();
void KeydownPatch_ASM();
void ItemNamePatch_ASM();
void WeatherPatch_ASM();
void LightingPatch_ASM();
DWORD __fastcall D2CLIENT_GetUnitFromId_STUB(DWORD unitid, DWORD unittype);
void __stdcall ShakeScreenPatch(DWORD *xpos, DWORD *ypos);
void __fastcall PacketReceivedInterceptHook(BYTE* aPacket, DWORD aLength);
DWORD __fastcall D2CLIENT_GetUiVar_STUB(DWORD varno);
BOOL PermShowItemsPatch();
BOOL PermShowItemsFix();
void __fastcall KeydownPatch(BYTE keycode, BYTE repeat);
void __fastcall ItemNamePatch(wchar_t *name, UnitAny *item);
void __fastcall NextGameNamePatch(D2EditBox* box, BOOL(__stdcall *FunCallBack)(D2EditBox*, DWORD, DWORD));

struct HK_GAME_INFO
{
	struct
	{
		DWORD PotionGUID;
		//DWORD PotionType;
	}Potions[16];	//Player身上的药水，按顺序保存;

	struct
	{
		DWORD id;
		WORD x;
		WORD y;
		DWORD type;	//药水还是物品
	}ItemToPickup[256];
	WORD ItemToPickupNum;
	BOOL bBackToTown;
	BOOL bPickupCorpse;
	DWORD CorpseGUID;
	DWORD MercGUID;	//佣兵的id

	BOOL bPlayerPoisoned;	//是否是中毒状态 

	//Temp Golbal Vars
	BOOL bBOT_TitleDisplay;
	//BOOL b0x15PacketReceived;
	DWORD nGambleItemID;

	BOOL bLevelToBeInitialized;

	BOOL bCloseGameFlag;
	BOOL bGameClosed;
};

typedef int __stdcall _D2COMMON_GetUnitStat_Fun(UnitAny*, DWORD, DWORD);
extern _D2COMMON_GetUnitStat_Fun *D2COMMON_GetUnitStat;
typedef DWORD __stdcall _D2COMMON_GetItemFlag_Fun(UnitAny *item, DWORD flagmask, DWORD lineno, char *filename);
extern _D2COMMON_GetItemFlag_Fun *D2COMMON_GetItemFlag;
extern const short MAX_DIST_FOR_SINGLE_TP;

struct PickupItemsConfig
{
	enum Operator {
		Undefined = 0,
		GreaterOrEqual = 1,
		LessOrEqual,
		Equal,
		Greater,
		Less
	};
	enum MatchResult {
		NotMatch = 0,
		Match = 1,
		NeedIdentify = 2
	};

	struct ItemConfigAttr {
		BYTE index;
		int value;
		char op;
		ItemConfigAttr* pNextAttr;
	};
	struct ItemConfigDiary
	{
		const char* pItemNameStr;
		int Param;
		MatchResult (*pFuncIsItemAttrMatch)(UnitAny* item, int Param, const ItemConfigAttr*);
	};

	static inline BOOL ItemAttrMatch(int a, Operator op, int b)
	{
		switch (op)
		{
		case GreaterOrEqual:
			return a >= b;
		case Greater:
			return a > b;
		case LessOrEqual:
			return a <= b;
		case Less:
			return a < b;
		case Equal:
			return a == b;
		}
		return FALSE;
	}
	static inline MatchResult _pFuncIsUnitStatMatch(UnitAny* unit, int Param, const ItemConfigAttr* pConfig) {
		//if (!(item->pItemData->nFlag & ITEMFLAG_UNIDENTIFIED))	//未辨识的物品
		return D2COMMON_GetItemFlag(unit, ITEMFLAG_IDENTIFIED, 0, "?") ?
			(ItemAttrMatch(D2COMMON_GetUnitStat(unit, Param, 0), (Operator)pConfig->op, pConfig->value) ? Match : NotMatch) :
			NeedIdentify;
	}
	static inline MatchResult _pFuncIsUnitStatMatch2(UnitAny* unit, int Param, const ItemConfigAttr* pConfig) {
		return ItemAttrMatch(D2COMMON_GetUnitStat(unit, Param, 0), (Operator)pConfig->op, pConfig->value) ? Match : NotMatch;
	}
	static inline MatchResult _pFuncIsItemLevelMatch(UnitAny* unit, int , const ItemConfigAttr* pConfig) {
		return ItemAttrMatch(unit->pItemData->nItemLevel, (Operator)pConfig->op, pConfig->value) ? Match : NotMatch;
	}
	/*static inline BOOL _pFuncIsUnitToBlockMatch(UnitAny* unit, int Param, const ItemConfigAttr* pConfig) {
		int toblock = D2COMMON_GetUnitStat(unit, Param, 0);
		//盾牌的block不同char查看不一样
		extern UnitAny** p_D2CLIENT_PlayerUnit;
		switch ((*p_D2CLIENT_PlayerUnit)->nTxtFileNo)
		{
		case Player_Type_Sorceress:
		case Player_Type_Necromancer:
		case Player_Type_Druid:
			toblock += 20;
			break;
		case Player_Type_Amazon:
		case Player_Type_Assassin:
		case Player_Type_Barbarian:
			toblock += 25;
			break;
		case Player_Type_Paladin:
			toblock += 30;
			break;
		}
		return  toblock >= ConfigValue;
	}*/
	static inline MatchResult _pFuncIsUnitLifeManaMatch(UnitAny* unit, int Param, const ItemConfigAttr* pConfig) {
		return D2COMMON_GetItemFlag(unit, ITEMFLAG_IDENTIFIED, 0, "?") ?
			(ItemAttrMatch((D2COMMON_GetUnitStat(unit, Param, 0) >> 8), (Operator)pConfig->op, pConfig->value) ? Match : NotMatch) : NeedIdentify;
	}
	static inline MatchResult _pFuncIsUnitFlagMatch(UnitAny* unit, int Param, const ItemConfigAttr* pConfig) {
		//此类目前只有eht一种
		DWORD flg = D2COMMON_GetItemFlag(unit, Param, 0, "?");
		if (flg)
			flg = 1;
		return ItemAttrMatch(flg, (Operator)pConfig->op, pConfig->value) ? Match : NotMatch;
	}
	static inline MatchResult _pFuncIsUnitSkillTabMatch(UnitAny* unit, int Param, const ItemConfigAttr* pConfig) {
		return D2COMMON_GetItemFlag(unit, ITEMFLAG_IDENTIFIED, 0, "?") ?
			(ItemAttrMatch(D2COMMON_GetUnitStat(unit, STAT_ADD_SKILL_TAB, Param), (Operator)pConfig->op, pConfig->value) ? Match : NotMatch) : NeedIdentify;
	}
	static inline MatchResult _pFuncIsUnitSingleSkillMatch(UnitAny* unit, int Param, const ItemConfigAttr* pConfig) {
		return D2COMMON_GetItemFlag(unit, ITEMFLAG_IDENTIFIED, 0, "?") ?
			(ItemAttrMatch(D2COMMON_GetUnitStat(unit, Param, (pConfig->value & 0xFFFF0000) >> 16),
				(Operator)pConfig->op, pConfig->value & 0xFFFF) ? Match : NotMatch) : NeedIdentify;
	}	
	static inline MatchResult _pFuncIsUnitClassSkillMatch(UnitAny* unit, int Param, const ItemConfigAttr* pConfig) {
		return D2COMMON_GetItemFlag(unit, ITEMFLAG_IDENTIFIED, 0, "?") ?
			(ItemAttrMatch(D2COMMON_GetUnitStat(unit, STAT_ADD_CLASS_SKILLS, Param), (Operator)pConfig->op, pConfig->value) ? Match : NotMatch) : NeedIdentify;
	}

	static const ItemConfigDiary ItemConfigTable[49];
	void erase()
	{
		auto p = pFirstAttr;
		while (p)
		{
			auto q = p;
			p = p->pNextAttr;
			delete q;
		}
		pFirstAttr = NULL;
		Code = Quality = 0;
	}
	PickupItemsConfig() :Code(0), Quality(0), pFirstAttr(NULL) {};
	~PickupItemsConfig()
	{
		erase();
	}

	static void swap(PickupItemsConfig& A, PickupItemsConfig& B)
	{
		auto tmp = A.Code;
		auto tmp2 = A.Quality;
		auto tmp3 = A.pFirstAttr;
		A.Code = B.Code;
		A.Quality = B.Quality;
		A.pFirstAttr = B.pFirstAttr;
		B.Code = tmp;
		B.Quality = tmp2;
		B.pFirstAttr = tmp3;
	}

	static char FindAttr(const char* AttrName)
	{
		for (int i = 0; i < SIZEOFARRAY(ItemConfigTable); ++i)
		{
			if (!strcmp(AttrName, ItemConfigTable[i].pItemNameStr))
				return i;
		}
		return -1;
	}

	short Code;
	BYTE Quality;
	ItemConfigAttr* pFirstAttr;
};

#define MAX_ITEM_CONFIG_NUM 256
struct HK_Game_Config
{
	char Acc_Name[15];
	char Acc_Pass[20];
	BYTE Acc_Pos;
	BYTE PrimarySkill;
	BYTE SecondSkill;
	BYTE DefendSkill;
	BYTE AssistSkill;	//辅助技能比如pal专注
	BYTE PrimarySkillHand;
	BYTE SecondSkillHand;
	BYTE AssistSkillHand;
	WORD PrimarySkillDelay;
	WORD SecondSkillDelay;
	BYTE ReorganizeItem;	//是否整理装备
	BYTE ReorganizeItemPosStartCol;
	BYTE ReorganizeItemPosEndCol;

	char RandomGameName;
	char Game_Name[10];
	short Game_Difficulty;
	short Use_Merc;	//是否使用佣兵
	short ExitWhenMonsterImm;

	DWORD MinimumMoneyToStartGamble;
	DWORD KeepMoneyAfterGamble;

	char TargetBoss[128];
	char bClearMonsterNearby;
	short GambleItems[16];
	char GambleItemsNum;

	//Cube
	char bEnableCubeGem;
	char bEnableCubeGrandCharm;
	
	PickupItemsConfig ItemsConfigForBot[MAX_ITEM_CONFIG_NUM];
	short ItemsConfigNum;

	//map
	BOOL bPermShowConfig;
	BOOL bWeatherEffect;
};

enum Player_Skill {
	TELEPORT = 0x36,
	FROZEN_ORB = 0x40,
	GLACIAL_SPIKE = 0x37,
	BLIZZARD = 0x3b,
	TELEKINESIS = 0x2B,
	STATIC_FIELD = 0x2A,
	FROZEN_ARMOR = 0x28,
};

extern HK_GAME_INFO g_Game_Data;
extern HK_Game_Config g_Game_Config;
extern HANDLE g_hRunThreadHandle;

//////////////////////////////////////////////////////////////
const enum D2_Game_Location { D2_LOC_UNKNOWN, D2_LOC_LOGIN, D2_LOC_ACC_INPUT, D2_LOC_CHAR_SEL, D2_LOC_LOBBY, D2_LOC_PENDING };
const enum StartLocation{	From_StartPoint,	From_Door, From_TradeNPC,	From_HealerNPC, From_WP, From_Unknown};

BOOL Login();
BOOL CreateGame(const char* ,int);
void CloseGame();
D2_Game_Location WhereAmI(HWND hD2win);
BOOL CheckLifeAndMana(BOOL *bNeedHeal);
BOOL GotoCoordinate(int DestX, int DestY, short MaxStepCount = 35, BOOL bBackToTown = TRUE);
DWORD GetCurrentAct(DrlgLevel* currlvl);
DrlgLevel *GetUnitDrlgLevel(UnitAny *unit);
BOOL GetEntrancePosAndRoom2(WORD lvfrom, WORD lvto, POINT& pt, DrlgRoom2* &retroom2);

void SelectSkill(int skill, BYTE hand = 2);
void CastSkillToMonster(WORD x, WORD y, BYTE hand);
void CastSkillToMonster(DWORD id, BYTE hand);
void TP2Coordinate(int destx, int desty, int HowNear, int max_dist_perstep, int maxtpnum, int randomstep = 3);
void PickItem();

DrlgLevel* __fastcall GetDrlgLevel(DrlgMisc *drlgmisc, DWORD levelno);
BOOL TakeWP(DWORD wpID, DWORD, DWORD wpLvl);
void BackToTown(StartLocation& s);
void BackToTownToBuyPotions(WORD nFromLevel);
BOOL TestPlayerInTown(UnitAny *unit);
BOOL GoTakeWP(StartLocation& s, DWORD DestWP);
BOOL GotoHealNPC(StartLocation& s);
void GotoWaypoint(StartLocation& s);
BOOL GotoGetHeal(StartLocation& s);
void CheckPlayerStatus(BOOL &bNeedBuyBlood, BOOL& bNeedBuyMana, BOOL& bHasUnidentifiedInventoryItems, BOOL& bNeedRepair, BOOL& bBuyTownScrol);
BOOL GotoRepairItems(StartLocation& s);
BOOL GotoIdentifyAndBuyPotions(StartLocation& s);
BOOL GotoTradeNPC(StartLocation& s);
BOOL GotoResurMerc(StartLocation& s);
BOOL GotoDoor(StartLocation& s);
void UpdateNPCPos(DWORD NPCGUID, POINT& pt);
BOOL IsMonsterImmune(UnitAny* monster, UnitStat s);
void UpdateBeltPotionPos();
void CleanupInventory();
void CheckCorpseAndCleanupInventory();
BOOL GotoActLevelFromWP(StartLocation& s, WORD *LvlArr, short nLvlNum);
BOOL GotoFindAndTakeEntrance(WORD ToLvl);
DWORD FindUnitGUIDFromPos(DrlgRoom1* room, const POINT& UnitPos);
DWORD FindUnitGUIDPosFromTxtFileNo(DrlgRoom1* room, POINT& UnitPos, const DWORD nTxtFileNo, UnitNo unittype, BOOL bOnlyFindBoss = FALSE, WORD UniqueMonsterID = 0);
BOOL GetLevelPresetObj(WORD nLevelNo, POINT& pt, DrlgRoom2** pRoom2, const char* strObjName, DWORD ObjNO);
void RevealCurrentLevelMap();
void InitCurrentLevelDrlgRoom();
BOOL IsMonsterKilled(DWORD MonsterID);
void GetCurrentRoom1Monsters(const DrlgRoom1* room1, DWORD* MonsterGUIDArray, const DWORD NumOfArray, DWORD& NumOfMonster);
void GetCurrentRoom1UnhandledMonsters(const DrlgRoom1* room1, DWORD* MonsterGUIDArray, const DWORD NumOfArray, DWORD& NumOfMonster);
void SearchItemsWantedNearby(const DrlgRoom1* room);
void PrepareBeforeFight(StartLocation& s);
void GotoDeositMoneyAndGamble(StartLocation& s);
DWORD IsMonsterNearby();
void GetMonstersNearby(DWORD* MonsterGUIDArray, const DWORD NumOfArray, DWORD& NumOfMonster);
BOOL FindPathAndTP2DestRoom(const DrlgRoom2 *pRoomFrom, const DrlgRoom2 *pRoomTo);
DWORD LoopCurrentLevelRoom2(WORD nLevelNo, std::function<DWORD(DrlgRoom2*)> pFuncProcRoom2);
BOOL FindRoom2BetweenLevels(WORD nLevelFrom, WORD nLevelTo, DrlgRoom2* &pRoomFrom, DrlgRoom2* &pRoomTo, DWORD ObjID);
void CastDefendSkill();
void ShopItem(DWORD NPCID);
void ClearRoomMonsters(DrlgRoom1* Room, int DisFromMonster = 10);
BOOL FindAndClearPathToDestRoom(const DrlgRoom2 *pRoomFrom, const DrlgRoom2 *pRoomTo);
void InteractObject(DWORD nTxtFileNO);
void GotoReorgnizeInventoryItems(StartLocation& s);
void GetItemSize(UnitAny* item, WORD& width, WORD& height, DWORD nLocation);
BOOL FindPosToStoreItemToBank(WORD width, WORD height, WORD& PosX, WORD& PosY);
BOOL GotoBank(StartLocation& s);
void CloseMenu(UIVar ui);
void DepositItem(DWORD ItemID, WORD ToPosX, WORD ToPosY);
void GetItemName(UnitAny* item, char* Name);
DWORD GetTradeNPCID_Pos(POINT& pt);
void WaitToBeReadyToCastSkill(int MaxWaitTime);
//////////////////////////////////////////////////////////////

#ifdef _DEBUG
void PrintCurrentLevelPresetUnit(DWORD);
#endif