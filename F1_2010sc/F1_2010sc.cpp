// F1_2010sc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Windows.h>
#include "..\F1_2010sc_eh1\F1_2010sc_eh1.h"
#include "..\common\errors.h"
#include "..\common\log.h"

int _tmain(int argc, _TCHAR* argv[])
{
    HANDLE hThread = NULL;
    int iRes;
    PROCESS_INFORMATION pProcessInfo;
    void *vMem = NULL;

    ZeroMemory(&pProcessInfo, sizeof(pProcessInfo));

    do {
        Log lLog;

        // initialize logging
        if (!lLog.Initialize(_T("F1_2010sc.log"))) {
            iRes = ErrorLog;
            break;
        } else {
            lLog.Write(_T("log initialized"));
        } // if else

        // start F1 2010 suspended
	    STARTUPINFO pStartupInfo;
	    ZeroMemory(&pStartupInfo, sizeof(STARTUPINFO));
	    pStartupInfo.cb = sizeof(STARTUPINFO);
        if (!CreateProcess(_T("f1_2010.exe"), NULL, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &pStartupInfo, &pProcessInfo)) {
            lLog.Write(_T("failed to create game process"));
            iRes = ErrorWinAPI;
            break;
        } else {
            lLog.Write(_T("game process created"));
        } // if else

        // allocate memory inside F1 2010
	    vMem = VirtualAllocEx(pProcessInfo.hProcess, NULL, (_tcslen(HOOK1_DLL) + 1) * sizeof(TCHAR), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (!vMem) {
            lLog.Write(_T("failed to allocate memory inside game process"));
            iRes = ErrorWinAPI;
            break;
        } else {
            lLog.Write(_T("memory inside game process allocated"));
        } // if else

        // write hook library name into allocated memory
	    SIZE_T stWritten;
        if (!WriteProcessMemory(pProcessInfo.hProcess, vMem, HOOK1_DLL, (_tcslen(HOOK1_DLL) + 1) * sizeof(TCHAR), &stWritten)) {
            lLog.Write(_T("failed to write hook library name into process memory"));
            iRes = ErrorWinAPI;
            break;
        } else {
            lLog.Write(_T("hook library name written into process memory"));
        } // if else

        // execute hook library
	    HMODULE hKernel32 = GetModuleHandle(_T("Kernel32.dll"));
        if (!hKernel32) {
            lLog.Write(_T("failed to get Kernel32.dll module handle"));
            iRes = ErrorWinAPI;
            break;
        } else {
            lLog.Write(_T("got Kernel32.dll module handle"));
        } // if else
	    FARPROC fpLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryW");
        if (!fpLoadLibrary) {
            lLog.Write(_T("failed to get LoadLibraryW address"));
            iRes = ErrorWinAPI;
            break;
        } else {
            lLog.Write(_T("got LoadLibraryW address"));
        } // if else
	    hThread = CreateRemoteThread(pProcessInfo.hProcess, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(fpLoadLibrary), vMem, 0, NULL);
        if (!hThread) {
            lLog.Write(_T("failed to create remote thread inside game process"));
            iRes = ErrorWinAPI;
            break;
        } else {
            lLog.Write(_T("created remote thread inside game process"));
        } // if else

        lLog.Write(_T("waiting"));
        Sleep(100);

        // resume F1 2010
	    ResumeThread(pProcessInfo.hThread);
        lLog.Write(_T("game resumed"));

        // wait till it's finish
        WaitForSingleObject(pProcessInfo.hProcess, INFINITE);
        lLog.Write(_T("waiting till game finish"));

        iRes = ErrorOK;
    } while (false);

    // free allocated memory
    if (pProcessInfo.hProcess && vMem) {
        VirtualFreeEx(pProcessInfo.hProcess, vMem, (_tcslen(HOOK1_DLL) + 1) * sizeof(TCHAR), MEM_RELEASE);
    } // if

    // close handles
    if (hThread) {
	    CloseHandle(hThread);
    } // if
    if (pProcessInfo.hProcess) {
	    CloseHandle(pProcessInfo.hProcess);
    } // if

	return iRes;
}

