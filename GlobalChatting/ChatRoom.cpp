#pragma once
#include <iostream>
#include <string>
#include <map>
using namespace std;

class ChatRoom {      
public:            
    map<int, string> chatRooms = { 
        {1, "Main"},
        {2, "Gaming"}, 
        {3, "CS:GO"}, 
        {4, "Valorant"}, 
        {5, "League Of Legends"}, 
        {6, "Clash Royale"}, 
        {7, "Clash of Clans"}, 
        {8, "Only Spotify"}
    };
    
    string getRoomsListString() const {
        // Start with a header
        string roomList = "--- Available Chat Rooms ---\n";

        for (const auto& pair : chatRooms) {
            // Concatenate the ID (converted to string), space, name, and newline
            roomList += "[" + to_string(pair.first) + "] " + pair.second + "\n";
        }

        roomList += "----------------------------\n";

        return roomList;
    }
};  
