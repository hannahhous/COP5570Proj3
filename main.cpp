#include <iostream>
#include <signal.h>

#include "TelnetServer.h"

#include <csignal>
#include <chrono>
#include <thread>

#include "TelnetServer.h"

volatile sig_atomic_t shouldExit = 0;
TelnetServer* globalServer = nullptr; // Global pointer to access server from signal handler

void signalHandler(int signal) {
    shouldExit = 1;

    // If we have a global server pointer, trigger shutdown directly
    if (globalServer) {
        globalServer->running = false; // Directly set the running flag to false
    }
}

int main() {
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    int port = 8023;

    TelnetServer server;
    globalServer = &server; // Set the global pointer

    if (!server.start(port)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "Server running. Press Ctrl+C to stop." << std::endl;

    // Main loop
    while (!shouldExit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Check more frequently
    }

    std::cout << "Shutting down..." << std::endl;
    server.stop();
    std::cout << "Server stopped." << std::endl;

    globalServer = nullptr; // Clear the pointer before exiting
    return 0;
}
/*

volatile sig_atomic_t shouldExit = 0;

void signalHandler(int signal) {
    shouldExit = 1;
}


int main()
{
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    int port = 8023;

    TelnetServer server;
    if (!server.start(port)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "Server running. Press Ctrl+C to stop." << std::endl;

    // Main loop
    while (!shouldExit) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Shutting down..." << std::endl;
    server.stop();

    return 0;
}

*/