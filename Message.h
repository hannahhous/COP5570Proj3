#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <ctime>

enum class MessageStatus { READ, UNREAD };

class Message {
private:
    int id;
    std::string sender;
    std::string recipient;
    std::string title;
    std::string content;
    time_t timestamp;
    MessageStatus status;

public:
    Message(int id, const std::string& sender, const std::string& recipient,
            const std::string& title, const std::string& content)
        : id(id), sender(sender), recipient(recipient), title(title), content(content),
          status(MessageStatus::UNREAD) {
        timestamp = std::time(nullptr);
    }

    int getId() const { return id; }
    std::string getSender() const { return sender; }
    std::string getRecipient() const { return recipient; }
    std::string getTitle() const { return title; }
    std::string getContent() const { return content; }
    time_t getTimestamp() const { return timestamp; }
    MessageStatus getStatus() const { return status; }

    void markAsRead() { status = MessageStatus::READ; }

    std::string getFormattedTimestamp() const {
        char buffer[80];
        struct tm* timeinfo = localtime(&timestamp);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        return std::string(buffer);
    }

    std::string getHeader() const {
        return std::to_string(id) + ". " +
               (status == MessageStatus::UNREAD ? "[NEW] " : "") +
               "From: " + sender + ", Title: " + title + ", Date: " + getFormattedTimestamp();
    }
};

// MessageManager singleton to manage all messages
class MessageManager {
private:
    std::unordered_map<std::string, std::vector<std::shared_ptr<Message>>> userMessages;
    int nextMessageId;
    std::mutex messagesMutex;

    // Private constructor for singleton
    MessageManager() : nextMessageId(1) {}

public:
    // Get the singleton instance
    static MessageManager& getInstance() {
        static MessageManager instance;
        return instance;
    }

    // Send a message
    void sendMessage(const std::string& sender, const std::string& recipient,
                    const std::string& title, const std::string& content) {
        std::lock_guard<std::mutex> lock(messagesMutex);

        auto message = std::make_shared<Message>(nextMessageId++, sender, recipient, title, content);
        userMessages[recipient].push_back(message);

        // TODO: Save messages to disk periodically
    }

    // Get all messages for a user
    std::vector<std::shared_ptr<Message>> getUserMessages(const std::string& username) {
        std::lock_guard<std::mutex> lock(messagesMutex);

        if (userMessages.find(username) != userMessages.end()) {
            return userMessages[username];
        }
        return std::vector<std::shared_ptr<Message>>();
    }

    // Get a specific message by ID for a user
    std::shared_ptr<Message> getUserMessage(const std::string& username, int messageId) {
        std::lock_guard<std::mutex> lock(messagesMutex);

        if (userMessages.find(username) != userMessages.end()) {
            for (auto& message : userMessages[username]) {
                if (message->getId() == messageId) {
                    return message;
                }
            }
        }
        return nullptr;
    }

    // Delete a message
    bool deleteMessage(const std::string& username, int messageId) {
        std::lock_guard<std::mutex> lock(messagesMutex);

        if (userMessages.find(username) != userMessages.end()) {
            auto& messages = userMessages[username];
            for (auto it = messages.begin(); it != messages.end(); ++it) {
                if ((*it)->getId() == messageId) {
                    messages.erase(it);
                    return true;
                }
            }
        }
        return false;
    }

    // Count unread messages for a user
    int countUnreadMessages(const std::string& username) {
        std::lock_guard<std::mutex> lock(messagesMutex);

        int count = 0;
        if (userMessages.find(username) != userMessages.end()) {
            for (auto& message : userMessages[username]) {
                if (message->getStatus() == MessageStatus::UNREAD) {
                    count++;
                }
            }
        }
        return count;
    }

    // Load messages from disk
    void loadMessages() {
        // TODO: Implement message loading from disk
    }

    // Save messages to disk
    void saveMessages() {
        // TODO: Implement message saving to disk
    }
};

#endif // MESSAGE_H