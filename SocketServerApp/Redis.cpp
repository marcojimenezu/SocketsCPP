#include "stdafx.h"
#include "Redis.h"
#include "Logging.h"

extern CLogging *pLogger;

CConnectionInformation::CConnectionInformation()
	: Port(0)
{
	memset(this->Host, 0, sizeof(this->Host));
}

CConnectionInformation::~CConnectionInformation()
{

}

CSentinel::CSentinel()
{

}

CSentinel::~CSentinel()
{

}

// Add a new Sentinel to the list
void CSentinel::AddSentinel(char* Host, unsigned long Port)
{
	CConnectionInformation l_ConnectionInformation;

	//strcpy_s(l_ConnectionInformation.Host, _countof(l_ConnectionInformation.Host), Host);
	strcpy_s(l_ConnectionInformation.Host, Host);
	l_ConnectionInformation.Port = Port;

	this->ConnectionInformation.push_back(l_ConnectionInformation);
}

// Task the Sentinel with getting the ip of the master redis server
bool CSentinel::GetConnectionInformation(CConnectionInformation& p_RedisConnectionInformation, unsigned long long Thread)
{
	bool l_result = false;

	unsigned long l_i = 1;

	for (vector<CConnectionInformation>::iterator l_pSentinelConnectionInformation = this->ConnectionInformation.begin()
		; l_pSentinelConnectionInformation != this->ConnectionInformation.end()
		; ++l_pSentinelConnectionInformation, ++l_i)
	{
		// Ask the sentinel for the master redis

		//connect to one sentinel

		//RedisDebugLog
		pLogger->Log(ELEVEL_1
			, "Thread[%8llu] RedisDebugLog: %s {"
			" Trying Sentinel %s:%lu"
			" } "
			, Thread
			, __FUNCTION__
			, l_pSentinelConnectionInformation->Host
			, l_pSentinelConnectionInformation->Port
			);

		timeval timeout = { 0, 500000 }; // 0.5 seconds
		redisContext* l_SentinelContext = redisConnectWithTimeout(l_pSentinelConnectionInformation->Host, l_pSentinelConnectionInformation->Port, timeout);

		if ((l_SentinelContext != nullptr) && (l_SentinelContext->err == REDIS_OK))
		{
			static char l_commandtext[1024];

			// Ask the Connected Sentinel for the master
			sprintf_s(l_commandtext, "sentinel get-master-addr-by-name %s"
				, "redis02"
				);

			//RedisDebugLog
			pLogger->Log(ELEVEL_1
				, "Thread[%8llu] RedisDebugLog: %s {"
				" Sending command \"%s\""
				" } "
				, Thread
				, __FUNCTION__
				, l_commandtext
				);

			redisReply* l_redisReply = (redisReply*)redisCommand(l_SentinelContext, l_commandtext);

			// if there was a response
			if (l_redisReply)
			{
				if ( (l_redisReply->type == REDIS_REPLY_ARRAY)	// if not the there was a problem
					&& (l_redisReply->elements > 1)				// it ust be two at least
					&& (l_redisReply->element[0]->type == REDIS_REPLY_STRING)
					&& (l_redisReply->element[1]->type == REDIS_REPLY_STRING) )
				{
						strcpy_s(p_RedisConnectionInformation.Host, l_redisReply->element[0]->str);
						p_RedisConnectionInformation.Port = atoi(l_redisReply->element[1]->str);
						// tell we are all good
						l_result = true;

						//RedisDebugLog
						pLogger->Log(ELEVEL_1
							, "Thread[%8llu] RedisDebugLog: %s {"
							" Result: %s:%lu"
							" } "
							, Thread
							, __FUNCTION__
							, p_RedisConnectionInformation.Host
							, p_RedisConnectionInformation.Port
							);

				}
				else
					//RedisDebugLog
					pLogger->Log(ELEVEL_1
					, "Thread[%8llu] RedisDebugLog: %s {"
					" %s invalid type result: %li"
					" } "
					, Thread
					, __FUNCTION__
					, l_commandtext
					, l_redisReply->type
					);

				freeReplyObject(l_redisReply);
			}

			if (l_SentinelContext != nullptr)
				redisFree(l_SentinelContext);

		} // if Context

		if (l_result == true)	// we already got the data
			break;				// then get the heck out of the loop

	}  // for vector

	return l_result;
}

CRedis::CRedis()
	: Context(nullptr)
	, PRedisPool(nullptr)
	, PoolObjectID(0)
	, Thread(0)
	//, m_mutex(nullptr)
{
	//this->m_mutex = CreateMutex(NULL, FALSE, NULL);
}


