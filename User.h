#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <atomic>
#include <mutex>

class User {
private:
    std::string username;
    std::string password;
    std::string info;
    int wins;
    int losses;
    float rating;
    bool isQuiet;
    std::unordered_set<std::string> blockedUsers;
    std::mutex userMutex;
    int clientSocket;
    bool isGuest;
    bool isPlaying;
    bool isObserving;
    int gameId;

public:
    User(const std::string& username, const std::string& password, int socket)
        : username(username), password(password), info(""), wins(0), losses(0), rating(1500.0f),
          isQuiet(false), clientSocket(socket), isGuest(username == "guest"),
          isPlaying(false), isObserving(false), gameId(-1) {}

    // Getters and setters
    std::string getUsername() const { return username; }
    bool checkPassword(const std::string& pwd) const { return password == pwd; }
    void setPassword(const std::string& pwd) { password = pwd; }
    void setInfo(const std::string& newInfo) { info = newInfo; }
    std::string getInfo() const { return info; }
    int getWins() const { return wins; }
    int getLosses() const { return losses; }
    float getRating() const { return rating; }
    bool isInQuietMode() const { return isQuiet; }
    void setQuietMode(bool quiet) { isQuiet = quiet; }
    int getSocket() const { return clientSocket; }
    void setSocket(int socket) { clientSocket = socket; }
    bool isUserGuest() const { return isGuest; }
    bool isInGame() const { return isPlaying; }
    void setPlaying(bool playing) { isPlaying = playing; }
    bool isUserObserving() const { return isObserving; }
    void setObserving(bool observing) { isObserving = observing; }
    int getGameId() const { return gameId; }
    void setGameId(int id) { gameId = id; }

    // Game statistics methods
    void addWin() { wins++; updateRating(true); }
    void addLoss() { losses++; updateRating(false); }

    // User blocking methods
    void blockUser(const std::string& user) {
        std::lock_guard<std::mutex> lock(userMutex);
        blockedUsers.insert(user);
    }

    void unblockUser(const std::string& user) {
        std::lock_guard<std::mutex> lock(userMutex);
        blockedUsers.erase(user);
    }

    bool isBlocked(const std::string& user) const {
        return blockedUsers.find(user) != blockedUsers.end();
    }

    std::vector<std::string> getBlockedUsers() const {
        std::vector<std::string> result;
        for (const auto& user : blockedUsers) {
            result.push_back(user);
        }
        return result;
    }

private:
    void updateRating(bool won) {
        // Basic ELO-like rating system
        if (won) {
            rating += 15.0f;
        } else {
            rating = std::max(1000.0f, rating - 15.0f);
        }
    }
};

// UserManager singleton to manage all users
class UserManager {
private:
    std::unordered_map<std::string, std::shared_ptr<User>> users;
    std::unordered_map<int, std::string> socketToUser;
    std::mutex usersMutex;

    // Private constructor for singleton
    UserManager() {
        // Create default guest account
        users["guest"] = std::make_shared<User>("guest", "", -1);
    }

public:
    // Get the singleton instance
    static UserManager& getInstance() {
        static UserManager instance;
        return instance;
    }

    // User registration and login
    bool registerUser(const std::string& username, const std::string& password, int socket) {
        std::lock_guard<std::mutex> lock(usersMutex);

        // Check if username already exists
        if (users.find(username) != users.end()) {
            return false;
        }

        // Create new user
        users[username] = std::make_shared<User>(username, password, socket);

        // Save user data to disk
        saveUsers();

        return true;
    }

    bool loginUser(const std::string& username, const std::string& password, int socket) {
        std::lock_guard<std::mutex> lock(usersMutex);

        auto it = users.find(username);
        if (it == users.end() || !it->second->checkPassword(password)) {
            return false;
        }

        // Update socket and track connection
        it->second->setSocket(socket);
        socketToUser[socket] = username;

        return true;
    }

    bool loginGuest(int socket) {
        std::lock_guard<std::mutex> lock(usersMutex);
        socketToUser[socket] = "guest";
        return true;
    }

    void logoutUser(int socket) {
        std::lock_guard<std::mutex> lock(usersMutex);

        auto it = socketToUser.find(socket);
        if (it != socketToUser.end()) {
            std::string username = it->second;
            if (username != "guest" && users.find(username) != users.end()) {
                users[username]->setSocket(-1); // Mark as disconnected
            }
            socketToUser.erase(it);
        }
    }

    std::string getUsernameBySocket(int socket) {
        std::lock_guard<std::mutex> lock(usersMutex);

        auto it = socketToUser.find(socket);
        if (it != socketToUser.end()) {
            return it->second;
        }
        return "";
    }

    std::shared_ptr<User> getUserByUsername(const std::string& username) {
        std::lock_guard<std::mutex> lock(usersMutex);

        auto it = users.find(username);
        if (it != users.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<User> getUserBySocket(int socket) {
        std::string username = getUsernameBySocket(socket);
        if (!username.empty()) {
            return getUserByUsername(username);
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<User>> getOnlineUsers() {
        std::lock_guard<std::mutex> lock(usersMutex);

        std::vector<std::shared_ptr<User>> result;
        for (const auto& pair : socketToUser) {
            std::string username = pair.second;
            if (username != "guest" && users.find(username) != users.end()) {
                result.push_back(users[username]);
            }
        }
        return result;
    }

    void saveUsers() {
        // TODO: Implement user data persistence
        // Save user data to disk in a format that can be loaded when server restarts
    }

    void loadUsers() {
        // TODO: Implement user data loading
        // Load user data from disk when server starts
    }
};

#endif // USER_H