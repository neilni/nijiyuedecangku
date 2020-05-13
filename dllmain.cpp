// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "misc.h"
#include "ShareData.h"
#include "D2Hack1.11b.h"


extern void InstallPatchs();
extern void RelocPatchsAddr();
extern void ResetHackGlobalObj();
extern void RemoveD2Patchs();
extern void DeleteD2Patchs();

HANDLE g_hD2ClientCmdEvt = NULL;
HANDLE g_hD2HackReplyEvt = NULL;
HANDLE g_hD2HackMsgEvt = NULL;
HANDLE g_hD2ClientMsgConfirmEvt = NULL;
HANDLE g_hD2HackSharedBuf = NULL;
LPVOID g_D2HackSharedBuf = NULL;

BOOL g_bDllExitNow = FALSE;

BOOL InitialGlobalObj()
{
	char ObjName[64];
	sprintf_s(ObjName, sizeof(ObjName), "%s%d", D2CLIENT_CMD, GetCurrentProcessId());
	g_hD2ClientCmdEvt = CreateEvent(
		NULL,               // default security attributes
		FALSE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		ObjName  // object name
		);

	if (g_hD2ClientCmdEvt == NULL)
		return FALSE;

	sprintf_s(ObjName, sizeof(ObjName), "%s%d", D2HACK_REPLY, GetCurrentProcessId());
	g_hD2HackReplyEvt = CreateEvent(
		NULL,               // default security attributes
		FALSE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		ObjName  // object name
		);

	if (g_hD2HackReplyEvt == NULL)
		return FALSE;

	sprintf_s(ObjName, sizeof(ObjName), "%s%d", D2HACK_MSG, GetCurrentProcessId());
	g_hD2HackMsgEvt = CreateEvent(
		NULL,               // default security attributes
		FALSE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		ObjName  // object name
		);

	if (g_hD2HackMsgEvt == NULL)
		return FALSE;

	sprintf_s(ObjName, sizeof(ObjName), "%s%d", D2CLIENT_MSG_CONFIRM, GetCurrentProcessId());
	g_hD2ClientMsgConfirmEvt = CreateEvent(
		NULL,               // default security attributes
		FALSE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		ObjName  // object name
		);

	if (g_hD2ClientMsgConfirmEvt == NULL)
		return FALSE;

	sprintf_s(ObjName, sizeof(ObjName), "%s%d", D2HACK_SHARED_BUF, GetCurrentProcessId());
	g_hD2HackSharedBuf = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		D2HACK_SHARED_BUF_SIZE,                // maximum object size (low-order DWORD)
		ObjName);                 // name of mapping object

	if (g_hD2HackSharedBuf == NULL)
	{
		DbgPrint("CreateFileMapping失败，Err：%x", GetLastError());
		return FALSE;
	}

	g_D2HackSharedBuf = (LPTSTR)MapViewOfFile(g_hD2HackSharedBuf,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		D2HACK_SHARED_BUF_SIZE);

	if (g_D2HackSharedBuf == NULL)
	{
		DbgPrint("MapViewOfFile失败，Err：%x", GetLastError());
		CloseHandle(g_hD2HackSharedBuf);
		return FALSE;
	}


	memset(&g_Game_Config, 0, sizeof(g_Game_Config));
	ResetHackGlobalObj();
	g_bDllExitNow = FALSE;
	return TRUE;
}

void CloseGlobalObj()
{
	CloseHandle(g_hD2ClientCmdEvt);
	CloseHandle(g_hD2HackReplyEvt);
	CloseHandle(g_hD2HackMsgEvt);
	CloseHandle(g_hD2ClientMsgConfirmEvt);

	UnmapViewOfFile(g_D2HackSharedBuf);
	CloseHandle(g_hD2HackSharedBuf);

	RemoveD2Patchs();
	DeleteD2Patchs();
}

extern DWORD WINAPI CommandProcess(LPVOID lpParam);
DWORD WINAPI ServiceLoop(LPVOID lpParam)
{
	UNREFERENCED_PARAMETER(lpParam);
	for (;;)
	{
		if (WaitForSingleObject(
			g_hD2ClientCmdEvt, // event handle
			INFINITE)    // indefinite wait
			== WAIT_OBJECT_0)
		{
			if (g_bDllExitNow)
			{
				DbgPrint("Service Loop Thread Exit!");			
				return 0;
			}
			D2HackCommandBuffer cmd = *(D2HackCommandBuffer*)g_D2HackSharedBuf;
			DbgPrint("Received a command!");

			DWORD dwThreadID;
			HANDLE hThread;
			hThread = CreateThread(
				NULL,              // default security
				0,                 // default stack size
				CommandProcess,        // name of the thread function
				&cmd,              // thread parameters
				0,                 // default startup flags
				&dwThreadID);
			CloseHandle(hThread);
		}
	}
	return 1;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		if (!InitialGlobalObj())
			return FALSE;

		RelocPatchsAddr();
		InstallPatchs();
		ReadConfigFile();
		DbgPrint("DLL已被注入D2中.");

		DWORD dwThreadID;
		HANDLE hThread;
		hThread = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			ServiceLoop,        // name of the thread function
			NULL,              // no thread parameters
			0,                 // default startup flags
			&dwThreadID);
		if (hThread == NULL)
			return FALSE;
		CloseHandle(hThread);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:

		g_bDllExitNow = TRUE;
		SetEvent(g_hD2ClientCmdEvt);
		CloseGlobalObj();		
		DbgPrint("DLL Detach!");
		break;
	}
	return TRUE;
}

