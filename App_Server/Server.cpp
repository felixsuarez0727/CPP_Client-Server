#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

const string MESSAGE = "GET_TIMESTAMP";
const int MAX_CONN = 100;
const int BUFFER = 1024;
const int INTERVAL = 30;
const int PORT = 41123;

vector<int> clients;

/*
 * This funcion is used to format the timestamp to string.
 */
std::string formatTimestamp(const std::string &timestampStr)
{
    // Convert timestamp string to time_t
    time_t timestamp = std::stoi(timestampStr);

    // Convert time_t to struct tm in local time zone
    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);

    // Format time as string in desired format
    char buffer[40];
    std::strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S]", &timeinfo);
    return std::string(buffer);
}

/*
 * This funcion is used to print in the log the client time and IP.
 */
void log_timestamp(char *buffer, int clientSocket)
{
    time_t now = time(0);
    struct tm *local_time = localtime(&now);

    // Obtener la dirección IP del cliente
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getpeername(clientSocket, (struct sockaddr *)&addr, &len);
    char *ip = inet_ntoa(addr.sin_addr);

    char *clientDate = inet_ntoa(addr.sin_addr);

    // Crear el mensaje de log
    char log_message[256];
    sprintf(log_message, "[%04d-%02d-%02d %02d:%02d:%02d] Client %s - Client time: %s\n",
            local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday,
            local_time->tm_hour, local_time->tm_min, local_time->tm_sec, ip, formatTimestamp(buffer).c_str());

    // Escribir el mensaje en el archivo de log
    FILE *log_file = fopen("app.log", "a");
    if (log_file)
    {
        fputs(log_message, log_file);
        fclose(log_file);
    }
}

/*
 *  This funcion is used to send a timestamp to a random client every 30 seconds.
 */
void send_timestamp()
{
    while (true)
    {
        if (!clients.empty())
        {
            std::random_device rd;
            std::uniform_int_distribution<> dis(0, clients.size() - 1);
            int client_socket = clients[dis(rd)];

            // Send message to client
            cout << "Sending message to client: " << client_socket << endl;
            send(client_socket, MESSAGE.c_str(), MESSAGE.length(), 0);

            // Receive response from client
            char buffer[BUFFER];
            int bytes_received = recv(client_socket, buffer, BUFFER, 0);
            buffer[bytes_received] = '\0';
            cout << "Received response from client: " << buffer << endl;

            // Write response in logs.
            log_timestamp(buffer, client_socket);
        }

        this_thread::sleep_for(chrono::seconds(INTERVAL));
    }
}

int main()
{
    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        cerr << "Error: could not create server socket" << endl;
        return -1;
    }

    // Bind server socket to port
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        cerr << "Error: could not bind server socket to port " << PORT << endl;
        return -1;
    }

    // Listen for client connections
    if (listen(server_socket, MAX_CONN) == -1)
    {
        cerr << "Error: could not listen for client connections" << endl;
        return -1;
    }

    thread send_thread(send_timestamp);
    
    // Accept client connections
    cout << "Server awaiting new connections" << endl;
    while (true)
    {
        // If reached max connections allowed
        if (clients.size() >= MAX_CONN)
        {
            // Sleep and skip this connection attempt.
            this_thread::sleep_for(1s);
            cout << "Max number of connections reached" << endl;
            continue;
        }

        sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);
        int client_socket = accept(server_socket, (sockaddr *)&client_address, &client_address_size);
        if (client_socket == -1)
        {
            cerr << "Error: could not accept client connection" << endl;
            continue;
        }
        clients.push_back(client_socket);
        cout << "Accepted new client connection: " << client_socket << endl;
    }

    // Join thread for sending timestamps
    send_thread.join();

    return 0;
}