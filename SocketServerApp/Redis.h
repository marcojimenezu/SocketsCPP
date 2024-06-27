#pragma once

#include <stdlib.h>
#include <vector>
#include <time.h>
#include <Windows.h>
#include <crtdbg.h>
#include <queue>

using namespace std;


#include <hiredis\hiredis.h>


enum OperationType
{
	CustomerOperation = 1,
	CarrierOperation = 2,
	CallConnect = 3
};

class CConnectionInformation
{
public:
	CConnectionInformation();
	virtual ~CConnectionInformation();

	char Host[129];
	unsigned long Port;

};

class CSemaphore
{
public:
	CSemaphore()
	{
		this->m_semaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	} // Semaphore()

	CSemaphore(int available)
	{
		this->m_semaphore = CreateSemaphore(NULL, available, LONG_MAX, NULL);
	} // Semaphore()

	~CSemaphore()
	{

		CloseHandle(this->m_semaphore);
	}

	void Wait()
	{
		WaitForSingleObject(this->m_semaphore, INFINITE);
	}

	void Post()
	{
		ReleaseSemaphore(this->m_semaphore, 1, NULL);
	} // Post()

	void Post(int how_many)
	{
		ReleaseSemaphore(this->m_semaphore, how_many, NULL);
	}

private:
	HANDLE m_semaphore;
};

class CMutex
{
public:
	CMutex(){ this->m_mutex = CreateMutex(NULL, FALSE, NULL); }
	virtual ~CMutex() { if (this->m_mutex) CloseHandle(this->m_mutex); }
	bool Lock(DWORD dwMilliseconds = INFINITE) { return (WaitForSingleObject(this->m_mutex, dwMilliseconds) == WAIT_OBJECT_0); };
	void Unlock() { ReleaseMutex(this->m_mutex); };
private:
	HANDLE m_mutex;
};

class CSentinel
{
public:
	CSentinel();
	virtual ~CSentinel();

	// Add a new Sentinel to the list
	void AddSentinel(char* Host, unsigned long Port);
	// Task the Sentinel with getting the ip of the master redis server
	bool GetConnectionInformation(CConnectionInformation& p_RedisConnectionInformation, unsigned long long Thread = 0);

protected:

private:
	vector<CConnectionInformation> ConnectionInformation;
};

typedef CConnectionInformation* PConnectionInformation;

class CRedisPool;

class CRedis : private CMutex
{
public:
	CRedis();
	virtual ~CRedis();

	// Assign either a Customer or Carrier Trunk
	int FraudANI(int CompanyID, LPCSTR ANI);

	CRedisPool* PRedisPool;

	unsigned long long PoolObjectID;

	// save the calling thred for logging purposes
	unsigned long long Thread;

protected:

private:

	//bool Lock(DWORD dwMilliseconds = INFINITE);
	//void Unlock();
	// saves the connection context
	redisContext* Context;
		// ensures there is a connection to redis
	bool Connect();

	//Executes the command towards Redis
	redisReply* Command(char* CommandText);
	//Prepares the command for Redis
	bool Authenticate();
};

typedef CRedis* PRedis;

class CRedisPool : public queue< PRedis >, private CSemaphore, CMutex
{
public:
	CRedisPool();
	virtual ~CRedisPool();
	void Add(PRedis);
	PRedis Get(void);
	unsigned __int64 Fill(unsigned __int64 p_HowMany = 1);
	unsigned __int64 Empty();
	// Assign either a Customer or Carrier Trunk
	int FraudANI(int CompanyID, LPCSTR ANI, unsigned long long Thread = 0);

	CSentinel Sentinel;
	char* SetFraudANISHA(char* SHA);

	char* SetAuthenticationPassword(char* AuthenticationPassword);

	char m_FraudANISHA[41];

	char m_AuthenticationPassword[129];

};


typedef CRedisPool* PRedisPool;