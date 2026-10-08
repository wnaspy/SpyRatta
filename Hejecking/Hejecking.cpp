#include "Windows.h"
#include "stdio.h"
#include <detours.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib,"detours.lib")

typedef int(WINAPI* fnMessageBoxA)
(
	HWND hWnd,
	LPCSTR lpText,
	LPCSTR lpCaption,
	UINT uType
);
fnMessageBoxA g_fnMessageBoxA = MessageBoxA;


INT WINAPI MyMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
	printf("Hacked by ZLick3r\n");
	return g_fnMessageBoxA(hWnd, "Hacked by Zlick3r", "tbao", uType);
};

BOOL InstallHook()
{
	DWORD detoursError = NO_ERROR;
	if ((detoursError = DetourTransactionBegin()) != NO_ERROR)
	{
		printf("Hooking not OK\n");
		return false;
	}
	if ((detoursError = DetourUpdateThread(GetCurrentThread())) != NO_ERROR) {
		printf("Cant update\n");
		DetourTransactionAbort();
		return false;
	}
	if ((detoursError = DetourAttach((PVOID*)&g_fnMessageBoxA, MyMessageBoxA)) != NO_ERROR)
	{
		printf("faile\n");
		DetourTransactionAbort();
		return false;
	}
	if ((detoursError = DetourTransactionCommit()) != NO_ERROR) {
		printf("Fails\n");
		DetourTransactionAbort();
		return false;
	}

	printf("Success\n");
	return TRUE;
}



int main()
{

	MessageBoxA(NULL, "Hacked By kon1", "tbao", MB_OK);
	if (!InstallHook())
	{
		return -1;
	}
	MessageBoxA(NULL, "Hacked By kon2", "tbao", MB_OK);
	return 0;
}
