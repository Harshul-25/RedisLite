#include "functions.h"
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <iostream>     // For cout
#include <unistd.h>     // For read
#include <arpa/inet.h>

const int PORT=6379;

// //The way RESP is used in Redis as a request-response protocol is the following:
// Clients send commands to a Redis server as a RESP Array of Bulk Strings.
// The server replies with one of the RESP types according to the command implementation.

// helper function to send request to server
void sendReq(int &client, string &req)
{
    const char *data_ptr = req.data();
    size_t data_size = req.size();

    int bytes_sent = 0;

    while (data_size > 0)
    {
        bytes_sent = send(client, data_ptr, data_size, 0);
        if (bytes_sent == -1)
        {
            cout << "Unable to send req with errno: " << errno << endl;
            return;
        }

        data_ptr += bytes_sent;
        data_size -= bytes_sent;
    }
}

string getMsg(int &client)
{

    const int BUF_LENGTH = 512;
    vector<char> buffer(BUF_LENGTH);
    string response = "";
    int bytesReceived = 0;
    do
    {
        bytesReceived = read(client, &buffer[0], buffer.size());
        // append string from buffer.
        if (bytesReceived == -1)
        {
            cout << "Unable to recieve request with errno: " << errno << endl;
        }
        else
        {
            response.append(buffer.cbegin(), buffer.cbegin()+bytesReceived);
        }
    } while (bytesReceived == BUF_LENGTH);
    string Msg = deserialize(response);
    return Msg;
}

void process(string &line, int &client)
{
    vector<string> data;
    parse(line,data);
    string req = serialize(data);
    sendReq(client,req);
    string response = getMsg(client);
    cout<<response<<endl;
}

int main()
{

    int client = socket(AF_INET, SOCK_STREAM, 0);
    if (client == -1)
    {
        std::cout << "Failed to create socket. errno: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sockaddr.sin_port = htons(PORT);

    // Connect with the server
    int status = connect(client, (struct sockaddr *)&sockaddr, sizeof(sockaddr));

    if(status<0){
        cout<<"Error connecting with the server! errno: "<<errno<<endl;
        exit(EXIT_FAILURE);
    }
    cout << "Connected to the server!" << endl;
    cout << "Welcome to RedisLite!!" << endl;
    string line;
    for (; cout << "RedisLite > " && getline(cin, line);)
    {
        if (line == "exit")
            break;
        if (!line.empty())
        {
            process(line,client);
        }
    }
    close(client);
    std::cout << "Goodbye.\n";
}
