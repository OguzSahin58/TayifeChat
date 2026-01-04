#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

bool running = true;

void receive_messages(SOCKET sock) {
    char buffer[BUFFER_SIZE];

    while (running) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = recv(sock, buffer, BUFFER_SIZE, 0);

        if (valread > 0) {
            std::string message(buffer);

            // Only show messages from other users (not acknowledgments)
            if (message.find("Message sent to") == std::string::npos) {
                std::cout << "\r" << std::string(50, ' ') << "\r"; // Clear current line
                std::cout << buffer << std::endl;
                std::cout << "You: " << std::flush;
            }
        }
        else if (valread == 0) {
            std::cout << "\nServer disconnected" << std::endl;
            running = false;
            break;
        }
        else {
            if (running) {
                std::cerr << "\nReceive failed: " << WSAGetLastError() << std::endl;
            }
            running = false;
            break;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = { 0 };

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return -1;
    }

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        std::cerr << "Socket creation error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return -1;
    }

    // Configure server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // Connect to server
    std::cout << "Connecting to chat server..." << std::endl;
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    std::cout << "Connected to chat server!\n" << std::endl;

    // Receive authentication prompt
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    std::cout << buffer;

    // Send username
    std::string username;
    std::getline(std::cin, username);
    send(sock, username.c_str(), username.length(), 0);

    // Receive password prompt or error
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    std::cout << buffer;

    // Check if authentication failed (username not found)
    if (strstr(buffer, "Error") != NULL) {
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    // Send password
    std::string password;
    std::getline(std::cin, password);
    send(sock, password.c_str(), password.length(), 0);

    // Receive authentication result
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    std::cout << buffer;

    // Check if authentication failed (wrong password)
    if (strstr(buffer, "Error") != NULL) {
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    // Receive room list and prompt together
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = 0;
    std::string full_message;

    // Keep receiving until we get the room prompt
    while (true) {
        bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            full_message += buffer;

            // Check if we received the room prompt
            if (full_message.find("Enter room number") != std::string::npos) {
                break;
            }
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    std::cout << full_message;

    // Send room choice
    std::string room_choice;
    std::getline(std::cin, room_choice);
    send(sock, room_choice.c_str(), room_choice.length(), 0);

    // Receive welcome or error message
    memset(buffer, 0, BUFFER_SIZE);
    int valread = recv(sock, buffer, BUFFER_SIZE, 0);
    std::cout << "\n" << buffer << std::endl;

    // Check if connection was rejected
    if (strstr(buffer, "Invalid") != NULL || strstr(buffer, "Disconnecting") != NULL) {
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    // Start receiving thread
    std::thread receive_thread(receive_messages, sock);

    // Main sending loop
    std::string message;
    while (running) {
        std::cout << "You: ";
        std::getline(std::cin, message);

        if (message.empty()) {
            continue;
        }

        // Send message
        send(sock, message.c_str(), message.length(), 0);

        // Check if exit command
        if (message == "exit") {
            running = false;
            break;
        }
    }

    // Wait for receive thread to finish
    if (receive_thread.joinable()) {
        receive_thread.join();
    }

    // Close socket
    closesocket(sock);
    WSACleanup();

    std::cout << "Disconnected from server" << std::endl;

    return 0;
}