//
// Logging.h: Interface for the CLogging class.
//

#if !defined(AFX_LOGGING_H__7D34F06D_CA78_4B63_9FCA_6683AACB404A__INCLUDED_)
#define AFX_LOGGING_H__7D34F06D_CA78_4B63_9FCA_6683AACB404A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <time.h>

#ifdef _MUTEX
#include "mutex.h"
#else
#include "ATMutex.h"
#endif // _MUTEX

#include "Queue.h"
#include <string>

//using namespace std;

enum cLEVELS {
	ELEVEL_1 = 0,
	ELEVEL_2,
	ELEVEL_3,
	ELEVEL_4,
	ELEVEL_5,
	MAX_ELEVELS,
};

class CLogging
{
private:
	SYSTEMTIME m_LastSystemTime;

	long     m_bBreakOnHourChange;
	long     m_bStartupHdr;

	DWORD    m_dwTotalBytesWritten;
	DWORD    m_dwMaxBytesPerLogFile;
	DWORD    m_dwFileCounter;
	DWORD    m_dwTotalWriteBytes;

	char     m_szTempLogFile[MAX_PATH + 1];
	char     m_szLogFile[MAX_PATH + 1];
	char     m_szModuleName[MAX_PATH + 1];
	char     m_szSection[MAX_PATH + 1];
	char     m_szLogName[MAX_PATH + 1];
	char     m_szIniFileName[MAX_PATH + 1];
	char     m_szAppNumber[50 + 1];

	char     m_szOutBuffer[15 * 1024 + 1];

	long     m_nFlags;

	Queue< std::string* > m_LogQueue;

#ifdef _MUTEX
	Mutex    m_LogMutex;
#else
	CATMutex m_LogMutex;
#endif // _AT_MUTEX

	HANDLE   m_hDateMonitorThread;
	HANDLE   m_hQueueMonitorThread;
	DWORD    m_dwThreadId;

public:
	CLogging(const char* szLogName);
	CLogging(const bool bBreakOnHourChange = false, const bool bStartupHdr = true, const char* szLogName = "", const char* szIniName = "");
	virtual ~CLogging();

	void Log(char* szLogMsg, ...);
	void Log(const cLEVELS nLevel, char* szMsg, ...);

	void LogNT(char* szLogMsg, ...);
	void LogNT(const cLEVELS nLevel, char* szMsg, ...);

private:
	void  IniFileName(char* szIniFileName, const int nSize);
	void  BuildLogFileName();
	void  GetCurrentLogFileSize();
	void  StartLogFile();
	void  WriteLogFileHeader();
	void  LogFileDateCheck();

	char* BuildDateTime(char* szDateTime);
	char* BuildLogDateTime(char* szDateTime);

	bool  DateCheck();

	static DWORD WINAPI DateMonitorThread(void* lpvoid);
	void   DateMonitorThread();

	static DWORD WINAPI QueueMonitorThread(void* lpvoid);
	void   QueueMonitorThread();

	void   WriteLogFileMsg(const char* szLogMsg, const bool bWriteNow = false);
	void   LoadLogErrorLevels();

	inline int  SetErrorLevel(const cLEVELS nLevel, const bool bFlag)  { return (bFlag << nLevel); }
	inline bool IsErrorLevelSet(const cLEVELS nLevel) const            { return (m_nFlags & (1 << nLevel)) != 0; }

	inline void QueueAdd(char* pData) { m_LogQueue.Add((std::string*)(new std::string(pData))); }
	inline std::string* QueueWait()     { return (std::string*)m_LogQueue.Wait(); }

	void MakeLogDir(const char* szLogFile);
};

#endif
