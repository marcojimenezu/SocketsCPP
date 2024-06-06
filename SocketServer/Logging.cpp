#include "Logging.h"
#include <direct.h>

//extern std::atomic<bool> g_KeepRunning;
extern bool g_KeepRunning;

/*-----------------------------------------------------------------------------
|
|  METHOD : CLogging() & ~CLogging()
|
|  PURPOSE: Constructor & Destructor.
|
-----------------------------------------------------------------------------*/
CLogging::CLogging(const bool bBreakOnHourChange, const bool bStartupHdr, const char* szLogName, const char* szIniName)
: m_bBreakOnHourChange(bBreakOnHourChange), m_bStartupHdr(bStartupHdr),
m_dwTotalBytesWritten(0), m_dwMaxBytesPerLogFile(0), m_dwFileCounter(0), m_dwTotalWriteBytes(0)
{
	*m_szOutBuffer = 0;

	*m_szIniFileName = 0;

	if (*szIniName)
		strcpy(m_szIniFileName, szIniName);
	else
		IniFileName(m_szIniFileName, sizeof(m_szIniFileName));

	*m_szAppNumber = 0;
	GetPrivateProfileString("Log Versions", szLogName, "0", m_szAppNumber, sizeof(m_szAppNumber), m_szIniFileName);
	*m_szAppNumber = (*m_szAppNumber == '0') ? 0 : *m_szAppNumber;

	strcpy(m_szSection, szLogName);

	strcpy(m_szLogName, szLogName);
	strcat(m_szLogName, m_szAppNumber);

	m_nFlags = 0;
	LoadLogErrorLevels();

	GetLocalTime(&m_LastSystemTime);
	BuildLogFileName();
	GetCurrentLogFileSize();
	WriteLogFileHeader();

	m_hDateMonitorThread = CreateThread(NULL, 0, DateMonitorThread, this, 0, &m_dwThreadId);
	m_hQueueMonitorThread = CreateThread(NULL, 0, QueueMonitorThread, this, 0, &m_dwThreadId);
} // CLogging()

CLogging::CLogging(const char* szLogName)
: m_dwTotalBytesWritten(0), m_dwMaxBytesPerLogFile(0), m_dwFileCounter(0), m_dwTotalWriteBytes(0)
{
	*m_szOutBuffer = 0;

	*m_szIniFileName = 0;
	IniFileName(m_szIniFileName, sizeof(m_szIniFileName));

	*m_szAppNumber = 0;
	GetPrivateProfileString("Log Versions", szLogName, "0", m_szAppNumber, sizeof(m_szAppNumber), m_szIniFileName);
	*m_szAppNumber = (*m_szAppNumber == '0') ? 0 : *m_szAppNumber;

	strcpy(m_szSection, szLogName);

	strcpy(m_szLogName, szLogName);
	strcat(m_szLogName, m_szAppNumber);

	m_nFlags = 0;
	LoadLogErrorLevels();

	GetLocalTime(&m_LastSystemTime);
	BuildLogFileName();
	GetCurrentLogFileSize();
	WriteLogFileHeader();

	m_hDateMonitorThread = CreateThread(NULL, 0, DateMonitorThread, this, 0, &m_dwThreadId);
	m_hQueueMonitorThread = CreateThread(NULL, 0, QueueMonitorThread, this, 0, &m_dwThreadId);
} // CLogging()


CLogging::~CLogging()
{
	TerminateThread(m_hDateMonitorThread, 0);
	CloseHandle(m_hDateMonitorThread);
	TerminateThread(m_hQueueMonitorThread, 0);
	CloseHandle(m_hQueueMonitorThread);
	WriteLogFileMsg("", true);
	StartLogFile();
} // ~CLogging()


