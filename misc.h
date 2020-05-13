#pragma once

#include "ShareData.h"

#define INST_NOP 0x90
#define INST_CALL 0xe8
#define INST_JMP 0xe9
#define INST_JMPR 0xeb
#define INST_RET 0xc3

enum DllNo { DLLNO_D2CLIENT, DLLNO_D2COMMON, DLLNO_D2GFX, DLLNO_D2WIN, DLLNO_D2LANG, DLLNO_D2CMP, DLLNO_D2MULTI, DLLNO_BNCLIENT, DLLNO_D2NET, DLLNO_STORM, DLLNO_FOG };

enum DllBase {
	DLLBASE_D2CLIENT = 0x6FAB0000,
	DLLBASE_D2COMMON = 0x6FD50000,
	DLLBASE_D2GFX = 0x6FA80000,
	DLLBASE_D2WIN = 0x6F8E0000,
	DLLBASE_D2LANG = 0x6FC00000,
	DLLBASE_D2CMP = 0x6FE10000,
	DLLBASE_D2MULTI = 0x6F9D0000,
	DLLBASE_BNCLIENT = 0x6FF20000,
	DLLBASE_D2NET = 0x6FFB0000, // conflict with STORM.DLL
	DLLBASE_STORM = 0x6FBF0000,
	DLLBASE_FOG = 0x6FF50000
};

#define SIZEOFARRAY(x) (sizeof(x)/sizeof(x[0]))
#define DLLOFFSET(a1,b1) (((DLLNO_##a1)|(( ((b1)<0)?(b1):(b1)-DLLBASE_##a1 )<<8)))
#define D2TXTCODE(x) ( ((x)&0xff)<<24 | ((x)>>8&0xff)<<16 | ((x)>>16&0xff)<<8 | ((x)>>24&0xff) )
#define WAIT_UNTIL(time, waitnum, cond, bExit) {\
		int ____tmpNum__ = 0;\
		do{\
			Sleep(time);\
			++____tmpNum__;\
		}while((____tmpNum__ < waitnum) && !(cond));\
		if((bExit) && !(cond)) KillMe();\
	}

struct Patch_t {
	void(*func)(LPVOID, LPVOID, DWORD);
	LPVOID pOriginalAddr;
	LPVOID pHookAddr;
	BYTE len;
	BYTE *oldcode;
};

struct PatchItem_t {
	Patch_t *patch;
	int num;
	BOOL fInitOnDemand;
};

void DbgPrint(const char *format, ...);
void DbgPrint(const WCHAR *format, ...);

inline wchar_t * wcsrcat(wchar_t *dest, wchar_t *src)
{
	memmove(dest + wcslen(src), dest, (wcslen(dest) + 1)*sizeof(wchar_t));
	memcpy(dest, src, wcslen(src)*sizeof(wchar_t));
	return dest;
}

void ReadLocalBYTES(LPVOID pAddress, LPVOID buf, int len);
void WriteLocalBYTES(LPVOID pAddress, void *buf, int len);
void FillLocalBYTES(LPVOID pAddress, BYTE ch, int len);
DWORD ReadLocalDWORD(LPVOID pAddress);
void InterceptLocalCode2(BYTE inst, LPVOID pOldCode, DWORD offset, int len);
void InterceptLocalCode(BYTE inst, LPVOID pOldCode, LPVOID pNewCode, int len);
void PatchCALL(LPVOID addr, LPVOID param, DWORD len);
void PatchFILL(LPVOID addr, LPVOID param, DWORD len);
void PatchVALUE(LPVOID addr, LPVOID param, DWORD len);
void PatchJMP(LPVOID addr, LPVOID param, DWORD len);
LPVOID GetDllOffset(char *dll, int offset);
LPVOID GetDllOffset(int num);
LPVOID g_initD2Ptr(int offset);

void LButtonClick(HWND hwnd, WORD x, WORD y);
void RButtonClick(HWND hwnd, WORD x, WORD y);
void SendVKey(HWND hwnd, char c, short repeat = 1);
void SendString(HWND hwnd, const char* str);
void ReadConfigFile();
int GetItemCode(int ItemNo);
void SendHackMessage(D2HackReplyMsgType type, const char *format, ...);
void SendHackMessage(D2HackReplyMsgType type, const wchar_t *format, ...);
void SendHackMessage(D2HackReplyMsgType type, const D2HackReply::ItemInfo& r);

inline void KillMe()
{
	*(char*)NULL = 0;
}

typedef struct
{
	DWORD x;
	DWORD y;
	SIZE size;
}RoomPosSize;

typedef struct TreeNode MonsterPathTree;
struct TreeNode
{
	RoomPosSize RoomPos;
	LPVOID pRoom;
	MonsterPathTree *pNextNodeList;
	short NextNum;
	MonsterPathTree *pPreNode;	// 用于指示下一步路径
};

inline void InitMonsterPathTreeNode(MonsterPathTree& t)
{
	memset(&t, 0, sizeof(t));
}
void NewNodeAndAttachtoPathTree(MonsterPathTree *to, const MonsterPathTree& from);
void DeleteMonsterPathTree(MonsterPathTree* t);
MonsterPathTree* PathTreeNodeAlreadyExist(MonsterPathTree* header, LPVOID target);

#define SIMPLE_HEAP_SIZE 128
typedef struct SimplePathHeap
{
	MonsterPathTree* _SimpleHeap[SIMPLE_HEAP_SIZE];
	int nHeapHead, nHeapTail;

	SimplePathHeap()
	{
		nHeapHead = 1;
		nHeapTail = 0;
	}
	BOOL PushHeapItem(MonsterPathTree* item)
	{
		if ((nHeapHead + 1) % SIMPLE_HEAP_SIZE == nHeapTail)
		{
			DbgPrint("AddHeapItem（）堆溢出！！");
			return FALSE;
		}
		_SimpleHeap[nHeapHead++] = item;
		nHeapHead = nHeapHead % SIMPLE_HEAP_SIZE;
		return TRUE;
	}
	BOOL PopHeapItem(MonsterPathTree* &item)
	{
		if ((nHeapTail + 1) % SIMPLE_HEAP_SIZE == nHeapHead)
			return FALSE;
		nHeapTail = (nHeapTail + 1) % SIMPLE_HEAP_SIZE;
		item = _SimpleHeap[nHeapTail];
		
		return TRUE;
	}

}SimplePathHeap;