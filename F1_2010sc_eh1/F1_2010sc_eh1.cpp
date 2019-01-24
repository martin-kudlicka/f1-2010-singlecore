// F1_2010sc_eh1.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "F1_2010sc_eh1.h"

#include "..\common\log.h"
#include <Tlhelp32.h>
#include "..\easyhook\easyhook.h"
#include "..\F1_2010sc_eh2\F1_2010sc_eh2.h"

#ifndef _M_X64
	#pragma comment(lib, "..\\easyhook\\EasyHook32.lib")
#else
	#pragma comment(lib, "..\\easyhook\\EasyHook64.lib")
#endif

#ifdef _MANAGED
#pragma managed(push, off)
#endif

typedef BOOL (WINAPI *tCreateProcessW)(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

HANDLE g_hRemoteProcess = NULL;
HANDLE g_hRemoteThread = NULL;
Log g_lLog;
void *g_vMem = NULL;

const BOOL InitHooks();
const BOOL Initialize();
void InjectHook2(const LPPROCESS_INFORMATION pProcessInfo);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			return Initialize();
		case DLL_PROCESS_DETACH:
			// unhook
			LhUninstallAllHooks();

			// free allocated memory
			if (g_hRemoteProcess && g_vMem) {
				VirtualFreeEx(g_hRemoteProcess, g_vMem, (_tcslen(HOOK1_DLL) + 1) * sizeof(TCHAR), MEM_RELEASE);
			} // if
			// close handles
			if (g_hRemoteThread) {
				CloseHandle(g_hRemoteThread);
			} // if
		default:
			return TRUE;
    } // switch
} // DllMain

BOOL WINAPI HookCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	FARPROC fpCreateProcessW;
    LhBarrierGetCallback(reinterpret_cast<PVOID *>(&fpCreateProcessW));

	g_lLog.Write(_T("HookCreateProcessW called for file %s"), lpCommandLine);
	DWORD dwCustomCreationFlags;
	if (_tcscmp(lpCommandLine, _T("\"F1_2010_game.exe\" ")) == 0) {
		g_lLog.Write(_T("main game started with suspend flag"));
		dwCustomCreationFlags = CREATE_SUSPENDED;
	} else {
		dwCustomCreationFlags = 0;
	} // if else

    BOOL bRes = reinterpret_cast<tCreateProcessW>(fpCreateProcessW)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | dwCustomCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

	if (bRes && (dwCustomCreationFlags & CREATE_SUSPENDED)) {
		InjectHook2(lpProcessInformation);
	} // if

    return bRes;
} // HookCreateProcessW

const BOOL InitHooks()
{
    BOOL bRes;
    HANDLE hSnapshot = INVALID_HANDLE_VALUE;

    do {
        // get CreateProcessW
        HMODULE hmKernel32 = GetModuleHandle(_T("Kernel32.dll"));
        if (!hmKernel32) {
            g_lLog.Write(_T("failed to get Kernel32.dll module handle"));
            bRes = FALSE;
            break;
        } else {
            g_lLog.Write(_T("got Kernel32.dll module handle"));
        } // if else
        FARPROC fpCreateProcessW = GetProcAddress(hmKernel32, "CreateProcessW");
        if (!fpCreateProcessW) {
            g_lLog.Write(_T("failed to get CreateProcessW address"));
            bRes = FALSE;
            break;
        } else {
            g_lLog.Write(_T("got CreateProcessW address"));
        } // if else

        // go through threads in this process and hook other than this one
        hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            g_lLog.Write(_T("failed to get Toolhelp32 snapshot"));
            bRes = false;
            break;
        } else {
            g_lLog.Write(_T("got Toolhelp32 snapshot"));
        } // if else
        THREADENTRY32 pThreadEntry;
        pThreadEntry.dwSize = sizeof(THREADENTRY32);
        if (Thread32First(hSnapshot, &pThreadEntry)) {
            do {
                if (pThreadEntry.th32OwnerProcessID == GetCurrentProcessId() && pThreadEntry.th32ThreadID != GetCurrentThreadId()) {
                    ULONG ulACLEntries[1] = {pThreadEntry.th32ThreadID};

                    // hook for CreateProcessW
                    TRACED_HOOK_HANDLE thhCreateProcessW = new HOOK_TRACE_INFO();
                    if (!thhCreateProcessW) {
                        g_lLog.Write(_T("failed to create hook for CreateProcessW"));
                        bRes = FALSE;
                        break;
                    } else {
                        g_lLog.Write(_T("created hook for CreateProcessW"));
                    } // if else
                    NTSTATUS nsStatus = LhInstallHook(fpCreateProcessW, HookCreateProcessW, fpCreateProcessW, thhCreateProcessW);
					if (FAILED(nsStatus)) {
						g_lLog.Write(_T("failed to LhInstallHook for CreateProcessW"));
						bRes = FALSE;
						break;
					} else {
						g_lLog.Write(_T("LhInstallHook for CreateProcessW done"));
					} // if else
					nsStatus = LhSetInclusiveACL(ulACLEntries, sizeof(ulACLEntries), thhCreateProcessW);
					if (FAILED(nsStatus)) {
						g_lLog.Write(_T("failed to LhSetInclusiveACL for CreateProcessW"));
						bRes = FALSE;
						break;
					} else {
						g_lLog.Write(_T("LhSetInclusiveACL for CreateProcessW done"));
					} // if else
                } // if
            } while (Thread32Next(hSnapshot, &pThreadEntry));
        } // if

        bRes = TRUE;
    } while (false);

    return bRes;
} // InitHooks

