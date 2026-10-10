#pragma once
#include <string>
#include <utility>

class Venue
{
private:
    int id{0};
    std::string name;
    std::string address;
    int capacity{0};

public:
    Venue() = default;
    Venue(int id, std::string name, std::string address, int capacity)
        : id(id), name(std::move(name)), address(std::move(address)), capacity(capacity) {}

    // Getters
    int getId() const { return id; }
    const std::string &getName() const { return name; }
    const std::string &getAddress() const { return address; }
    int getCapacity() const { return capacity; }

    // Setters
    void setId(int newId) { id = newId; }
    void setName(const std::string &newName) { name = newName; }
    void setAddress(const std::string &newAddress) { address = newAddress; }
    void setCapacity(int newCapacity) { capacity = newCapacity; }
};