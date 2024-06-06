#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <thread>
#include <chrono>
#include <Windows.h>
#include <sstream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <ws2tcpip.h>  // Necesario para inet_pton
#include "stdafx.h"

#include "ConstantesGenerales.h"
#include "Logging.h"
#include "Queue.h"

Queue< std::string* > m_LogQueue;

// Definición de la estructura de datos
struct Message {
	int operationID;
	std::string data;

	// Constructores
	Message() : operationID(0), data("") {}
	Message(int id, const std::string& str) : operationID(id), data(str) {}
};
Message DeserializeMessage(const std::vector<char>& data);
void HandleClient(SOCKET clientSocket);

inline void QueueAdd(char* pData) { m_LogQueue.Add((std::string*)(new std::string(pData))); }
inline std::string* QueueWait()     { return (std::string*)m_LogQueue.Wait(); }