const BOOL Initialize()
{
    BOOL bRes;

    do {
        bRes = g_lLog.Initialize(_T("F1_2010sc_eh1.log"));
        if (!bRes) {
            break;
        } // if

        bRes = InitHooks();
        if (!bRes) {
            g_lLog.Write(_T("InitHooks failed"));
        } else {
            g_lLog.Write(_T("InitHooks OK"));
        } // if else
    } while (false);

    return bRes;
} // Initialize

void InjectHook2(const LPPROCESS_INFORMATION pProcessInfo)
{
	g_lLog.Write(_T("injecting second hook library"));

	g_hRemoteProcess = pProcessInfo->hProcess;

	do {
		// allocate memory inside F1 2010 main
		g_vMem = VirtualAllocEx(g_hRemoteProcess, NULL, (_tcslen(HOOK2_DLL) + 1) * sizeof(TCHAR), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (!g_vMem) {
			g_lLog.Write(_T("failed to allocate memory inside game main process"));
			break;
		} else {
			g_lLog.Write(_T("memory inside game main process allocated"));
		} // if else

		// write hook library name into allocated memory
		SIZE_T stWritten;
		if (!WriteProcessMemory(g_hRemoteProcess, g_vMem, HOOK2_DLL, (_tcslen(HOOK2_DLL) + 1) * sizeof(TCHAR), &stWritten)) {
			g_lLog.Write(_T("failed to write hook library name into main process memory"));
			break;
		} else {
			g_lLog.Write(_T("hook library name written into main process memory"));
		} // if else

		// execute hook library
		HMODULE hKernel32 = GetModuleHandle(_T("Kernel32.dll"));
		if (!hKernel32) {
			g_lLog.Write(_T("failed to get Kernel32.dll module handle"));
			break;
		} else {
			g_lLog.Write(_T("got Kernel32.dll module handle"));
		} // if else
		FARPROC fpLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryW");
		if (!fpLoadLibrary) {
			g_lLog.Write(_T("failed to get LoadLibraryW address"));
			break;
		} else {
			g_lLog.Write(_T("got LoadLibraryW address"));
		} // if else
		g_hRemoteThread = CreateRemoteThread(g_hRemoteProcess, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(fpLoadLibrary), g_vMem, 0, NULL);
		if (!g_hRemoteThread) {
			g_lLog.Write(_T("failed to create remote thread inside game main process"));
			break;
		} else {
			g_lLog.Write(_T("created remote thread inside game main process"));
		} // if else

		g_lLog.Write(_T("waiting"));
		Sleep(100);

		// resume F1 2010 main
		ResumeThread(pProcessInfo->hThread);
		g_lLog.Write(_T("main game resumed"));
	} while (false);
} // InjectHook2

#ifdef _MANAGED
#pragma managed(pop)
#endif

