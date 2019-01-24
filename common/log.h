#pragma once

#include <Windows.h>
#include <tchar.h>

class Log
{
    public:
        ~Log();
        Log();

        const bool Initialize(LPCTSTR pName);
        const bool Write(LPCTSTR pMessage) const;
		const bool Write(LPCTSTR pMessage, const DWORD &pParam) const;
		const bool Write(LPCTSTR pMessage, LPCTSTR pParam) const;

    private:
		static const int MAX_MESSAGE = 1024;

        HANDLE m_hFile;
        LPTSTR m_szFile;
}; // Log