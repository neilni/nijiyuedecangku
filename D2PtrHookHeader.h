#pragma once
#include "misc.h"
#include "D2Struct.h"
#include "D2Hack1.11b.h"

#define D2FUNCPTR(d1,v1,t1,t2,o1) typedef t1 d1##_##v1##_t##t2; d1##_##v1##_t *d1##_##v1 = (d1##_##v1##_t *)g_initD2Ptr(DLLOFFSET(d1,o1));
#define D2VARPTR(d1,v1,t1,o1)     typedef t1 d1##_##v1##_t;    d1##_##v1##_t *p_##d1##_##v1 = (d1##_##v1##_t *)g_initD2Ptr(DLLOFFSET(d1,o1));
#define D2ASMPTR(d1,v1,o1)        DWORD d1##_##v1 = (DWORD)g_initD2Ptr(DLLOFFSET(d1,o1));

//D2中有用的函数或变量表
D2FUNCPTR(D2GFX, GetHwnd, HWND __stdcall, (), -10022)

D2FUNCPTR(D2CLIENT, GetPlayerXOffset, int __stdcall, (), DLLBASE_D2CLIENT + 0x1BBE0)
D2FUNCPTR(D2CLIENT, GetPlayerYOffset, int __stdcall, (), DLLBASE_D2CLIENT + 0x1BBF0)
D2FUNCPTR(D2CLIENT, PrintGameStringAtTopLeft, void __stdcall, (const wchar_t* text, int arg2), DLLBASE_D2CLIENT + 0x16780)
D2FUNCPTR(D2CLIENT, RecvCommand07, void __fastcall, (BYTE *cmdbuf), DLLBASE_D2CLIENT + 0xBD6C0)
D2FUNCPTR(D2CLIENT, RecvCommand08, void __fastcall, (BYTE *cmdbuf), DLLBASE_D2CLIENT + 0xBD650)
D2FUNCPTR(D2CLIENT, GetDifficulty, BYTE __stdcall, (), DLLBASE_D2CLIENT + 0x30070)
D2FUNCPTR(D2CLIENT, SetUiVar, DWORD __fastcall, (DWORD varno, DWORD howset, DWORD unknown1), DLLBASE_D2CLIENT + 0x65690)
D2FUNCPTR(D2CLIENT, GetUiVar_I, DWORD __fastcall, (DWORD varno), DLLBASE_D2CLIENT + 0x61380)
D2FUNCPTR(D2CLIENT, GetQuestInfo, void* __stdcall, (), DLLBASE_D2CLIENT + 0x56BD0)
D2FUNCPTR(D2CLIENT, CalcShake, void __stdcall, (DWORD *xpos, DWORD *ypos), DLLBASE_D2CLIENT + 0x93300)
D2FUNCPTR(D2CLIENT, RevealAutomapRoom, void __stdcall, (DrlgRoom1 *room1, DWORD clipflag, AutomapLayer *layer), DLLBASE_D2CLIENT + 0x52A20)
D2FUNCPTR(D2CLIENT, GetItemName_Orig, void __stdcall, (UnitAny *item, wchar_t *name, DWORD Unknown/*always 0x100*/), DLLBASE_D2CLIENT + 0x75CF0)//Added by Leon
D2FUNCPTR(D2CLIENT, GetMonsterName_Orig, const wchar_t*, (UnitAny*)/*eax*/, DLLBASE_D2CLIENT + 0x4c360)	// Added by Leon
D2FUNCPTR(D2CLIENT, GetPosUnit, UnitAny* __stdcall, (UnitInventory*, DWORD xPos, DWORD yPos, char* Unknown1, char* Unknown2, DWORD U1, DWORD location), DLLBASE_D2CLIENT + 0xca94)// Added by Leon
D2FUNCPTR(D2CLIENT, ReadyToCastSkill_Orig, BOOL , (UnitAny*/*eax*/, DWORD * Unknown1), DLLBASE_D2CLIENT + 0x1D110)// Added by Leon //是否能够释放魔法，比如魔力是否足够，FCR间隔时间等等
D2FUNCPTR(D2CLIENT, UnknownFunUsed, DWORD  __stdcall, (UnitAny*), DLLBASE_D2CLIENT + 0xC296)// Added by Leon