/*-----------------------------------------------------------------------------
|
|  METHOD : BuildLogFileName()
|
|  PURPOSE: Builds the Log file's date and time which is part of the
|           logfile name.
|
|  RETURNS: char* - Formatted date & time.
|
-----------------------------------------------------------------------------*/
void CLogging::BuildLogFileName()
{
	char szFileName[MAX_PATH + 1], szDateTime[100];
	//GetModuleFileName( NULL, szFileName, sizeof( szFileName ) );
	strcpy(szFileName, m_szIniFileName);
	strcpy(strrchr(szFileName, '.'), "");

	strncpy(m_szLogFile, (*szFileName == '"') ? szFileName + 1 : szFileName, 3);
	*(m_szLogFile + 3) = '\0';																													// "C:\"

	strcpy(m_szTempLogFile, m_szLogFile);																							// "C:\"

	char* pLastSlash = strrchr(szFileName, '\\');
	strcpy(m_szModuleName, pLastSlash ? pLastSlash + 1 : "UnKnown");

	strcat(m_szTempLogFile, *m_szLogName ? m_szLogName : m_szModuleName);							// "C:\AppName"
	strcat(m_szTempLogFile, ".Log");																									// "C:\AppName.Log"

	strcat(m_szLogFile, "Logs");																											// "C:\Logs"
	strcat(m_szLogFile, "\\");
	strcat(m_szLogFile, *m_szLogName ? m_szLogName : m_szModuleName);									// "C:\Logs\AppName"
	strcat(m_szLogFile, "\\");
	strcat(m_szLogFile, *m_szLogName ? m_szLogName : m_szModuleName);									// "C:\Logs\AppName\AppName"
	strcat(m_szLogFile, BuildDateTime(szDateTime));																	// "C:\Logs\AppName\AppName-YYYYMMDD-HH00" (1)

	if (m_dwMaxBytesPerLogFile)
	{
		char szNum[10 + 1];
		sprintf(szNum, "-%0d", ++m_dwFileCounter);		 																	// (1) with: "-0#" if it gets here.
		strcat(m_szLogFile, szNum);
	}

	strcat(m_szLogFile, ".Log");																											// "C:\Logs\AppName\AppName-YYYYMMDD-HH00.Log"
} // BuildLogFileName()


/*-----------------------------------------------------------------------------
|
|  METHOD : BuildDateTime()
|
|  PURPOSE: Builds the Log file's date and time which is part of the
|           logfile name.
|
|  RETURNS: char* - Formatted date & time.
|
-----------------------------------------------------------------------------*/
char* CLogging::BuildDateTime(char* szDateTime)
{
	sprintf(szDateTime, "-%4d%02d%02d-%02d00",
		m_LastSystemTime.wYear, m_LastSystemTime.wMonth,
		m_LastSystemTime.wDay, m_LastSystemTime.wHour);

	return szDateTime;
} // BuildDateTime()


/*-----------------------------------------------------------------------------
|
|  METHOD : BuildLogDateTime()
|
|  PURPOSE: Builds the Log date time which is prefixed on most messages.
|
|  RETURNS: char* - Formatted date & time.
|
-----------------------------------------------------------------------------*/
char* CLogging::BuildLogDateTime(char* szDateTime)
{
	SYSTEMTIME sTime;
	GetLocalTime(&sTime);

	sprintf(szDateTime, "%4d-%02d-%02d %02d:%02d:%02d.%03d: ",
		sTime.wYear, sTime.wMonth, sTime.wDay,
		sTime.wHour, sTime.wMinute, sTime.wSecond, sTime.wMilliseconds);

	return szDateTime;
} // BuildLogDateTime()


/*-----------------------------------------------------------------------------
|
|  METHOD : Log()
|
|  PURPOSE: Logs a msg to the log file.
|
-----------------------------------------------------------------------------*/
void CLogging::Log(char* szMsg, ...)
{
	if (!szMsg)
	{
		return;
	}

	char szLogMsg[4 * 1024] = "";

	try {
		BuildLogDateTime(szLogMsg);

		// Build the error message into the szLogMsg variable.
		va_list list;
		va_start(list, szMsg);
		_vsnprintf(szLogMsg + 25, sizeof(szLogMsg)-25 - 1, szMsg, list);
		va_end(list);

#ifdef _QUEUE
		QueueAdd(szLogMsg);
#else
		LogFileDateCheck();
		WriteLogFileMsg(szLogMsg);
#endif // _QUEUE
	}
	catch (...) {
		strcpy(szLogMsg, "Error creating LOG entry: ");
		strcat(szLogMsg, szMsg);
		WriteLogFileMsg(szLogMsg);
	}
} // Log()

