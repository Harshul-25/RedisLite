#include "functions.h"
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <iostream>     // For cout
#include <unistd.h>     // For read
#include<chrono>        // For time 
#include <arpa/inet.h>
using namespace std;

#define MAXCLIENTS 20
const int PORT=6379;


struct Entry
{
    string data;
    int flag = 0;
    chrono::system_clock::time_point exptime;
    Entry(){}
    Entry(string data_){
        data = data_;
    }
    Entry(string data_,int seconds){
        flag=1;
        data=data_;
        exptime = chrono::system_clock::now() + chrono::seconds(seconds);
    }
    void set(Entry &a){
        data=a.data;
        flag=a.flag;
        exptime=a.exptime;
    }
};

unordered_map<string, Entry> cache; // key-value cache
std::mutex myMutex;                 // mutex to synchronize access to cache

bool isExpired(Entry &e){
    return chrono::system_clock::now() > e.exptime;
}


// helper function to send response to client
// type  0=simple, 1=block, -1=error , -2=empty , 2=Int
void sendResponse(int &connection, string data, int type=1)
{
    string response = "";
    if(type==2 && isInt(data))
        response = serialize(stoi(data));
    else
        response = serialize(data,type);
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

    // cout << "Response sent. Total bytes: " << bytes_sent << endl;
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

// function for handling SET
void handleSet(int &connection, vector<string>&command){
    string operation = command.front();
    if(command.size()!=5 && command.size()!=3){
        sendResponse(connection,"ERR wrong number of arguments for "+operation+" command",-1);
    }
    else if(command.size()>3 && (command[3]!="EX" && command[3]!="ex")){
        sendResponse(connection,"ERR invalid syntax",-1);
    }
    else{
        string key = command[1];
        string value = command[2];
        int seconds = -1;
        if(command.size()>3){
            if(!isInt(command[4])){
                sendResponse(connection,"ERR value is not an integer",-1);
                return;
            }
            else
            seconds = stoi(command[4]);
        }
        std::lock_guard<std::mutex> guard(myMutex);
        if(seconds!=-1){
            Entry e(value,seconds);
            cache[key].set(e);
        }
        else{
        Entry e(value);
        cache[key].set(e);
        }
        sendResponse(connection,"OK",0);
    }
}

// function for handling GET
void handleGet(int &connection, vector<string>&command){
    string operation = command.front();
    if(command.size()!=2){
        sendResponse(connection,"ERR wrong number of arguments for "+operation+" command",-1);
    }
    else{
        std::lock_guard<std::mutex> guard(myMutex);
        string key = command[1];
        if(cache.count(key)==0){                //check if the key exists
            sendResponse(connection,"",-2);
        }
        else {
            Entry t(cache[key]);
            if(t.flag&&isExpired(t)){          //checking for expiry
                cache.erase(key);
                sendResponse(connection,"",-2);
            }
            else{
                sendResponse(connection,t.data);
            }
        }
    }
}

void handleExist(int &connection,vector<string>&command){
    if(command.size()!=2){
        sendResponse(connection,"ERR wrong number of arguments for "+command[0]+" command",-1);
    }
    std::lock_guard<std::mutex> guard(myMutex);
    string key = command[1];
    if(cache.count(key))                //check if the key exists
        sendResponse(connection,"1",2);
    else
        sendResponse(connection,"0",2);
}

void handleDel(int &connection,vector<string>&command){
    if(command.size()!=2){
        sendResponse(connection,"ERR wrong number of arguments for "+command[0]+" command",-1);
    }
    std::lock_guard<std::mutex> guard(myMutex);
    string key = command[1];
    if(cache.count(key)){               //check if the key exists
        cache.erase(key);
        sendResponse(connection,"1",2);
    }                
    else
        sendResponse(connection,"0",2);
}

void handleInDec(int &connection,vector<string>&command){
    if(command.size()!=2){
        sendResponse(connection,"ERR wrong number of arguments for "+command[0]+" command",-1);
    }
    string op = command.front();
    string key = command[1];
    std::lock_guard<std::mutex> guard(myMutex);
    if(cache.count(key)==0){
        if(op=="INCR"){
            cache[key].data="1";
            sendResponse(connection,"1",2);
        }
        else{
            cache[key].data="-1";
            sendResponse(connection,"-1",2);
        }
        return;
    }
    Entry t(cache[key]);
    if(!isInt(t.data)){
        sendResponse(connection,"ERR value is not an integer",-1);
         return;
    }
    int no = stoi(t.data);
    if(op=="INCR") no++;
    else no--;
    t.data = to_string(no);
    cache[key].set(t);
    sendResponse(connection,t.data,2);
}

void handleConnection(int connection)
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
        for (auto& x : operation) {
            x = toupper(x);
        }
        command[0] = operation;
        if (operation == "PING")
        {
            string response = "PONG";
            sendResponse(connection, response);
        }
        else if(operation=="ECHO"){
            if(command.size()>2){
                sendResponse(connection,"ERR wrong number of arguments for "+operation+" command",-1);
            }
            else{
                string response = command[1];
                sendResponse(connection,response);
            }
        }
        else if(operation=="SET"){
            handleSet(connection,command);
        }
        else if(operation=="GET"){
            handleGet(connection,command);
        }
        else if(operation=="EXISTS"){
            handleExist(connection,command);
        }
        else if(operation=="DEL"){
            handleDel(connection,command);
        }
        else if(operation=="INCR"||operation=="DECR"){
            handleInDec(connection,command);
        }
        else{
            string response = "Unknown command "+command.front();
            sendResponse(connection,response,-1);
        }
    }
}

int main(int argc, char const *argv[])
{
    
    // Create a socket (IPv4, TCP)
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        cout << "Failed to create socket. errno: " << errno << endl;
        exit(EXIT_FAILURE);
    }

    // Listen to port 6379 on local host
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;  //IPv4 
    sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sockaddr.sin_port = htons(PORT);

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

    cout << "Server created successfully. Listening on PORT: "<<PORT<< endl;

    vector<thread *> threads;

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

        // Handle each connection in a separate thread
        threads.push_back(new thread(handleConnection, connection));
    }

    // joining all threads to main thread
    for (auto t : threads)
        t->join();
    
    close(sockfd);
     return 0;
}