D2VARPTR(D2CLIENT, PlayerUnit, UnitAny *, DLLBASE_D2CLIENT + 0x11C1E0)
D2VARPTR(D2CLIENT, pDrlgAct, DrlgAct *, DLLBASE_D2CLIENT + 0x11C2D0)
D2VARPTR(D2CLIENT, fExitAppFlag, DWORD, DLLBASE_D2CLIENT + 0xF18C0)
D2VARPTR(D2CLIENT, pUnitTable, POINT, DLLBASE_D2CLIENT + 0x10B470)
D2ASMPTR(D2CLIENT, GetUnitFromId_I, DLLBASE_D2CLIENT + 0x4B410)
D2VARPTR(D2CLIENT, xMapShake, int, DLLBASE_D2CLIENT + 0x11BAFC)
D2VARPTR(D2CLIENT, yMapShake, int, DLLBASE_D2CLIENT + 0xFD944)
D2VARPTR(D2CLIENT, UnitListHdr, UnitAny*, DLLBASE_D2CLIENT + 0x11C128)
D2VARPTR(D2CLIENT, pAutomapLayer, AutomapLayer *, DLLBASE_D2CLIENT + 0x11C154)

D2FUNCPTR(D2COMMON, GetDrlgLevel, DrlgLevel * __fastcall, (DrlgMisc *drlgmisc, DWORD levelno), -11058)
D2FUNCPTR(D2COMMON, InitDrlgLevel, void __stdcall, (DrlgLevel *drlglevel), -10741)
D2FUNCPTR(D2COMMON, GetObjectTxt, ObjectTxt * __stdcall, (DWORD objno), -10916)
D2FUNCPTR(D2COMMON, GetItemTxt, ItemTxt * __stdcall, (DWORD itemno), -10262)
D2FUNCPTR(D2COMMON, GetLevelTxt, LevelTxt * __stdcall, (DWORD levelno), -10445)
D2FUNCPTR(D2COMMON, GetUnitStat, int __stdcall, (UnitAny *unit, DWORD statno, DWORD unk), -10061)
D2FUNCPTR(D2COMMON, GetItemFromInventory, DWORD __stdcall, (UnitInventory* inv), -10535)
D2FUNCPTR(D2COMMON, GetNextItemFromInventory, DWORD __stdcall, (DWORD no), -11140)
D2FUNCPTR(D2COMMON, GetUnitFromItem, UnitAny * __stdcall, (DWORD arg1), -10748)
D2FUNCPTR(D2COMMON, GetItemValue, int __stdcall, (UnitAny * player, UnitAny * item, DWORD difficulty, void* questinfo, int value, DWORD flag), -10511)	
//D2COMMON_GetItemValue() comment: value=NPC_TxtFileNO, flag=0 buy，1 sell， 2 gamble . commnet by Leon
D2FUNCPTR(D2COMMON, GetItemFlag, DWORD __stdcall, (UnitAny *item, DWORD flagmask, DWORD lineno, char *filename), -10303)
D2VARPTR(D2COMMON, nWeaponsTxt, int, DLLBASE_D2COMMON + 0x9FB1C)
D2VARPTR(D2COMMON, nArmorTxt, int, DLLBASE_D2COMMON + 0x9FB24)
D2ASMPTR(D2COMMON, GetLevelIdFromRoom, -11021)

/*下俩函数用于返回宝箱或者Cube或者玩家背包空间的大小 - Added by Leon*/
D2FUNCPTR(D2COMMON, UnKnownFun1, void __stdcall, (DWORD U1, DWORD /*  0 */, LPVOID), DLLBASE_D2COMMON + 0x29810)
D2FUNCPTR(D2COMMON, UnKnownFun2, int __fastcall, (DWORD nLocation, UnitInventory *, LPVOID), DLLBASE_D2COMMON + 0x3e7c0) //注意前两个参数在EAX, EBX
/***************************************************/

