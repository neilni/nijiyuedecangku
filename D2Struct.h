#pragma once

struct PlayerData {
	wchar_t szName[1]; //+00
};

struct MonsterData {
	BYTE _1[0x16];
	struct {
		BYTE fUnk : 1;
		BYTE fNormal : 1;
		BYTE fChamp : 1;
		BYTE fBoss : 1;
		BYTE fMinion : 1;
	}; //+16
	BYTE _1aa;
	wchar_t szName[1]; //+18
	BYTE _1a[2]; // +1a
	BYTE anEnchants[9]; // +1c
	BYTE _2a; // +25
	WORD uniqueno; // +26
	BYTE _2[50 * 4 - 14];
	struct {
		DWORD _3[1];
		WORD uniqueno; //+04
		WORD _4[1];
		DWORD anEnchants[9]; //+08
	} *pEnchants; //+cc
};

struct LevelTxt {
	BYTE _1[0xf5];
	char cszName[40]; //+f5
	char cszName2[40]; //+11d
	char szLvlDesc[41]; //+145
	wchar_t szName[40]; //+16e
	wchar_t szName2[40]; //+16e
	BYTE nObjGrp[8]; // unk
	BYTE nObjPrb[8]; // unk
	BYTE _1b[0x79];
};

struct ItemTxt {
	wchar_t szName2[0x40]; //+00
	union {
		DWORD dwCode; // +80
		char szCode[4];
	};
	BYTE _2[0x70]; // +84
	WORD nLocaleTxtNo; // +f4
	BYTE _2a[0x28]; // +f6
	BYTE nType; // +11e
	BYTE _3[0x0d];
	BYTE fQuest; // +12a
};

struct MonsterTxt {
	BYTE _1[0x6];
	WORD nLocaleTxtNo; // +06
	WORD flag; // +08
	WORD _1a;
	union {
		DWORD flag1; // +0c
		struct {
			BYTE flag1a; // +0c
			BYTE flag1b; // +0d
			BYTE flag1c[2]; // +0e
		};
	};
	BYTE _2[0x22]; // +10
	WORD velocity; // +32
	BYTE _2a[0x52]; // +34
	WORD tcs[3][4]; //+86
	BYTE _2b[0x52]; // +9e
	wchar_t szDescriptor[0x3c]; //+f0
	BYTE _3[0x1a0];
};

struct ObjectTxt {
	char szName[0x40];	// +00
	wchar_t wszName[0x40]; // +40
	BYTE _1[4]; // +c0
	BYTE nSelectable0; //+c4
	BYTE _2[0x87];
	BYTE nOrientation; //+14c
	BYTE _2b[0x19];
	BYTE nSubClass; // +166
	BYTE _3[0x11];
	BYTE nParm0; //+178
	BYTE _4[0x39];
	BYTE nPopulateFn; //+1b2
	BYTE nOperateFn; //+1b3
	BYTE _5[8];
	DWORD nAutoMap; // +1bb
};

struct ObjectData {
	ObjectTxt *pTxtFile; //+00 ??
	union {
		BYTE nShrineNo;
		struct {
			BYTE _1 : 7;
			BYTE fChestLocked : 1;
		};
	}; //+04
};

struct ItemData {
	DWORD nQuality;
	BYTE _1[0x14]; // +04
	DWORD nFlag; // +18
	BYTE _2[0x10]; // +1c
	DWORD nItemLevel; // +2c
	BYTE _3[0x14]; // +30
	BYTE nLocation1; // +44					// location on body = xpos of item (only if on body) (spread)
	BYTE nLocation2; // +45					// 0 = inventory, 4=stash (spread), 0xff on body, 3=合成方块
};

struct DrlgRoom2;
struct DrlgRoom1;
struct DrlgMisc;
struct DrlgLevel;
struct UnitAny;

struct RoomTile {
	DWORD _1;
	DrlgRoom2 *pRoom2; //+04
	RoomTile *pNext; //+08
	DWORD *nNum; //+0c
};

struct D2Seed {
	DWORD d1, d2;
};

struct DrlgAct {
	DWORD _1;
	DrlgRoom1 *pRoom1; //+04
	DrlgMisc *pDrlgMisc; //+08
};

