#include "ATMutex.h"

/*-----------------------------------------------------------------------------
|
|  METHOD : CATMutex() & ~CATMutex()
|
|  PURPOSE: Constructor & Destructor.
|
-----------------------------------------------------------------------------*/
CATMutex::CATMutex(bool locked)
{
	m_hSemaphore = 0;
	m_hSemaphore = CreateSemaphore(NULL, 1, 1, NULL);

	if (locked)
	{
		LockMutex();
	}
} // CATMutex()

CATMutex::~CATMutex()
{
	if (m_hSemaphore)
	{
		CloseHandle(m_hSemaphore);
		m_hSemaphore = 0;
	}
} // ~CATMutex()


/*-----------------------------------------------------------------------------
|
|  METHOD : LockMutex()
|
|  PURPOSE: Locks the mutex.
|
-----------------------------------------------------------------------------*/
void CATMutex::LockMutex(DWORD  dwMilliSec)
{
	WaitForSingleObject(m_hSemaphore, dwMilliSec);//INFINITE );
} // LockMutex()

void CATMutex::Lock(DWORD  dwMilliSec)
{
	WaitForSingleObject(m_hSemaphore, dwMilliSec);//INFINITE );
} // Lock()


/*-----------------------------------------------------------------------------
|
|  METHOD : UnlockMutex()
|
|  PURPOSE: Unlocks the mutex.
|
-----------------------------------------------------------------------------*/
void CATMutex::UnlockMutex()
{
	ReleaseSemaphore(m_hSemaphore, 1, NULL);
} // UnlockMutex()

void CATMutex::Unlock()
{
	ReleaseSemaphore(m_hSemaphore, 1, NULL);
} // Unlock()

