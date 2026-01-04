#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <string>
#include <algorithm>
#include "database.cpp"
#include "chatroom.cpp"

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

struct ClientInfo {
    SOCKET socket;
    int id;
    std::string username;
    std::string room;
    int user_db_id;
};

std::map<int, ClientInfo> connected_clients;
std::mutex clients_mutex;
std::mutex cout_mutex;
int client_count = 0;

ChatRoomManager room_manager;
DatabaseManager db_manager;

void broadcast_to_room(const std::string& room_name, const std::string& message, int sender_id) {
    std::lock_guard<std::mutex> lock(clients_mutex);

    for (auto& pair : connected_clients) {
        if (pair.second.room == room_name && pair.second.id != sender_id) {
            send(pair.second.socket, message.c_str(), message.length(), 0);
        }
    }
}

void handle_client(SOCKET client_fd, int client_id) {
    char buffer[BUFFER_SIZE];
    ClientInfo client_info;
    client_info.socket = client_fd;
    client_info.id = client_id;

    // Authentication process
    const char* auth_prompt = "=== Chat Server Authentication ===\nEnter username: ";
    send(client_fd, auth_prompt, strlen(auth_prompt), 0);

    memset(buffer, 0, BUFFER_SIZE);
    int valread = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (valread <= 0) {
        closesocket(client_fd);
        return;
    }
    std::string username(buffer);

    // Check if username exists in database
    if (!db_manager.username_exists(username)) {
        const char* error_msg = "Error: Username not found in database. Please contact administrator.\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Authentication failed: Username '" << username << "' not found" << std::endl;
        }
        closesocket(client_fd);
        return;
    }

    // Ask for password
    const char* pass_prompt = "Enter password: ";
    send(client_fd, pass_prompt, strlen(pass_prompt), 0);

    memset(buffer, 0, BUFFER_SIZE);
    valread = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (valread <= 0) {
        closesocket(client_fd);
        return;
    }
    std::string password(buffer);

    // Authenticate user
    if (!db_manager.authenticate_user(username, password)) {
        const char* error_msg = "Error: Invalid password or account is inactive.\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Authentication failed: Invalid password for user '" << username << "'" << std::endl;
        }
        closesocket(client_fd);
        return;
    }

    // Update last login and log session
    int user_db_id = db_manager.get_user_id(username);
    db_manager.update_last_login(username);
    db_manager.log_session(user_db_id);

    client_info.username = username;
    client_info.user_db_id = user_db_id;

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "User '" << username << "' authenticated successfully (Client #" << client_id << ")" << std::endl;
    }

    const char* success_msg = "Authentication successful!\n\n";
    send(client_fd, success_msg, strlen(success_msg), 0);

    // Send available rooms
    std::string rooms_list = room_manager.list_rooms();
    send(client_fd, rooms_list.c_str(), rooms_list.length(), 0);

    // Ask for room choice
    const char* room_prompt = "\nEnter room number to join: ";
    send(client_fd, room_prompt, strlen(room_prompt), 0);

    memset(buffer, 0, BUFFER_SIZE);
    valread = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (valread <= 0) {
        closesocket(client_fd);
        return;
    }

    int room_choice = atoi(buffer);
    client_info.room = room_manager.get_room_name(room_choice);

    if (client_info.room.empty()) {
        const char* error_msg = "Invalid room choice. Disconnecting...";
        send(client_fd, error_msg, strlen(error_msg), 0);
        closesocket(client_fd);
        return;
    }

    // Add client to connected clients
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        connected_clients[client_id] = client_info;
    }

    // Notify room
    std::string join_msg = client_info.username + " has joined the room!";
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Client #" << client_id << " (" << client_info.username
            << ") joined " << client_info.room << std::endl;
    }

    broadcast_to_room(client_info.room, join_msg, client_id);

    std::string welcome = "Welcome to " + client_info.room + "! Type your messages below (type 'exit' to quit):\n";
    send(client_fd, welcome.c_str(), welcome.length(), 0);

    // Communication loop
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        valread = recv(client_fd, buffer, BUFFER_SIZE, 0);

        if (valread <= 0) {
            break;
        }

        std::string message(buffer);

        // Check for exit command
        if (message == "exit") {
            const char* goodbye = "Goodbye!";
            send(client_fd, goodbye, strlen(goodbye), 0);
            break;
        }

        // Broadcast message to room
        std::string full_message = "[" + client_info.username + "]: " + message;
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << client_info.room << " - " << full_message << std::endl;
        }

        broadcast_to_room(client_info.room, full_message, client_id);

        // Acknowledge to sender
        // std::string ack = "Message sent to " + client_info.room;
        // send(client_fd, ack.c_str(), ack.length(), 0);
    }

    // Remove client and notify room
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        connected_clients.erase(client_id);
    }

    std::string leave_msg = client_info.username + " has left the room.";
    broadcast_to_room(client_info.room, leave_msg, client_id);

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Client #" << client_id << " (" << client_info.username
            << ") disconnected from " << client_info.room << std::endl;
    }

    closesocket(client_fd);
}

int main() {
    WSADATA wsaData;
    SOCKET server_fd = INVALID_SOCKET;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Connect to database
    if (!db_manager.connect()) {
        std::cerr << "Failed to connect to database. Exiting..." << std::endl;
        return -1;
    }

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return -1;
    }

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return -1;
    }

    // Configure address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    // Listen for connections
    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    std::cout << "\n=== Chat Server Started ===" << std::endl;
    std::cout << "Server listening on port " << PORT << std::endl;
    std::cout << "Database: Connected" << std::endl;
    std::cout << "Available rooms:" << std::endl;
    std::cout << room_manager.list_rooms() << std::endl;
    std::cout << "Waiting for authenticated clients to connect...\n" << std::endl;

    std::vector<std::thread> client_threads;

    // Accept multiple clients
    while (true) {
        SOCKET client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);

        if (client_fd == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        client_count++;
        client_threads.push_back(std::thread(handle_client, client_fd, client_count));
        client_threads.back().detach();
    }

    // Cleanup
    closesocket(server_fd);
    WSACleanup();

    return 0;
}