D2FUNCPTR(D2NET, SendPacket_Orig, void __stdcall, (size_t len, DWORD arg1, BYTE* buf), -10020)

// D2WIN ptrs
D2FUNCPTR(D2WIN, SetTextSize, DWORD __fastcall, (DWORD size), -10170)
D2FUNCPTR(D2WIN, GetTextWidthFileNo, DWORD __fastcall, (const wchar_t *str, DWORD* width, DWORD* fileno), -10183)
D2FUNCPTR(D2WIN, DrawText, void __fastcall, (const wchar_t *str, int xpos, int ypos, DWORD col, DWORD unknown), -10064)
D2FUNCPTR(D2WIN, ORD_27D7, DWORD __fastcall, (LPCWSTR lpText, DWORD u1, DWORD u2), -10034)
D2FUNCPTR(D2WIN, CreateEditBox, D2EditBox* __fastcall, (DWORD style, int ypos, int xpos, DWORD arg4, DWORD arg5, DWORD arg6, DWORD arg7, DWORD arg8, DWORD arg9, DWORD size, void* buf), -10112)
D2FUNCPTR(D2WIN, DestroyEditBox, DWORD __fastcall, (D2EditBox* box), -10085)
D2FUNCPTR(D2WIN, GetEditBoxText, wchar_t* __fastcall, (D2EditBox* box), -10055)
D2FUNCPTR(D2WIN, AddEditBoxChar, DWORD __fastcall, (D2EditBox* box, BYTE keycode), -10084)
D2FUNCPTR(D2WIN, SetEditBoxText, void* __fastcall, (D2EditBox* box, wchar_t* txt), -10149)
D2FUNCPTR(D2WIN, SetEditBoxProc, void __fastcall, (D2EditBox* box, BOOL(__stdcall *FunCallBack)(D2EditBox*, DWORD, DWORD)), -10179)
D2FUNCPTR(D2WIN, SelectEditBoxText, void __fastcall, (D2EditBox* box), -10047)
D2VARPTR(D2WIN, pFocusedControl, D2EditBox*, DLLBASE_D2WIN + 0x20498)
D2FUNCPTR(D2WIN, DrawText2_I, DWORD __stdcall, (LPWSTR lpText, int x, int y, int nColor, DWORD u2), DLLBASE_D2WIN + 0xEE80)

#define D2Client_pPlayUnit (*p_D2CLIENT_PlayerUnit)
#define D2CLIENT_pDrlgAct (*p_D2CLIENT_pDrlgAct)
#define D2CLIENT_pAutomapLayer (*p_D2CLIENT_pAutomapLayer)
#define D2CLIENT_fExitAppFlag (*p_D2CLIENT_fExitAppFlag)
#define D2COMMON_nWeaponsTxt (*p_D2COMMON_nWeaponsTxt)
#define D2COMMON_nArmorTxt (*p_D2COMMON_nArmorTxt)
#define D2CLIENT_GetUiVar	D2CLIENT_GetUiVar_STUB
#define D2CLIENT_GetUnitFromId(unitid, unittype) ((UnitAny *)D2CLIENT_GetUnitFromId_STUB(unitid, unittype))
#define D2CLIENT_GetInventoryId		D2CLIENT_GetInventoryId_STUB
#define D2CLIENT_xMapShake (*p_D2CLIENT_xMapShake)
#define D2CLIENT_yMapShake (*p_D2CLIENT_yMapShake)

//D2中需要安装的钩子列表
Patch_t g_D2PatchList[] = {
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x76B66),ItemNamePatch_ASM, 6 },
#ifdef _DEBUG
	{ PatchCALL,(LPVOID)DLLOFFSET(D2NET, -10020),  SendPacketHook_ASM, 5 },
