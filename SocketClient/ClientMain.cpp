#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <thread>
#include <chrono>
#include <Windows.h>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <ws2tcpip.h>  // Necesario para inet_pton

int main() {
	// Inicializar Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Error al inicializar Winsock" << std::endl;
		return -1;
	}

	// Crear un socket
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Error al crear el socket" << std::endl;
		WSACleanup();
		return -1;
	}

	// Configurar la dirección del servidor
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345); // Puerto 12345
	if (inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr)) != 1) {
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
		// Enviar datos al servidor
		DWORD pid = GetCurrentProcessId();

		// Convertir el PID a una cadena
		std::stringstream ss;
		ss << "Hola desde el cliente con PID: " << pid << std::endl;
		std::string message = ss.str();

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
