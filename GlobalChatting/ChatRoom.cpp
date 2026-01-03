#ifndef CHATROOM_CPP
#define CHATROOM_CPP

#include <string>
#include <vector>
#include <sstream>

struct ChatRoom {
    int id;
    std::string name;
    std::string description;

    ChatRoom(int _id, std::string _name, std::string _desc)
        : id(_id), name(_name), description(_desc) {
    }
};

class ChatRoomManager {
private:
    std::vector<ChatRoom> rooms;

public:
    ChatRoomManager() {
        // Initialize default chat rooms
        rooms.push_back(ChatRoom(1, "General", "General discussion room"));
        rooms.push_back(ChatRoom(2, "Gaming", "Talk about games and gaming"));
        rooms.push_back(ChatRoom(3, "Technology", "Tech news and discussions"));
        rooms.push_back(ChatRoom(4, "Music", "Share and discuss music"));
        rooms.push_back(ChatRoom(5, "Sports", "Sports talk and updates"));
    }

    std::string list_rooms() {
        std::stringstream ss;
        ss << "Available Chat Rooms:\n";
        ss << "====================\n";

        for (const auto& room : rooms) {
            ss << room.id << ". " << room.name << " - " << room.description << "\n";
        }

        return ss.str();
    }

    std::string get_room_name(int room_id) {
        for (const auto& room : rooms) {
            if (room.id == room_id) {
                return room.name;
            }
        }
        return "";
    }

    bool is_valid_room(int room_id) {
        return room_id >= 1 && room_id <= rooms.size();
    }

    void add_room(std::string name, std::string description) {
        int new_id = rooms.size() + 1;
        rooms.push_back(ChatRoom(new_id, name, description));
    }

    int get_room_count() {
        return rooms.size();
    }
};

#endif