struct DrlgMisc {
	DWORD _1[33];
	DrlgAct *pDrlgAct; //+84
	DWORD nBossTombLvl; //+88
	DWORD _2[248];
	DrlgLevel *pLevelFirst; //+46c
	DWORD _3[2];
	DWORD nStaffTombLvl; // +478
};

struct DrlgLevel { //sizeof(DrlgLevel) = 0x230
	DWORD _1[5];
	DWORD nLevelNo; //+14
	DWORD _1a[120];
	D2Seed seed; //+1f8
	DWORD _2[1];
	DrlgRoom2 *pRoom2First; //+204
	DrlgMisc *pDrlgMisc; //+208
	DWORD _3[8];
	DrlgLevel *pLevelNext; //+22c
};

struct AutomapCell {
	DWORD fSaved; //+00
	WORD nCellNo; //+04
	WORD xPixel; //+06
	WORD yPixel; //+08
	WORD wWeight; //+a = -1, 0, 1
	AutomapCell *pLess; //+0c
	AutomapCell *pMore; //+10
};

struct AutomapLayer {
	DWORD nLayerNo;
	DWORD fSaved;
	AutomapCell *pFloors; //+08
	AutomapCell *pWalls; //+0c
	AutomapCell *pObjects; //+10
	AutomapCell *pExtras; //+14
	AutomapLayer *pNextLayer; // +18
};

struct PresetUnit {
	DWORD _1[2];
	DWORD yPos; //+08
	DWORD nTxtFileNo; //+0c
	DWORD _2[1];
	PresetUnit *pNext; //+1c
	DWORD xPos; //+20
	DWORD nUnitType; //+24
};

struct DrlgRoom2 {
	RoomTile *pRoomTiles; //
	DWORD _1[1];
	DWORD nPresetType; //+8
	DWORD _2[1];
	DWORD nRoomsNear; //+10
	DWORD _2a[2];
	DrlgLevel *pDrlgLevel; //+1c
	DWORD xPos; //+20
	DWORD yPos; //+24
	DWORD xPos1; //+28
	DWORD yPos1; //+2c
	DrlgRoom2 **paRoomsNear; //+30
	PresetUnit *pPresetUnits; //+34
	DrlgRoom2 *pNext; //+38
	DWORD _4[38];
	D2Seed seed; //+d4
	DWORD *nPresetType2No; //+dc
	DWORD _5[2];
	DrlgRoom1 *pRoom1; //+e8
};


struct DrlgRoom1 {
	D2Seed seed; //+00
	DWORD xpos1; //+08
	DWORD ypos1; //+0c
	DWORD xsize1; //+10
	DWORD ysize1; //+14
	DWORD xpos2; //+18
	DWORD ypos2; //+1c
	DWORD xsize2; //+20
	DWORD ysize2; //+24
	DWORD _2[3];
	DrlgRoom1 **paRoomsNear; //+34
	DrlgRoom2 *pRoom2; //+38
	UnitAny *pUnitFirst; //+3c
	DWORD _3[13];
	DrlgRoom1 *pNext; // +74
	DWORD _4;
	DWORD nRoomsNear; //+7c
};

struct ObjectPath {
	DWORD xpos1; //+00
	DWORD ypos1; //+04
	DWORD xpos2; //+08
	DWORD ypos2; //+0c
	WORD targetx; //+10
	WORD targety; //+12
	DWORD _1[2];
	DrlgRoom1 *pRoom1; //+1c
	DWORD _2[14];
	UnitAny *pTargetUnit; //+58
};

struct ItemPath {
	void * ptRoom;				// 0x00
	DWORD _1[2];				// 0x04
	DWORD xpos;				// 0x0C
	DWORD ypos;				// 0x10
	DWORD _2[5];				// 0x14
	BYTE * ptUnk;				// 0x28
	DWORD _3[5];				// 0x2C
};

struct UnitInventory {
	DWORD nFlag; //+00
	void* pGame2; //+04 ptGame+1c
	UnitAny *pUnit;
	DWORD nItem; //+0c
};

