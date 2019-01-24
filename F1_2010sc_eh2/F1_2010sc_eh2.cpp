// F1_2010sc_eh2.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "F1_2010sc_eh2.h"

#include "..\common\log.h"
#include <Tlhelp32.h>
#include "..\easyhook\easyhook.h"

#ifndef _M_X64
	#pragma comment(lib, "..\\easyhook\\EasyHook32.lib")
#else
	#pragma comment(lib, "..\\easyhook\\EasyHook64.lib")
#endif

#ifdef _MANAGED
#pragma managed(push, off)
#endif

typedef void (WINAPI *tGetSystemInfo)(LPSYSTEM_INFO lpSystemInfo);
//typedef BOOL (WINAPI *tSetProcessAffinityMask)(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask);

Log g_lLog;

const BOOL InitHooks();
const BOOL Initialize();

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			return Initialize();
		case DLL_PROCESS_DETACH:
			// unhook
			LhUninstallAllHooks();
		default:
			return TRUE;
	} // switch
} // DllMain

void WINAPI HookGetSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
	FARPROC fpGetSystemInfo;
	LhBarrierGetCallback(reinterpret_cast<PVOID *>(&fpGetSystemInfo));

	g_lLog.Write(_T("GetSystemInfo called"));

	reinterpret_cast<tGetSystemInfo>(fpGetSystemInfo)(lpSystemInfo);
	lpSystemInfo->dwNumberOfProcessors = 2;
} // HookGetSystemInfo

/*BOOL WINAPI HookSetProcessAffinityMask(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask)
{
	FARPROC fpSetProcessAffinityMask;
	LhBarrierGetCallback(reinterpret_cast<PVOID *>(&fpSetProcessAffinityMask));

	g_lLog.Write(_T("SetProcessAffinityMask called as %d"), dwProcessAffinityMask);

	BOOL bRes = reinterpret_cast<tSetProcessAffinityMask>(fpSetProcessAffinityMask)(hProcess, dwProcessAffinityMask);

	return bRes;
} // HookSetProcessAffinityMask*/

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
		/*FARPROC fpSetProcessAffinityMask = GetProcAddress(hmKernel32, "SetProcessAffinityMask");
		if (!fpSetProcessAffinityMask) {
			g_lLog.Write(_T("failed to get SetProcessAffinityMask address"));
			bRes = FALSE;
			break;
		} else {
			g_lLog.Write(_T("got SetProcessAffinityMask address"));
		} // if else*/
		FARPROC fpGetSystemInfo = GetProcAddress(hmKernel32, "GetSystemInfo");
		if (!fpGetSystemInfo) {
			g_lLog.Write(_T("failed to get GetSystemInfo address"));
			bRes = FALSE;
			break;
		} else {
			g_lLog.Write(_T("got GetSystemInfo address"));
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

					/*// hook for SetProcessAffinityMask
					TRACED_HOOK_HANDLE thhSetProcessAffinityMask = new HOOK_TRACE_INFO();
					if (!thhSetProcessAffinityMask) {
						g_lLog.Write(_T("failed to create hook for SetProcessAffinityMask"));
						bRes = FALSE;
						break;
					} else {
						g_lLog.Write(_T("created hook for SetProcessAffinityMask"));
					} // if else
					NTSTATUS nsStatus = LhInstallHook(fpSetProcessAffinityMask, HookSetProcessAffinityMask, fpSetProcessAffinityMask, thhSetProcessAffinityMask);
					if (FAILED(nsStatus)) {
						g_lLog.Write(_T("failed to LhInstallHook for SetProcessAffinityMask"));
						bRes = FALSE;
						break;
					} else {
						g_lLog.Write(_T("LhInstallHook for SetProcessAffinityMask done"));
					} // if else
					nsStatus = LhSetInclusiveACL(ulACLEntries, sizeof(ulACLEntries), thhSetProcessAffinityMask);
					if (FAILED(nsStatus)) {
						g_lLog.Write(_T("failed to LhSetInclusiveACL for SetProcessAffinityMask"));
						bRes = FALSE;
						break;
					} else {
						g_lLog.Write(_T("LhSetInclusiveACL for SetProcessAffinityMask done"));
					} // if else*/

					// hook for GetSystemInfo
					TRACED_HOOK_HANDLE thhGetSystemInfo = new HOOK_TRACE_INFO();
					if (!thhGetSystemInfo) {
						g_lLog.Write(_T("failed to create hook for GetSystemInfo"));
						bRes = FALSE;
						break;
					} else {
						g_lLog.Write(_T("created hook for GetSystemInfo"));
					} // if else
					NTSTATUS nsStatus = LhInstallHook(fpGetSystemInfo, HookGetSystemInfo, fpGetSystemInfo, thhGetSystemInfo);
					if (FAILED(nsStatus)) {
						g_lLog.Write(_T("failed to LhInstallHook for GetSystemInfo"));
						bRes = FALSE;
						break;
					} else {
						g_lLog.Write(_T("LhInstallHook for GetSystemInfo done"));
					} // if else
					nsStatus = LhSetInclusiveACL(ulACLEntries, sizeof(ulACLEntries), thhGetSystemInfo);
					if (FAILED(nsStatus)) {
						g_lLog.Write(_T("failed to LhSetInclusiveACL for GetSystemInfo"));
						bRes = FALSE;
						break;
					} else {
						g_lLog.Write(_T("LhSetInclusiveACL for GetSystemInfo done"));
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
		bRes = g_lLog.Initialize(_T("F1_2010sc_eh2.log"));
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

#ifdef _MANAGED
#pragma managed(pop)
#endif

