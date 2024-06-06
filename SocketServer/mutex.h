//
// mutex.h: Implementation of the Semaphore and Mutex classes.
//

#ifndef MUTEX_H
#define MUTEX_H

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <process.h>

class Semaphore
{
private:
	HANDLE m_semaphore;

public:
	Semaphore()
	{
		m_semaphore = CreateSemaphore(NULL, 0, 0x7ffffff, NULL);
	} // Semaphore()

	Semaphore(int available)
	{
		m_semaphore = CreateSemaphore(NULL, available, 0x7ffffff, NULL);
	} // Semaphore()

	~Semaphore()
	{

		CloseHandle(m_semaphore);
	}

	void Wait()
	{
		WaitForSingleObject(m_semaphore, INFINITE);
	}

	void Post()
	{
		ReleaseSemaphore(m_semaphore, 1, NULL);
	} // Post()

	void Post(int how_many)
	{
		ReleaseSemaphore(m_semaphore, how_many, NULL);
	}
};


class Mutex
{
private:
	CRITICAL_SECTION lock;

public:
	Mutex()
	{
		InitializeCriticalSection(&lock);
	}

	~Mutex()
	{
		DeleteCriticalSection(&lock);
	}

	void Lock()
	{
		EnterCriticalSection(&lock);
	}

	void Unlock()
	{
		LeaveCriticalSection(&lock);
	}
};

#endif // MUTEX_H