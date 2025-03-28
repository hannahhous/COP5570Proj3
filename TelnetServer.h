#ifndef TELNETSERVER_H
#define TELNETSERVER_H

#include <atomic>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <sys/socket.h>
#include <thread>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "SocketUtils.h"
#include "TelnetClientHandler.h"

class TelnetServer
{
public:
    TelnetServer() : serverSocket(-1), running(false)
    {
    }

    bool start(int port)
    {
        //
        // Create a socket.
        // Arguments:
        //     domain: AF_INET = IPv4
        //     type: SOCK_STREAM = TCP
        //     protocol: 0 = let system choose
        // Returns:
        //     file descriptor, used for subsequent socket operations
        //
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0)
        {
            perror("socket");
            return false;
        }

        //
        // Set socket options
        // Arguments:
        //     file descriptor returned by socket()
        //     protocol level: SOL_SOCKET = ???
        //     optname: SO_REUSEADDR = allows this socket to be used with multiple client connections
        //     optval: pointer to the value of the option (1 == true?)
        //     size of option value variable
        //
        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            perror("setsockopt");
            close(serverSocket);
            return false;
        }

        // Socket binding address info
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;  // IPv4
        serverAddr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available ports
        serverAddr.sin_port = htons(port);  // Convert from host byte order to network byte order

        //
        // Bind the socket to a local address and port number
        // Arguments:
        //   file descriptor
        //   pointer to variable containing address info
        //   size of address info struct
        //
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
        {
            // If failed, close socket and return error
            perror("bind");
            close(serverSocket);
            return false;
        }

        if (listen(serverSocket, 10) < 0) {
            perror("listen");
            close(serverSocket);
            return false;
        }

        // Set the server to non-blocking mode
        // This means the low-level recv() calls (see SocketUtils::receiveData ) will not block if no data is available
        if (!SocketUtils::setNonBlocking(serverSocket))
        {
            close(serverSocket);
            return false;
        }

        running = true;

        // Start the thread to accept new connections
        acceptThread = std::thread(&TelnetServer::acceptConnections, this);

        std::cout << "Gomoku server started on port " << port << std::endl;
        return true;
    }

    void stop()
    {
        running = false;

        if (acceptThread.joinable())
        {
            acceptThread.join();
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            clients.clear();
        }

        if (serverSocket >= 0)
        {
            close(serverSocket);
            serverSocket = -1;
        }

        std::cout << "Server stopped" << std::endl;
    }

    void broadcastMessage(std::string msg)
    {
        for (auto client: clients)
        {
            client->sendMessage(msg);
        }
    }


private:
    void acceptConnections()
    {
        while (running)
        {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            // Non-blocking accept
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

            if (clientSocket < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // No pending connections, sleep a bit
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
                else
                {
                    perror("accept");
                    continue;
                }
            }

            // Set client socket to non-blocking mode
            if (!SocketUtils::setNonBlocking(clientSocket))
            {
                close(clientSocket);
                continue;
            }

            // Create a client handler for this connection
            std::lock_guard<std::mutex> lock(mutex);
            clients.push_back(std::make_shared<TelnetClientHandler>(clientSocket));

            // Send a test message
            clients.back()->sendMessage("Welcome to TelnetServer.");
            broadcastMessage("New client joined.");

            // Log connection
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            std::cout << "New connection from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
        }
    }

private:
    int serverSocket;
    std::atomic<bool> running;
    std::thread acceptThread;
    std::vector<std::shared_ptr<TelnetClientHandler>> clients;
    std::mutex mutex;

};

#endif //TELNETSERVER_H