#endif
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0xBDFCF),PacketReceivedInterceptHook_ASM, 6 },
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x32B40),GameLoopPatch_ASM, 7},
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x33883),GameEndPatch_ASM, 5},
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x63EBA),PermShowItemsPatch_ASM, 6 },
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x66D5E),PermShowItemsPatch_ASM, 6 },
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x4D233),PermShowItemsPatch2_ASM, 6 },
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x5DDF3),PermShowItemsPatch_ASM, 6 },//added by Leon
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x66EC5),KeydownPatch_ASM, 7 },	
	{ PatchVALUE, (LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x6324F),(LPVOID)0xe990, 2 }, //ground gold name
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x32BB2), (LPVOID)ShakeScreenPatch, 5},
	{ PatchFILL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x9349F), (LPVOID)INST_NOP, 2 }, //force perspective shake
	{ PatchFILL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x1BDFF), (LPVOID)INST_NOP, 2 }, //force get shake
	{ PatchFILL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x933C6), (LPVOID)INST_NOP, 15 }, //kill add shake
	{ PatchFILL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x933D5), (LPVOID)INST_NOP, 2 }, //kill add shake, part 2
	{ PatchJMP,(LPVOID)DLLOFFSET(D2COMMON, DLLBASE_D2COMMON + 0x1E216), (LPVOID)WeatherPatch_ASM, 5 },
	{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x35ED7), (LPVOID)LightingPatch_ASM, 6 },
	//{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0xBF194), (LPVOID)SetStatePatch_ASM, 6 },	//TBD Leon 不如在PacketReceive函数实现方便
	//{ PatchCALL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0xBF151), (LPVOID)EndStatePatch_ASM, 6 },	//TBD

	//{ PatchCALL, (LPVOID)DLLOFFSET(D2MULTI, DLLBASE_D2MULTI + 0x148EB), (LPVOID)NextGameNamePatch, 5 },
	//{ PatchCALL, (LPVOID)DLLOFFSET(D2MULTI, DLLBASE_D2MULTI + 0x14C09), (LPVOID)NextGameNamePatch, 5 },
	{ PatchFILL,(LPVOID)DLLOFFSET(D2CLIENT, DLLBASE_D2CLIENT + 0x2812b), (LPVOID)INST_NOP, 8 }, //修正鼠标移动问题
};

//重定位地址
void RelocPatchsAddr()
{
	for (int i = 0; i < SIZEOFARRAY(g_D2PatchList); i++)
		g_D2PatchList[i].pOriginalAddr = GetDllOffset((int)g_D2PatchList[i].pOriginalAddr);
}

void InstallPatch(Patch_t& patch)
{
	//保存旧代码，安装新代码
	patch.oldcode = new BYTE[patch.len];
	ReadLocalBYTES(patch.pOriginalAddr, patch.oldcode, patch.len);
	if (patch.func)
		patch.func(patch.pOriginalAddr, patch.pHookAddr, patch.len);
}

void InstallPatchs()
{
	for (int i = 0; i < SIZEOFARRAY(g_D2PatchList); i++)
		InstallPatch(g_D2PatchList[i]);
}

inline void RemovePatch(Patch_t& patch)
{
	if (patch.oldcode)
		WriteLocalBYTES(patch.pOriginalAddr, patch.oldcode, patch.len);	
}

void RemoveD2Patchs()
{
	Patch_t* p = g_D2PatchList;

	do {
		RemovePatch(*p);
	} while (++p < (g_D2PatchList + SIZEOFARRAY(g_D2PatchList)));
}

inline void DeletePatch(Patch_t& patch)
{
	delete patch.oldcode;
}

void DeleteD2Patchs()
{
	Patch_t* p = g_D2PatchList;

	do {
		delete p->oldcode;
	} while (++p < (g_D2PatchList + SIZEOFARRAY(g_D2PatchList)));
}

#ifdef _DEBUG
void SendPacketHook(size_t pkSize, DWORD arg, BYTE* buf)
{
	if (buf[0] != 0x6d) {//   不trace ping 包
		DbgPrint("Client Packet(%d):%02x%02x %02x%02x %02x%02x %02x%02x  %02x%02x %02x%02x %02x%02x %02x%02x \
			%02x%02x %02x%02x %02x%02x %02x%02x  %02x%02x %02x%02x %02x%02x %02x%02x\n", pkSize,
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8],
			buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15], buf[16],
			buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23], buf[24], buf[25],
			buf[26], buf[27], buf[28], buf[29], buf[30], buf[31]);
	}
}