CRedis::~CRedis()
{
	//if (this->Context)
	//	redisFree(this->Context);

	//if (this->m_mutex)
	//	CloseHandle(this->m_mutex);
}

//bool CRedis::Lock(DWORD dwMilliseconds)
//{
//	return (WaitForSingleObject(this->m_mutex, dwMilliseconds) == WAIT_OBJECT_0);
//}
//
//void CRedis::Unlock()
//{
//	ReleaseMutex(this->m_mutex);
//}

// ensures there is a connection to redis
bool CRedis::Connect()
{
	// Disconnect if there are errors
	if ((this->Context != nullptr) && (this->Context->err != REDIS_OK))
	{
		redisFree(this->Context);
		this->Context = nullptr;
	} // if ((this->Context != nullptr) && (this->Context->err != REDIS_OK))

	// Connect if there is not an open connection
	if (this->Context == nullptr)
	{
		CConnectionInformation l_ConnectionInformation;

		if (this->PRedisPool->Sentinel.GetConnectionInformation(l_ConnectionInformation, this->Thread))
		{
			timeval timeout = { 0, 500000 }; // 0.5 seconds
			this->Context = redisConnectWithTimeout(l_ConnectionInformation.Host, l_ConnectionInformation.Port, timeout);
		}

		if ((this->Context != nullptr) && (this->Context->err == REDIS_OK))
		{
			redisEnableKeepAlive(this->Context);

			if (!this->Authenticate())
			{
				redisFree(this->Context);
				this->Context = nullptr;
			}
		}
	}

	// Checks for open context with no errors.
	return ((this->Context != nullptr) && (this->Context->err == REDIS_OK));
}

//Executes the command towards Redis
redisReply* CRedis::Command(char* CommandText)
{
	redisReply* l_redisReply = nullptr;

	//Try three times if either the conection failover mechanism can't get a hold of a redis server or if the command returns empty

	//Try ONLY ONCE TEMPORARILY until we learn how long it takes to timeout once the REDIS part gets overloaded.
	//for (unsigned int i = 0; ( (i < 3) && (l_redisReply == nullptr) ); ++i)
	{
		if (this->Connect())
		{
			//RedisDebugLog
			//pLogger->Log(ELEVEL_1
			//	, "Thread[%8llu] RedisDebugLog: %s {"
			//	" CommandText \"%s\""
			//	" PoolObjectID %llu"
			//	" } "
			//	, this->Thread
			//	, __FUNCTION__
			//	, CommandText
			//	, this->PoolObjectID
			//	);

			l_redisReply = (redisReply*)redisCommand(this->Context, CommandText);

			//RedisDebugLog
			pLogger->Log(ELEVEL_1
				, "Thread[%8llu] RedisDebugLog: %s {"
				" CommandText \"%s\""
				" l_redisReply = 0x%p"
				" l_redisReply->type = %li"
				" l_redisReply->integer = %lli"
				" l_redisReply->len = %li"
				" l_redisReply->str = \"%s\""
				" l_redisReply->elements = %llu"
				" PoolObjectID %llu"
				" } "
				, this->Thread
				, __FUNCTION__
				, CommandText
				, (void*)l_redisReply
				, (l_redisReply != nullptr) ? l_redisReply->type : 0
				, (l_redisReply != nullptr) ? l_redisReply->integer : 0
				, (l_redisReply != nullptr) ? l_redisReply->len : 0
				, (l_redisReply != nullptr) && (l_redisReply->len) ? l_redisReply->str : ""
				, (l_redisReply != nullptr) ? (unsigned long long)l_redisReply->elements : 0ull
				, this->PoolObjectID
				);

			if (l_redisReply == nullptr)
				pLogger->Log(ELEVEL_1
					, "Thread[%8llu] RedisDebugLog: %s {"
					" ERROR: No Response/Timeout!"
					" PoolObjectID %llu"
					" } "
					, this->Thread
					, __FUNCTION__
					, this->PoolObjectID
					);

		}
	}

	return l_redisReply;
}

bool CRedis::Authenticate()
{
	bool l_result = false;
	redisReply* l_redisReply = nullptr;

	const char l_AuthenticationCommand[] = { "auth" };

	static char l_commandtext[_countof(l_AuthenticationCommand) + _countof(this->PRedisPool->m_AuthenticationPassword) + (2 * sizeof(char))];

	sprintf_s(l_commandtext, "%s %s"
		, l_AuthenticationCommand
		, this->PRedisPool->m_AuthenticationPassword
		);

	l_redisReply = this->Command(l_commandtext);

	if (l_redisReply)
	{
		if (l_redisReply->type == REDIS_REPLY_STATUS)  // if not the there was a problem
		{
			l_result = (l_redisReply->len == 2); /* "OK" */
		}

		freeReplyObject(l_redisReply);
	}

	return true;
}

