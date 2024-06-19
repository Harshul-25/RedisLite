#include "functions.h"
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <iostream>     // For cout
#include <unistd.h>     // For read
#include <arpa/inet.h>
using namespace std;

#define MAXCLIENTS 20

// helper function to send response to client
void sendResponse(int &connection, string data)
{
    string response = serialize(data);
    const char *data_ptr = response.data();
    size_t data_size = response.size();

    int bytes_sent = 0;

    while (data_size > 0)
    {
        bytes_sent = send(connection, data_ptr, data_size, 0);
        if (bytes_sent == -1)
        {
            cout << "Unable to send response with errno: " << errno << endl;
            return;
        }

        data_ptr += bytes_sent;
        data_size -= bytes_sent;
    }

    cout << "Response sent. Total bytes: " << bytes_sent << endl;
}

// helper function to read command from client
int getCommand(int &connection, vector<string> &command)
{

    const int BUF_LENGTH = 512;
    vector<char> buffer(BUF_LENGTH);
    string req = "";
    int bytesReceived = 0;
    do
    {
        bytesReceived = read(connection, &buffer[0], buffer.size());
        // append string from buffer.
        if (bytesReceived == -1)
        {
            cout << "Unable to recieve request with errno: " << errno << endl;
        }
        else
        {
            req.append(buffer.cbegin(), buffer.cbegin()+bytesReceived);
        }
    } while (bytesReceived == BUF_LENGTH);
    if(req.empty()){
        return -1;
    }

    if (deserialize_arr(req, command) == -1)
    {
        command.clear();
        cout << "Invalid RESP format"<< endl;
    }
    return 0;
}


void handleConnection(int &connection)
{
    while (true)
    {
        vector<string> command;
        int status = getCommand(connection, command);
        if(status==-1){
            cout<<"Client disconnected"<<endl;
            close(connection);
            break;
        }
        if(command.empty()){
            sendResponse(connection,"Invalid RESP format");
        }
        string operation = command.front();
        if (operation == "PING")
        {
            string response = "PONG";
            sendResponse(connection, response);
        }
        else if(operation=="ECHO"){
            string response = "";
            for(int i=1;i<command.size();i++) response=response+command[i]+" ";
            response.pop_back();
            sendResponse(connection,response);
        }
        else{
            string response = "Unknown command "+command.front();
            sendResponse(connection,response);
        }
    }
}

int main(int argc, char const *argv[])
{
    // string req= "*2\r\n$4\r\necho\r\n$11\r\nhello world\r\n";
    // handleConnection(req);

    // Create a socket (IPv4, TCP)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        cout << "Failed to create socket. errno: " << errno << endl;
        exit(EXIT_FAILURE);
    }

    // Listen to port 6379 on local host
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sockaddr.sin_port = htons(6379);

    // Bind socket to an IP address and port
    if (bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
    {
        cout << "Failed to bind to port. errno: " << errno << endl;
        exit(EXIT_FAILURE);
    }

    // Start listening. Hold at most MaxClients (defined at top) connections in the queue
    if (listen(sockfd, MAXCLIENTS) < 0)
    {
        cout << "Failed to listen on socket. errno: " << errno << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Server created successfully." << endl;

    int addrlen = sizeof(sockaddr);

    while (true)
    {
        // Grab a connection from the queue
        int connection = accept(sockfd, (struct sockaddr *)&sockaddr, (socklen_t *)&addrlen);
        if (connection < 0)
        {
            cout << "Failed to grab connection. errno: " << errno << endl;
            exit(EXIT_FAILURE);
        }
        handleConnection(connection);
    }
    close(sockfd);
     return 0;
}