void __declspec(naked) SendPacketHook_ASM()
{
	__asm {
		push ebp;
		mov ebp, esp;
		pushad;
		pushfd;

		push[ebp + 20];
		push[ebp + 16];
		push[ebp + 12];
		call SendPacketHook;

		add esp, 12;

		popfd;
		popad;
		mov esp, ebp;
		pop ebp;

		//original code
		mov eax, ds:[6FBFB248h];
		ret;
	}
}
#endif

void __declspec(naked) PacketReceivedInterceptHook_ASM()
{
	__asm {
		mov eax, esi; // save length

		// original code
		movzx esi, bl;
		mov ebp, esi
		shl ebp, 1
		add ebp, esi

		pushad;
		// Call our clean c function
		mov edx, eax;
		mov ecx, edi;
		call PacketReceivedInterceptHook;
		// Return to game
		popad;
		add dword ptr[esp], 1;
		ret;
	}
}

void __declspec(naked) WeatherPatch_ASM()
{
	__asm {
		cmp[g_Game_Config.bWeatherEffect], 1
			je rainold
			xor al, al
			rainold :
		ret 0x04
	}
}

void __declspec(naked) LightingPatch_ASM()
{
	__asm {
		//[esp+4+0] = ptr red
		//[esp+4+1] = ptr green
		//[esp+4+2] = ptr blue
		//return eax = intensity
		cmp[g_Game_Config.bWeatherEffect], 1
			je lightold

			mov eax, 0xff
			mov byte ptr[esp + 4 + 0], al
			mov byte ptr[esp + 4 + 1], al
			mov byte ptr[esp + 4 + 2], al
			add dword ptr[esp], 0x72;
		ret
			lightold :
		//original code
		push esi
			call D2COMMON_GetLevelIdFromRoom
			ret
	}
}

void __declspec(naked) PermShowItemsPatch_ASM()
{
	__asm {
		call PermShowItemsPatch
		test eax, eax
		ret
	}
}

void __declspec(naked) PermShowItemsPatch2_ASM()
{
	__asm {
		push eax
		call PermShowItemsPatch
		mov ecx, eax
		pop eax
		ret
	}
}

void __declspec(naked) KeydownPatch_ASM()
{
	__asm {
		//edi = ptr to (hwnd, msg, wparam, lparam)
		mov cl, [edi + 0x08] //nVirtKey (wparam)
		mov dl, [edi + 0x0c + 3] //lKeyData (lparam)
		and dl, 0x40 //bit 30 of lKeyData (lparam)
		call KeydownPatch
		//original code
		test byte ptr[edi + 0x0c + 3], 0x40 //bit 30 of lKeyData (lparam)
		ret
	}
}

void __fastcall NextGameNamePatch(D2EditBox* box, BOOL(__stdcall *FunCallBack)(D2EditBox*, DWORD, DWORD))
{
	//D2WIN_SetEditBoxText(box, wszGameName);
	//D2WIN_SelectEditBoxText(box);
	// original code
	D2WIN_SetEditBoxProc(box, FunCallBack);
}

DWORD __declspec(naked) __fastcall D2CLIENT_GetUiVar_STUB(DWORD varno)
{
	__asm {
		mov eax, ecx
		jmp D2CLIENT_GetUiVar_I
	}
}

DWORD __declspec(naked) __fastcall D2CLIENT_GetUnitFromId_STUB(DWORD unitid, DWORD unittype)
{
	__asm
	{
		pop eax;
		push edx; // unittype
		push eax; // return address

		shl edx, 9;
		mov eax, p_D2CLIENT_pUnitTable;
		add edx, eax;
		mov eax, ecx; // unitid
		and eax, 0x7F;
		jmp D2CLIENT_GetUnitFromId_I;
	}
}