// Assign either a Customer or Carrier Trunk
int CRedis::FraudANI(int CompanyID, LPCSTR ANI)
{
	char* l_SHA = this->PRedisPool->m_FraudANISHA;

	int l_result = 0;

	//RedisDebugLog
	pLogger->Log(ELEVEL_1
		, "Thread[%8llu] RedisDebugLog: %s {"
		" SHA %s"
		" PoolObjectID %llu"
		" } "
		, this->Thread
		, __FUNCTION__
		, l_SHA
		, this->PoolObjectID
		);

	// No lock needed. We are now in a synchronized pool
	// if (this->Lock( (WaitLonger) ? 10000 : 1000))
	{

		redisReply* l_redisReply = nullptr;

		char l_commandtext[1024];

		SYSTEMTIME l_systemtime;
		GetSystemTime(&l_systemtime);

		sprintf_s(l_commandtext, "evalsha %s 0 %li:%s:%s %s %li %hu,%hu,%hu,%hu"
			, l_SHA
			, CompanyID		// CompanyID
			, "none"		// Customer
			, "ANI"			// Operation
			, ANI			// ANI
			, 4				// Precision
			, l_systemtime.wYear, l_systemtime.wMonth, l_systemtime.wDay, l_systemtime.wHour	// timestamp
			);

		l_redisReply = this->Command(l_commandtext);

		if (l_redisReply)
		{
			if (l_redisReply->type == REDIS_REPLY_STRING)  // if not there was a problem
			{
				// string lenght zero means no fraud detected
				//l_result = (int)l_redisReply->len;
				l_result = (int)atoi(l_redisReply->str ? l_redisReply->str : "");
			}
			else
				//RedisDebugLog
				pLogger->Log(ELEVEL_1
				, "Thread[%8llu] RedisDebugLog: %s {"
				" redisReply unexpected type"
				" PoolObjectID %llu"
				" } "
				, this->Thread
				, __FUNCTION__
				, this->PoolObjectID
				);

			freeReplyObject(l_redisReply);
		}

		// No lock needed. We are now in a synchronized pool
		// this->Unlock();

	} // if (this->Lock(200))

	return l_result;
}

CRedisPool::CRedisPool()
{
	memset(this->m_FraudANISHA, 0, sizeof(this->m_FraudANISHA));

	memset(this->m_AuthenticationPassword, 0, sizeof(this->m_AuthenticationPassword));
}

CRedisPool::~CRedisPool()
{
	this->Empty();
}

void CRedisPool::Add(PRedis pRedis)
{
	Lock();

	try 
	{
		push(pRedis);
	}
	catch (...) 
	{

	}

	Unlock();

	Post(); // Wake up any waiting threads.

}

PRedis CRedisPool::Get(void)
{
	PRedis pRedis(NULL);

	Wait(); // Wait for something to show up.

	Lock();

	try 
	{
		if (!empty())
		{
			pRedis = front();
			pop();
		}
		else
		{
			pRedis = NULL;
		}
	}
	catch (...) 
	{

	}

	Unlock();

	return pRedis;

}

unsigned __int64 CRedisPool::Fill(unsigned __int64 p_HowMany)
{
	unsigned __int64 l_Added = 0;

	for (unsigned __int64 i = 0; i < p_HowMany; ++i)
	{
		PRedis l_Redis = new CRedis;

		if (l_Redis)
		{
			l_Redis->PRedisPool = this;
			l_Redis->PoolObjectID = i + 1;

			this->Add(l_Redis);
			++l_Added;
		}

	}

	return (l_Added);
}

unsigned __int64  CRedisPool::Empty()
{
	unsigned __int64 l_result = this->size();

	while (this->empty() == false)
	{
		PRedis l_Redis = (PRedis)this->Get();
		
		if (l_Redis)
			delete l_Redis;
	}

	return l_result;
}

char* CRedisPool::SetFraudANISHA(char* SHA)
{
	strcpy_s(this->m_FraudANISHA, SHA);

	return SHA;
}

char* CRedisPool::SetAuthenticationPassword(char* AuthenticationPassword)
{
	strcpy_s(this->m_AuthenticationPassword, AuthenticationPassword);

	return AuthenticationPassword;
}

// Assign either a Customer or Carrier Trunk
int CRedisPool::FraudANI(int CompanyID, LPCSTR ANI, unsigned long long Thread)
{
	int l_result = 0;

	PRedis l_Redis = this->Get();

	if (l_Redis)
	{
		l_Redis->Thread = Thread;

		l_result = l_Redis->FraudANI(CompanyID, ANI);

		this->Add(l_Redis);
	}

	return l_result;
}