void CLogging::Log(const cLEVELS nLevel, char* szMsg, ...)
{
	if (!szMsg || !IsErrorLevelSet(nLevel))
	{
		return;
	}

	char szLogMsg[4 * 1024] = "";

	try {
		BuildLogDateTime(szLogMsg);

		// Build the error message into the szLogMsg variable.
		va_list list;
		va_start(list, szMsg);
		_vsnprintf(szLogMsg + 25, sizeof(szLogMsg)-25 - 1, szMsg, list);
		va_end(list);

#ifdef _QUEUE
		QueueAdd(szLogMsg);
#else
		LogFileDateCheck();
		WriteLogFileMsg(szLogMsg);
#endif // _QUEUE
	}
	catch (...) {
		strcpy(szLogMsg, "Error creating LOG entry: ");
		strcat(szLogMsg, szMsg);
		WriteLogFileMsg(szLogMsg);
	}
} // Log()


/*-----------------------------------------------------------------------------
|
|  METHOD : LogNT()
|
|  PURPOSE: Logs without placing the time in the log string.
|
-----------------------------------------------------------------------------*/
void CLogging::LogNT(char* szMsg, ...)
{
	if (!szMsg)
	{
		return;
	}

	char szLogMsg[4 * 1024] = "";

	try {
		// Build the error message into the szLogMsg variable.
		va_list list;
		va_start(list, szMsg);
		_vsnprintf(szLogMsg, sizeof(szLogMsg)-1, szMsg, list);
		va_end(list);

#ifdef _QUEUE
		QueueAdd(szLogMsg);
#else
		LogFileDateCheck();
		WriteLogFileMsg(szLogMsg);
#endif // _QUEUE
	}
	catch (...) {
		strcpy(szLogMsg, "Error creating LOG entry: ");
		strcat(szLogMsg, szMsg);
		WriteLogFileMsg(szLogMsg);
	}
} // LogNT()

void CLogging::LogNT(const cLEVELS nLevel, char* szMsg, ...)
{
	if (!szMsg || !IsErrorLevelSet(nLevel))
	{
		return;
	}

	char szLogMsg[4 * 1024] = "";

	try {
		// Build the error message into the szLogMsg variable.
		va_list list;
		va_start(list, szMsg);
		_vsnprintf(szLogMsg, sizeof(szLogMsg)-1, szMsg, list);
		va_end(list);

#ifdef _QUEUE
		QueueAdd(szLogMsg);
#else
		LogFileDateCheck();
		WriteLogFileMsg(szLogMsg);
#endif // _QUEUE
	}
	catch (...) {
		strcpy(szLogMsg, "Error creating LOG entry: ");
		strcat(szLogMsg, szMsg);
		WriteLogFileMsg(szLogMsg);
	}
} // LogNT()


/*-----------------------------------------------------------------------------
|
|  METHOD : DateCheck()
|
|  PURPOSE: Checks the current date with the last system time. If the year,
|           month, day, or hour have changed, the bNewLogFile flagged on.
|           (Also, if the flag is off, the total bytes written counter is
|           checked and the bNewLogFile flag is set accordingly).
|
|  RETURNS: true  - if its time for a new log file.
|           false - otherwise.
|
-----------------------------------------------------------------------------*/
bool CLogging::DateCheck()
{
	SYSTEMTIME nowTime;
	GetLocalTime(&nowTime);

	bool bNewLogFile = (m_LastSystemTime.wYear != nowTime.wYear ||
		m_LastSystemTime.wMonth != nowTime.wMonth ||
		m_LastSystemTime.wDay != nowTime.wDay) ||
		(m_bBreakOnHourChange && m_LastSystemTime.wHour != nowTime.wHour);

	if (bNewLogFile)
	{
		m_dwFileCounter = 0;
	}
	else
	{
		if (m_dwMaxBytesPerLogFile && m_dwTotalBytesWritten >= m_dwMaxBytesPerLogFile)
		{
			bNewLogFile = true;
		}
	}

	return bNewLogFile;
} // DateCheck()


/*-----------------------------------------------------------------------------
|
|  METHOD : LogFileDateCheck()
|
|  PURPOSE: Checks if it is time to start a new logfile.
|
-----------------------------------------------------------------------------*/
void CLogging::LogFileDateCheck()
{
	if (DateCheck())
	{
		StartLogFile();
		WriteLogFileHeader();
	}
} // LogFileDateCheck()


