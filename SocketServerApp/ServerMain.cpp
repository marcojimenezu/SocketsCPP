#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

const int MAX_CLIENTS = 500;  // Número máximo de clientes permitidos
int connectedClients = 0;  // Variable contadora de clientes conectados
bool g_KeepRunning = true;

void HandleClient(SOCKET clientSocket) {
	char buffer[1024];

	std::cout << "Cliente conectado" << std::endl;

	while (true) {
		// Esperar a que el cliente envíe datos
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
			// Error o conexión cerrada por el cliente
			std::cerr << "Error al recibir datos del cliente o conexión cerrada" << std::endl;
			break; // Salir del bucle y terminar el hilo
		}
		else {
			buffer[bytesReceived] = '\0';
			std::cout << "Mensaje del cliente: " << buffer << std::endl;

			// Enviar una respuesta al cliente
			const char* responseMessage = "¡Mensaje recibido por el servidor!";
			if (send(clientSocket, responseMessage, strlen(responseMessage), 0) == SOCKET_ERROR) {
				std::cerr << "Error al enviar respuesta al cliente" << std::endl;
				break; // Salir del bucle y terminar el hilo
			}
		}
	}

	// Cerrar la conexión con el cliente
	closesocket(clientSocket);
}

int main() {

	g_KeepRunning = true;

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

	while (g_KeepRunning) {
		// Aceptar una conexión entrante
		sockaddr_in clientAddr;
		int clientAddrLen = sizeof(clientAddr);

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

	// Cerrar los sockets
	closesocket(serverSocket);
	WSACleanup();

	return 0;
}