DWORD __declspec(naked) __fastcall D2CLIENT_GetInventoryId_STUB(UnitAny* pla, DWORD unitno, DWORD arg3)
{
	__asm {
		push    esi
			push    edi
			test    ecx, ecx
			jz      short loc_6FAB1952
			mov     eax, p_D2CLIENT_UnitListHdr
			mov		eax, [eax]
			mov     ecx, [ecx]UnitAny.nUnitId
			test    eax, eax
			jz      short loc_6FAB1952
			mov     esi, [esp + 0x0c]

			loc_6FAB1936:
		cmp[eax + 4], edx
			jnz     short loc_6FAB194B
			cmp[eax + 0Ch], ecx
			jnz     short loc_6FAB194B
			test    esi, esi

			jnz     short loc_6FAB195A
			mov     edi, [eax + 20h]
			test    edi, edi
			jz      short loc_6FAB195A

			loc_6FAB194B :
		mov     eax, [eax + 30h]
			test    eax, eax
			jnz     short loc_6FAB1936

			loc_6FAB1952 :
		pop     edi
			or eax, 0FFFFFFFFh
			pop     esi
			retn    4
			loc_6FAB195A :
			mov     eax, [eax + 8]
			pop     edi
			pop     esi
			retn    4
	}
}

void __declspec(naked) ItemNamePatch_ASM()
{
	__asm {
		//ebx = ptr unit item
		//edi = ptr item name string
		mov ecx, edi
			mov edx, ebx
			call ItemNamePatch
			mov al, [ebp + 0x12a]
			ret
	}
}

void D2NET_SendPacket(size_t len, DWORD arg1, BYTE* buf)
{
	if (!g_Game_Data.bCloseGameFlag)
		D2NET_SendPacket_Orig(len, arg1, buf);
}

void __fastcall D2CLIENT_GetItemName(UnitAny *item, wchar_t *name)
{
	D2CLIENT_GetItemName_Orig(item, name, 0x100);
}

const wchar_t* __fastcall D2CLIENT_GetMonsterName(UnitAny* unit)
{
	__asm
	{
		mov eax, ecx//unit	
		call D2CLIENT_GetMonsterName_Orig
	}
}

BOOL __fastcall D2CLIENT_ReadyToCastSkill(UnitAny* pPlayerUnit, DWORD* Unknown) //是否能够释放魔法，比如魔力是否足够，FCR间隔时间等等
{
	DWORD dw = D2CLIENT_UnknownFunUsed(pPlayerUnit);
	__asm
	{
		mov eax, ecx //pPlayerUnit
		lea ebx, dw
		push Unknown
		call D2CLIENT_ReadyToCastSkill_Orig
	}
}

//获取雇佣兵
inline UnitAny* GetPlayerMercUnit()
{
	return D2CLIENT_GetUnitFromId(D2CLIENT_GetInventoryId(D2Client_pPlayUnit, 7, 0), UNITNO_MONSTER);
}

//获取身上或者宝箱里（x，y）位置的物品UnitAny结构，nLocation=0：身上，4：宝箱, 3：方块里
UnitAny* GetPosUnit(DWORD x, DWORD y, DWORD nLocation)
{
	char tmp[64];
	DWORD u = nLocation == 4 ? 0xc : nLocation == 0 ? 2 : nLocation == 3 ? 9 : 0;
	return D2CLIENT_GetPosUnit(D2Client_pPlayUnit->pInventory, x, y, tmp, tmp + 4, u, nLocation);
}

//获取背包、宝箱或者Cube的储藏空间大小
void GetCabinetSize(UnitAny* pPlayer, DWORD nLocation, BYTE& nWidth, BYTE& nHeight)
{
	DWORD u = nLocation == 4 ? 0xc : nLocation == 0 ? 2 : nLocation == 3 ? 9 : 0;
	BYTE LocalVar[64];
	BYTE* nRet;
	D2COMMON_UnKnownFun1(u, 0, LocalVar);
	__asm {
		mov eax, [nLocation];
		add eax, 2;
		mov ebx, [pPlayer];
		mov ebx, dword ptr[ebx + 60h];
		lea ecx, [LocalVar];
		push ecx;
		call D2COMMON_UnKnownFun2;

		mov[nRet], eax;

	}
	if (nRet)
	{
		nWidth = nRet[8];
		nHeight = nRet[9];
	}
	else
	{
		nWidth = nHeight = 0;
	}
}