/*-----------------------------------------------------------------------------
|
|  METHOD : StartLogFile()
|
|  PURPOSE: Moves the current log file to \logs\xxxx\nnn-xx.log.
|
-----------------------------------------------------------------------------*/
void CLogging::StartLogFile()
{
	m_LogMutex.Lock();

	try
	{
		GetLocalTime(&m_LastSystemTime);
		MakeLogDir(m_szLogFile);
		rename(m_szTempLogFile, m_szLogFile);
		m_dwTotalBytesWritten = 0;
		BuildLogFileName();
	}
	catch (...)
	{
	}

	m_LogMutex.Unlock();
} // StartLogFile()


/*-----------------------------------------------------------------------------
|
|  METHOD : WriteLogFileHeader()
|
|  PURPOSE: Writes the startup header to the log file.
|
-----------------------------------------------------------------------------*/
void CLogging::WriteLogFileHeader()
{
	if (m_bStartupHdr)
	{
		Log(" ");
		Log("----------   %s   ---------", m_szLogName);
		Log(" ");
	}
} // WriteLogFileHeader()


/*-----------------------------------------------------------------------------
|
|  METHOD : WriteLogFileMsg()
|
|  PURPOSE: Concats a message to the LogMsg buffer and writes the OutBuffer
|           to the logfile if bWriteNow is TRUE, or if msg is > 10k.
|
-----------------------------------------------------------------------------*/
void CLogging::WriteLogFileMsg(const char* szLogMsg, const bool bWriteNow)
{
	m_LogMutex.Lock();

	HANDLE hLogFile = INVALID_HANDLE_VALUE;

	try
	{
		if (*szLogMsg)
		{
			int nMsgLen = strlen(szLogMsg);
			memcpy(m_szOutBuffer + m_dwTotalWriteBytes, szLogMsg, nMsgLen);
			m_dwTotalWriteBytes += nMsgLen;

			memcpy(m_szOutBuffer + m_dwTotalWriteBytes, "\r\n", 2);
			m_dwTotalWriteBytes += 2;
		}

		if (*m_szOutBuffer && (bWriteNow || m_dwTotalWriteBytes > 1024))
		{
			hLogFile = CreateFile(m_szTempLogFile,
				GENERIC_READ | GENERIC_WRITE,
				//FILE_SHARE_READ | FILE_SHARE_WRITE,
				FILE_SHARE_READ,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, // FILE_ATTRIBUTE_ARCHIVE,
				NULL);

			if (hLogFile != INVALID_HANDLE_VALUE)
			{
				DWORD dwResult = SetFilePointer(hLogFile, 0, NULL, FILE_END);

				DWORD dwWritten = 0;
				BOOL bResult = WriteFile(hLogFile, m_szOutBuffer, m_dwTotalWriteBytes, &dwWritten, NULL);
				m_dwTotalBytesWritten += dwWritten;
				m_dwTotalWriteBytes = 0;

				CloseHandle(hLogFile);
			}

			*m_szOutBuffer = 0;
		} // if ( len() )
	}
	catch (...)
	{
		*m_szOutBuffer = 0;
		if (hLogFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hLogFile);
		}
	}

	m_LogMutex.Unlock();
} // WriteLogFileMsg()


/*-----------------------------------------------------------------------------
|
|  METHOD : DateMonitorThread()
|
|  PURPOSE: Monitors for a DateCheck() change every minute. (Min CPU usage).
|
|  RETURNS: 1 - if lpvoid is null.
|           0 - if it should ever stop looping.
|
-----------------------------------------------------------------------------*/
DWORD WINAPI CLogging::DateMonitorThread(void* lpvoid)
{
	CLogging* pCLogging = (CLogging*)lpvoid;

	if (!pCLogging)
	{
		return 1;
	}

	pCLogging->DateMonitorThread();
	return 0;
} // DateMonitorThread()

void CLogging::DateMonitorThread()
{
	int nCount = -1;

	while (g_KeepRunning)
	{
		Sleep(10 * 1000);
		LoadLogErrorLevels();

		WriteLogFileMsg("", true);

		if ((++nCount % 6) == 0)
		{
			nCount = 0;
			if (DateCheck())
			{
				StartLogFile();
				WriteLogFileHeader();
			}
		}
	} // while()
} // DateMonitorThread()


