#include<bits/stdc++.h>
using namespace std;

void print_resp(string s){
        for(char &c:s){
            if(c=='\n') cout<<"\\n";
            else if(c=='\r') cout<<"\\r";
            else cout<<c;
        }
        cout<<endl;
};

// deserialize block string
string deserialize(string data)
{
    if (data.front() != '$')
    {
        return "";
    }
    size_t p = 0;
    p = data.find("\r\n", 1);
    int size = stoi(data.substr(1, p - 1));
    p += 2;
    string result = data.substr(p,size);
    p+=size;
    if (p!=data.size()-2||data[p]!='\r'||data[p+1]!='\n'){
        return "";
    }
    return result;
}

// return -1 if error
int deserialize_arr(string &data, vector<string> &commands)
{
    char type_indicator = data[0];
    if (type_indicator != '*')
    return -1;

    size_t pos = 0;
    pos = data.find("\r\n");
    if (pos == string::npos)
        return -1;
    int n = stoi(data.substr(1, pos - 1));
    commands.resize(n);
    pos+=2;
    for (int i = 0; i < n; i++)
    {
        size_t pos_end = data.find("\r\n", pos);
        if (pos_end == string::npos)
            return -1;
        
        int size = stoi(data.substr(pos+1,pos_end-pos-1));
        pos_end += size+3;
        string temp = deserialize(data.substr(pos, pos_end - pos+1));
        if (temp == "")
            return -1;
        else
            commands[i] = temp;
        pos = pos_end+1;
    }

    if (pos != data.size())
        return -1;
    return 0;
}

// type  0=simple, 1=block, -1=error
string serialize(string &data, int type = 1)
{
    string result = "";
    switch (type)
    {
    case 1:
        result = "$" + to_string(data.size()) + "\r\n" + data + "\r\n";
        break;

    case 0:
        result = "+" + data + "\r\n";
        break;

    case -1:
        result = "-" + data + "\r\n";
        break;

    default:
        break;
    }
    return result;
}

string serialize(int data)
{
    return ":" + to_string(data) + "\r\n";
}

string serialize(vector<string> &data)
{
    string result = "*" + to_string(data.size()) + "\r\n";
    for (auto &item : data)
    {
        result += serialize(item);
    }
    return result;
}