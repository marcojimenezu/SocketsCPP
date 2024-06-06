#include <iostream>
#include <hiredis\hiredis.h>
#include "SocketServerApp.h"


const int MAX_CLIENTS = 500;  // Número máximo de clientes permitidos
int connectedClients = 0;  // Variable contadora de clientes conectados
bool g_KeepRunning = true;
CLogging *pLogger;

Message DeserializeMessage(const std::vector<char>& data) {
	// Extraer el operationID del primer entero en los datos
	int operationID;
	std::memcpy(&operationID, data.data(), sizeof(int));

	// Los datos restantes son la cadena de caracteres
	std::string dataString(data.begin() + sizeof(int), data.end());

	Message message;
	message.operationID = operationID;
	message.data = dataString;
	return message;
}

DWORD WINAPI m_HiloQueueMensajes(LPVOID lpParam) {
	while (true) {
		// Realizar acciones dentro del ciclo infinito
		std::string* str = QueueWait();
		//std::cout << "Mensaje del cliente: " << str->data() << std::endl;
		// Introducir un pequeño retraso para evitar consumir demasiados recursos de la CPU
		std::cout << "Mensaje del cliente: " << str->c_str() << std::endl;
		pLogger->Log(ELEVEL_1, "***************************************");
		pLogger->LogNT("Mensaje de pruebas",123456789);
		Sleep(300);
	}

	// El hilo nunca llegará a este punto, pero aquí puedes retornar un valor de salida si es necesario
	return 0;
}

void HandleClient(SOCKET clientSocket) {

	int bytesReceived = 0;
	char buffer[1024];
	const char* responseMessage = "¡Mensaje recibido por el servidor!";

	std::vector<char> receivedData(MAX_BUFFER_SIZE);
	std::cout << "Cliente conectado" << std::endl;

	while (true) {
		// Esperar a que el cliente envíe datos
		bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) { // cuando hubo un error de comunicación con el cliente
			std::cerr << "Error al recibir datos del cliente o conexión cerrada" << std::endl;
			break;
		}
		else { // Si hubo comunicación con el cliente y se recibió el mensaje
			buffer[bytesReceived] = '\0';
			QueueAdd(buffer);

			// Enviar una respuesta al cliente
			
			if (send(clientSocket, responseMessage, strlen(responseMessage), 0) == SOCKET_ERROR) {
				std::cerr << "Error al enviar respuesta al cliente" << std::endl;
				break; // Salir del bucle y terminar el hilo
			}
		}
	}

	// Cerrar la conexión con el cliente
	closesocket(clientSocket);
}

int hiredisConnection(){
	timeval timeout = { 60, 0 };
	//redisContext* context = redisConnectWithTimeout("logstream.wasras.com", 6379, timeout); // Auris
	//redisContext* context = redisConnectWithTimeout("172.25.25.19", 6379, timeout); // Auris
	//redisContext* context = redisConnectWithTimeout("192.168.68.131", 6379, timeout); // Linux
	redisContext* context = redisConnectWithTimeout("redis-17722.c124.us-central1-1.gce.redns.redis-cloud.com", 17722, timeout); // Redis-cloud
	if (context == nullptr || context->err) {
		if (context) {
			std::cerr << "Error: " << context->errstr << std::endl;
			redisFree(context);
		}
		else {
			std::cerr << "Can't allocate Redis context" << std::endl;
		}
		getchar();
		return 1;
	}

	redisReply* reply = (redisReply*)redisCommand(context, "AUTH %s", "speTwSQNDa6S14q3GiW262OgL1PtwxbJ");
	if (reply == nullptr) {
		std::cerr << "AUTH command failed" << std::endl;
		redisFree(context);
		getchar();
		return 1;
	}
	std::cout << "AUTH: " << reply->str << std::endl;
	freeReplyObject(reply);
	
	reply = (redisReply*)redisCommand(context, "SET %s %s", "Marco", "Testing2");
	if (reply == nullptr) {
		std::cerr << "SET command failed" << std::endl;
		redisFree(context);
		getchar();
		return 1;
	}
	std::cout << "SET: " << reply->str << std::endl;
	freeReplyObject(reply);

	reply = (redisReply*)redisCommand(context, "GET %s", "Marco");
	if (reply->type == REDIS_REPLY_STRING) {
		std::cout << "GET: " << reply->str << std::endl;
	}
	else {
		std::cerr << "GET command failed" << std::endl;
	}
	freeReplyObject(reply);
	redisFree(context);

	getchar();
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	char szINIFile[256];
	char szTitle[256];

	g_KeepRunning = true;

	hiredisConnection();
	return 0;

	// Inicializar Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Error al inicializar Winsock" << std::endl;
		return -1;
	}

	// Crear un socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Error al crear el socket" << std::endl;
		WSACleanup();
		return -1;
	}

	// Configurar la dirección del servidor
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345); // Puerto 12345
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	// Enlazar el socket a la dirección
	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Error al enlazar el socket" << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return -1;
	}

	// Poner el socket en modo de escucha
	if (listen(serverSocket, 5) == SOCKET_ERROR) {
		std::cerr << "Error en listen" << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return -1;
	}

	std::cout << "Servidor esperando conexiones..." << std::endl;

	DWORD dwThreadId;
	HANDLE hQueueMonitorThread = CreateThread(NULL, 0, m_HiloQueueMensajes, nullptr, 0, &dwThreadId);
	if (hQueueMonitorThread == NULL) {
		std::cerr << "Error al crear el hilo." << std::endl;
		return 1;
	}

	sprintf(szINIFile, "C:\\Debug\\ConfigIni.ini");	
	sprintf(szTitle, "Traces\\WSSSApp");
	pLogger = new CLogging(false, true, szTitle, szINIFile);

	sockaddr_in clientAddr;
	int clientAddrLen = 0;

	while (g_KeepRunning) {
		// Aceptar una conexión entrante
		//sockaddr_in clientAddr;
		clientAddrLen = sizeof(clientAddr);

		SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Error al aceptar la conexión" << std::endl;
			continue;  // Intentar aceptar la siguiente conexión
		}

		std::cout << "Cliente conectado" << std::endl;

		// Incrementar la variable contadora de clientes
		connectedClients++;

		// Crear un hilo para manejar el cliente
		std::thread(HandleClient, clientSocket).detach();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	TerminateThread(m_HiloQueueMensajes, 0);
	CloseHandle(hQueueMonitorThread);

	// Cerrar los sockets
	closesocket(serverSocket);
	WSACleanup();

	return 0;
}