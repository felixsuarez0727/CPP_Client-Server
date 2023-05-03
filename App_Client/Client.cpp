#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctime>
#include <cstdlib>

const char* SERVER_IP = "127.0.0.1";
const int INTERVAL = 30;
const int BUFFER = 1024;
const int PORT = 41123;

const int ERROR_CODE = -1;
const int OK_CODE = 0;

int main() {
    // Create client socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        std::cerr << "Error: could not create client socket" << std::endl;
        return ERROR_CODE;
    }

    // Connect to server
    sockaddr_in serverAddress = {AF_INET, htons(PORT), inet_addr(SERVER_IP)};

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Error: could not connect to server" << std::endl;
        return ERROR_CODE;
    }

    
    while (true) {
        std::cout << "Connected successfully\n";

        char requestBuffer[BUFFER];
        int bytesReceived = recv(clientSocket, requestBuffer, BUFFER, 0);

        if (bytesReceived == -1) {
            std::cerr << "Error: recv() failed" << std::endl;
            break;
        }
        
        requestBuffer[bytesReceived] = '\0';

        // Receive GET_TIMESTAMP 
        if (strcmp(requestBuffer, "GET_TIMESTAMP") == 0) {
            std::string currentTimeString = std::to_string(time(nullptr));

            // Respond with current time
            send(clientSocket, currentTimeString.c_str(), currentTimeString.size(), 0);
            std::cout << "Sending current time to server: " << currentTimeString << std::endl;
        }

        // Wait before checking for more requests
        sleep(INTERVAL);
    }

    // Close client socket
    close(clientSocket);
    return OK_CODE;
}