/*-----------------------------------------------------------------------------
|
|  METHOD : QueueMonitorThread()
|
|  PURPOSE: Monitors the LogQueue().
|
|  RETURNS: 1 - if lpvoid is null.
|           0 - if it should ever stop looping.
|
-----------------------------------------------------------------------------*/
DWORD WINAPI CLogging::QueueMonitorThread(void* lpvoid)
{
	CLogging* pCLogging = (CLogging*)lpvoid;

	if (!pCLogging)
	{
		return 1;
	}

	pCLogging->QueueMonitorThread();
	return 0;
} // QueueMonitorThread()

void CLogging::QueueMonitorThread()
{
	while (g_KeepRunning)
	{
		std::string* str = QueueWait();
		LogFileDateCheck();
		WriteLogFileMsg(str->data());
		delete str;
	}
} // QueueMonitorThread()


/*-----------------------------------------------------------------------------
|
|  METHOD : LoadLogErrorLevels()
|
|  PURPOSE: Sets the Error Level flags from the INI file.
|           This method is called every 10 seconds.
|
-----------------------------------------------------------------------------*/
void CLogging::LoadLogErrorLevels()
{
	long nFlags = 0;
	char szKey[50 + 1];

	for (int nErrorLevel = ELEVEL_1; nErrorLevel < MAX_ELEVELS; ++nErrorLevel)
	{
		sprintf(szKey, "ErrorLevel_%d", nErrorLevel + 1);
		nFlags |= SetErrorLevel((cLEVELS)nErrorLevel, GetPrivateProfileInt(szKey, "Enabled", 1, m_szIniFileName) == 1);
	} // for()

	InterlockedExchange(&m_nFlags, nFlags); // Set all the flags in one shot.
	InterlockedExchange((long*)&m_dwMaxBytesPerLogFile, (long)abs((long)GetPrivateProfileInt("Settings", "MaxBytesPerLogFile", 0, m_szIniFileName)));

	InterlockedExchange(&m_bStartupHdr, GetPrivateProfileInt(m_szSection, "StartupHdr", 0, m_szIniFileName) == 1);
	InterlockedExchange(&m_bBreakOnHourChange, GetPrivateProfileInt(m_szSection, "BreakOnHourChange", 0, m_szIniFileName) == 1);
} // LoadLogErrorLevels()


/*-----------------------------------------------------------------------------
|
|  METHOD : IniFileName()
|
|  PURPOSE: Creates the .INI filename from the executable name.
|
-----------------------------------------------------------------------------*/
void CLogging::IniFileName(char* szIniFileName, const int nSize)
{
	if (szIniFileName&& nSize > 0)
	{
		GetModuleFileName(NULL, szIniFileName, nSize);
		strcpy(strrchr(szIniFileName, '.') + 1, "ini");
	}
} // IniFileName()


/*-----------------------------------------------------------------------------
|
|  METHOD : GetCurrentLogFileSize()
|
|  PURPOSE: Sets the m_dwTotalBytesWritten variable on startup to the
|           current log filesize.
|
-----------------------------------------------------------------------------*/
void CLogging::GetCurrentLogFileSize()
{
	HANDLE hLogFile = CreateFile(m_szTempLogFile,
		GENERIC_READ | GENERIC_WRITE,
		//FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hLogFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwSize = GetFileSize(hLogFile, NULL);
		if (dwSize != 0xFFFFFFFF)
		{
			m_dwTotalBytesWritten = dwSize;
		}

		CloseHandle(hLogFile);
	}
} // GetCurrentLogFileSize()


/*-----------------------------------------------------------------------------
|
|  METHOD : MakeLogDir()
|
|  PURPOSE: Creates the target log directory which is taken from the
|           full-path of the logfile name.
|
-----------------------------------------------------------------------------*/
void CLogging::MakeLogDir(const char* szLogFile)
{
	char szDir[MAX_PATH + 1];

	strcpy(szDir, szLogFile);
	if (!*szDir)
	{
		return;
	}

	strcpy(strrchr(szDir, '\\') + 1, "");
	for (size_t i = 4; i < strlen(szDir); ++i)
	{
		if (szDir[i] == '\\')
		{
			szDir[i] = '\0';
			_mkdir(szDir);                    // Could check for existence here. OR not.
			szDir[i] = '\\';
		}
	}
} // MakeLogDir()

