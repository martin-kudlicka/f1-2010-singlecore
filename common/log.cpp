#include "stdafx.h"
#include "log.h"

#include <stdlib.h>

LPCTSTR LINE_END = _T("\r\n");

Log::~Log()
{
    if (m_hFile) {
        CloseHandle(m_hFile);
    } // if
    if (m_szFile) {
        free(m_szFile);
    } // if
} // ~Log

const bool Log::Initialize(LPCTSTR pName)
{
    if (pName) {
        m_szFile = _tcsdup(pName);
        if (!m_szFile) {
            return false;
        } // if
    } else {
        return false;
    } // if

    m_hFile = CreateFile(pName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    return m_hFile != INVALID_HANDLE_VALUE;
} // Initialize

Log::Log()
{
    m_hFile = INVALID_HANDLE_VALUE;
    m_szFile = NULL;
} // Log

const bool Log::Write(LPCTSTR pMessage) const
{
    if (m_hFile) {
        size_t stMsgCopy = (_tcslen(pMessage) + _tcslen(LINE_END) + 1) * sizeof(TCHAR);
        LPTSTR szMsgCopy = static_cast<LPTSTR>(malloc(stMsgCopy));
        if (!szMsgCopy) {
            return false;
        } // if

        _tcscpy_s(szMsgCopy, stMsgCopy / sizeof(TCHAR), pMessage);
        _tcscat_s(szMsgCopy, stMsgCopy / sizeof(TCHAR), LINE_END);

        DWORD dwWritten;
        BOOL bRes = WriteFile(m_hFile, szMsgCopy, static_cast<DWORD>(stMsgCopy), &dwWritten, NULL);

        free(szMsgCopy);

        return bRes != FALSE;
    } else {
        return false;
    } // if else
} // Write

const bool Log::Write(LPCTSTR pMessage, const DWORD &pParam) const
{
	TCHAR tcMessage[MAX_MESSAGE];

	_stprintf_s(tcMessage, sizeof(tcMessage) / sizeof(TCHAR), pMessage, pParam);

	return Write(tcMessage);
} // Write

const bool Log::Write(LPCTSTR pMessage, LPCTSTR pParam) const
{
	TCHAR tcMessage[MAX_MESSAGE];

	_stprintf_s(tcMessage, sizeof(tcMessage) / sizeof(TCHAR), pMessage, pParam);

	return Write(tcMessage);
} // Write