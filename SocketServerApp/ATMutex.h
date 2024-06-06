//
// ATMutex.h: Interface for the CATMutex class.
//

#ifndef _ATMUTEX_H
#define _ATMUTEX_H

#include <windows.h>

class CATMutex
{
public:
	CATMutex(bool locked = false);
	virtual ~CATMutex();

	virtual void LockMutex(DWORD dwMilliSec = INFINITE);
	virtual void UnlockMutex();

	virtual void Lock(DWORD  dwMilliSec = INFINITE);
	virtual void Unlock();

protected:
	HANDLE m_hSemaphore;
};

#endif // _ATMUTEX_H