struct UnitAny {
	DWORD nUnitType; //+00
	DWORD nTxtFileNo; //+04
	DWORD _1;
	DWORD nUnitId; // +0C
	DWORD eMode; // +10		//可用于判断物品装备在哪里，比如拿在鼠标上时eMode=4
	union {
		PlayerData *pPlayerData; //+14
		MonsterData *pMonsterData; //+14
		ObjectData *pObjectData; //+14
		ItemData *pItemData; //+14
	};
	DWORD _2;
	UnitAny *pNext; //+1c
	WORD _2a0;
	WORD unkNo;
	DWORD _2a[2];
	union {
		ObjectPath *pPos;
		ItemPath *pItemPath;
	};
	//struct {
	//	DWORD xpos1; //+00
	//	DWORD ypos1; //+04
	//	DWORD xpos2; //+08
	//	DWORD ypos2; //+0c
	//	WORD targetx; //+10
	//	WORD targety; //+12
	//	DWORD _1[2];
	//	DrlgRoom1 *pRoom1; //+1c
	//	DWORD _2[14];
	//	UnitAny *pTargetUnit; //+58
	//} *pPos; // +2c
	DWORD _2b[12];
	UnitInventory *pInventory; //+60
	DWORD _3[7];
	UnitAny *pNextPlayer; //+80
	DWORD _4b[4];
	DWORD nOwnerType; // +94
	DWORD nOwnerId; // +98
	BYTE _4c[42];
	BYTE  fFlags0 : 1; //+0c6, new added
	BYTE _4d;
	DWORD _5[8];
	UnitAny* pNextUnit;	//这个应该才是正确的UnitNext的位置，前面的应该有误 comment by Leon
	DWORD _5_1[1];
	DWORD fFlags; //+e8
	DWORD fFlags2; //+ec
	DWORD _6[7];
};

#define ITEMFLAG_ETHEREAL 0x400000		//无形物品
#define ITEMFLAG_IDENTIFIED  0x10	//未辨识物品

enum UnitStat {
	STAT_STRENGTH = 0,	//用于判断装备加多少力量
	STAT_DEX = 2,  //敏捷
	STAT_LIFE = 0x06,
	STAT_MAXLIFE = 0x07,
	STAT_MANA = 0x08,
	STAT_MAXMANA = 0x09,	//需要右移8位,也可以查询装备增加的mana
	STAT_STAMINA = 0x0a,
	STAT_MAXSTAMINA = 0x0b,
	STAT_LEVEL = 0x0c,
	STAT_GOLD = 0x0e,
	STAT_GOLD_IN_BANK = 0x0f,
	STAT_ENHANCED_DAMAGE = 0x12, //enhanced damage ， 对宝石、防具等有效，对武器无效，因为ed对武器其damage有体现，查询武器的damage代替
	STAT_TOBLOCK = 0x14,	//增加格挡率（CTB）
	STAT_DEFENCE = 0x1F,	//防御
	STAT_MAGIC_DAMAGE_REDUCED = 0x23,
	STAT_DAMAGE_REDUCED = 0x24,
	STAT_MAGIC_RESIST = 0x25,
	STAT_FIRE_RESIST = 0x27,
	STAT_LIGHTING_RESIST = 0x29,
	STAT_COLD_RESIST = 0x2B,
	STAT_POSION_RESIST = 0x2D,
	STAT_LIFE_LEECH = 0x3c,
	STAT_MANA_LEECH = 0x3e,
	STAT_QUANTITY = 0x46,		//物品数量，可用于查询回城、辨识卷轴书的数量
	STAT_DURABILITY = 0x48,		//耐久度
	STAT_MAX_DURABILITY = 0x49,
	STAT_EXTRA_GOLD = 0x4f,
	STAT_MAGIC_FIND = 0x50,
	STAT_ADD_CLASS_SKILLS = 0x53,	//增加某一角色技能等级
	STAT_IAS = 0x5d,
	STAT_FRW = 0x60,
	STAT_FHR = 0x63,
	STAT_FBR = 0x66,
	STAT_FCR = 0x69,
	STAT_ADD_SINGLE_SKILL = 0x6B,	//增加某一种技能
	STAT_DAMAGE_TAKEN_GOES_TO_MANA = 0x72,
	STAT_ALL_SKILLS = 0x7F,		//增加+1技能属性
	STAT_CRUSHING_BLOW = 0x88,
	STAT_MANA_AFTER_EACH_KILL = 0x8a,
	STAT_FIRE_ABSORB = 0x8f,
	STAT_LIGHTING_ABSORB = 0x91,
	STAT_COLD_ABSORB = 0x94,
	STAT_ADD_SKILL_TAB = 0xBC,	//增加某一系技能
	STAT_NUMSOCKETS = 0xc2,
	STAT_FIRE_MASTER = 0x149,
	STAT_LIGHT_MASTER = 0x14A,
	STAT_COLD_MASTER = 0x14B,
	STAT_POISON_MASTER = 0x14C,
	STAT_FIRE_PIERCE = 0x14D,
	STAT_LIGHT_PIERCE = 0x14E,
	STAT_COLD_PIERCE = 0x14F,
	STAT_POISON_PIERCE = 0x150,
};

