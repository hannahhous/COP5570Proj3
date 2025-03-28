#ifndef TELNETCLIENTHANDLER_H
#define TELNETCLIENTHANDLER_H
#include <thread>

class TelnetClientHandler
{
private:
    int clientSocket;
    std::atomic<bool> running;
    std::thread handlerThread;

public:
    TelnetClientHandler(int socket)
        : clientSocket(socket), running(true)
    {
        // Start the handler thread
        handlerThread = std::thread(&TelnetClientHandler::handleClient, this);
        handlerThread.detach();
    }

    ~TelnetClientHandler()
    {
        running = false;
    }

    bool sendMessage(const std::string& message) const
    {
        return SocketUtils::sendData(clientSocket, message + "\r\n");
    }

private:
    void handleClient()
    {
        int timeout_ms = 10000;

        // Main command loop
        while (running)
        {
            std::string rawData = SocketUtils::receiveData(clientSocket, timeout_ms);

            // Strip telnet control sequences and control characters
            std::string result;
            for (char c : rawData)
            {
                if (c >= 32 && c < 127)
                { // Printable ASCII
                    result += c;
                }
                else if (c == '\r' || c == '\n')
                {
                    result += c;
                }
            }

            // Extract the first line
            size_t pos = result.find("\r\n");
            if (pos != std::string::npos)
            {
                result = result.substr(0, pos);
            }

            if (result.empty())
            {
                // Socket might be closed or timed out
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            //********************************************************
            // Hook up logic for processing commands from clients here
            //********************************************************

            //!!! TODO: Process the command
            printf("Raw command [%d]: %s\n", clientSocket, result.c_str());

        }
    }
};

#endif //TELNETCLIENTHANDLER_H
