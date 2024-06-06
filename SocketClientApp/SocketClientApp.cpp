#include "SocketClientApp.h"
#include "stdafx.h"

std::vector<char> SerializeMessage(const Message& message) {
	std::vector<char> buffer(sizeof(int)+message.data.size());
	int* pOperationID = reinterpret_cast<int*>(&buffer[0]);
	*pOperationID = message.operationID;
	std::copy(message.data.begin(), message.data.end(), buffer.begin() + sizeof(int));
	return buffer;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// inicializáción de operación a ejecutar en el server

	int operationID = 1; // Operación para log de mensajes
	DWORD pid = GetCurrentProcessId();
	std::stringstream ss;
	ss << "Hola desde el cliente con PID: " << pid << std::endl;
	std::string message = ss.str();

	Message messageToSend(operationID, message);
	std::vector<char> serializedMessage = SerializeMessage(messageToSend);

	// Fin inicialización de operación

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Error al inicializar Winsock" << std::endl;
		return -1;
	}	

	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Error al crear el socket" << std::endl;
		WSACleanup();
		return -1;
	}

	// Configurar la dirección del servidor
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345);
	//if (inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr)) != 1) {
	if (inet_pton(AF_INET, "192.168.68.119", &(serverAddr.sin_addr)) != 1) {
		std::cerr << "Error al convertir la dirección IP" << std::endl;
		closesocket(clientSocket);
		WSACleanup();
		return -1;
	}

	// Conectar al servidor
	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Error al conectar al servidor" << std::endl;
		closesocket(clientSocket);
		WSACleanup();
		return -1;
	}

	for (int i = 0; i < 100; ++i) {
		// Imprimir el mensaje
		std::cout << message << std::endl;
		if (send(clientSocket, message.c_str(), strlen(message.c_str()), 0) == SOCKET_ERROR) {
			std::cerr << "Error al enviar datos al servidor" << std::endl;
			closesocket(clientSocket);
			WSACleanup();
			return -1;
		}

		// Esperar respuesta del servidor
		char buffer[1024];
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived == SOCKET_ERROR) {
			std::cerr << "Error al recibir datos del servidor" << std::endl;
		}
		else if (bytesReceived == 0) {
			std::cerr << "Conexión cerrada por el servidor" << std::endl;
		}
		else {
			buffer[bytesReceived] = '\0';
			std::cout << "Respuesta del servidor: " << buffer << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		i = 0; // Mantener el socket corriendo infinitamente
	}

	// Cerrar el socket después de completar las 100 solicitudes
	closesocket(clientSocket);
	WSACleanup();

	return 0;
}

