#include <iostream>
#include <string> // Added for std::string
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Connect to localhost

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to the server! You can start chatting. Type 'exit' to quit." << std::endl;

    char buffer[1024];
    std::string input;

    // ----- Main communication loop starts here -----
    while (true) {
        // 1. Get client user's message and send it
        std::cout << "Client: ";
        std::getline(std::cin, input);
        send(clientSocket, input.c_str(), input.length() + 1, 0); // +1 to include null terminator

        // Check if the client wants to exit
        if (input == "exit") {
            break;
        }

        // 2. Receive reply from server
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            std::cout << "Server disconnected." << std::endl;
            break; // Exit loop if server disconnects or an error occurs
        }

        std::cout << "Server: " << buffer << std::endl;

        // Check if the server sent an exit message
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
    }
    // ----- Loop ends -----

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}