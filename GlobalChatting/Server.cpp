#include <iostream>
#include <string>
#include <winsock2.h>
#include <thread> // Required for std::thread

#pragma comment(lib, "ws2_32.lib")

// This function will be executed by each client thread
void handle_client(SOCKET clientSocket) {
    std::cout << "Client connected on thread " << std::endl;

    char buffer[1024];

    // Communication loop for this client
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            std::cout << "Client on thread disconnected." << std::endl;
            break; // Exit the loop
        }

        std::cout << "[Client]: " << buffer << std::endl;

        // For simplicity, this server just echoes the message back.
        // You can add your custom logic here.
        std::string message = "Echo: ";
        message += buffer;
        send(clientSocket, message.c_str(), message.length() + 1, 0);
    }

    closesocket(clientSocket); // Close the socket for this client
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    listen(serverSocket, 5);
    std::cout << "Server is awake and listening on port 8080..." << std::endl;

    // ----- Main accept loop -----
    // This loop runs forever, accepting new clients.
    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue; // Continue to the next iteration to accept another client
        }

        // Create a new thread to handle this client
        // The handle_client function will now run on a separate thread
        std::thread clientThread(handle_client, clientSocket);
        clientThread.detach(); // Detach the thread to run independently
    }
    // ----- Loop ends (in practice, it doesn't) -----

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}