enum UnitNo {
	UNITNO_PLAYER = 0,
	UNITNO_MONSTER = 1,	//Monsters, NPCs, and Mercenaries
	UNITNO_OBJECT = 2,	//Stash, Waypoint, Chests, and other objects.
	UNITNO_MISSILE = 3,
	UNITNO_ITEM = 4,
	UNITNO_ROOMTILE = 5
};

enum PlayerType {
	Player_Type_Amazon = 0,
	Player_Type_Sorceress,
	Player_Type_Necromancer,
	Player_Type_Paladin,
	Player_Type_Barbarian,
	Player_Type_Druid,
	Player_Type_Assassin,
};

enum UIVar {
	UIVAR_UNK0 = 0, // always 1
	UIVAR_INVENTORY = 1, // hotkey 'B'
	UIVAR_STATS = 2, // hotkey 'C'
	UIVAR_CURRSKILL = 3, // left or right hand skill selection
	UIVAR_SKILLS = 4, // hotkey 'T'
	UIVAR_CHATINPUT = 5, // chat with other players, hotkey ENTER
	UIVAR_NEWSTATS = 6, // new stats button
	UIVAR_NEWSKILL = 7, // new skills button
	UIVAR_INTERACT = 8, // interact with NPC
	UIVAR_GAMEMENU = 9, // hotkey ESC
	UIVAR_ATUOMAP = 10, // hotkey TAB
	UIVAR_CFGCTRLS = 11, // config control key combination
	UIVAR_NPCTRADE = 12, // trade, game, repair with NPC
	UIVAR_SHOWITEMS = 13, // hotkey ALT
	UIVAR_MODITEM = 14, // add socket, imbue item
	UIVAR_QUEST = 15, // hotkey 'Q'
	UIVAR_UNK16 = 16,
	UIVAR_NEWQUEST = 17, // quest log button on the bottom left screen
	UIVAR_UNK18 = 18, // lower panel can not redraw when set
	UIVAR_UNK19 = 19, // init 1
	UIVAR_WAYPOINT = 20,
	UIVAR_MINIPANEL = 21,
	UIVAR_PARTY = 22, // hotkey 'P'
	UIVAR_PPLTRADE = 23, // trade, exchange items with other player
	UIVAR_MSGLOG = 24,
	UIVAR_STASH = 25,
	UIVAR_CUBE = 26,
	UIVAR_UNK27 = 27,
	UIVAR_INVENTORY2 = 28,
	UIVAR_INVENTORY3 = 29,
	UIVAR_INVENTORY4 = 30,
	UIVAR_BELT = 31,
	UIVAR_UNK32 = 32,
	UIVAR_HELP = 33, // help scrren, hotkey 'H'
	UIVAR_UNK34 = 34,
	UIVAR_UNK35 = 35, // init 1
	UIVAR_PET = 36, // hotkey 'O'
	UIVAR_QUESTSCROLL = 37, // for showing quest information when click quest item.
};

struct D2EditBox {
	DWORD _1;
	DWORD _2[6];
	void(__fastcall *func)(D2EditBox*); // +1c
	DWORD _3[0x0D];
	DWORD	dwSelectStart; // +54
	DWORD	dwSelectEnd; // +58
	wchar_t text[0x100];		// +5c
	DWORD	dwCursorPos;		// 0x25C
};