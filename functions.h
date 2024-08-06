#include<bits/stdc++.h>
using namespace std;

// for debuging
void print_resp(string s){
        for(char &c:s){
            if(c=='\n') cout<<"\\n";
            else if(c=='\r') cout<<"\\r";
            else cout<<c;
        }
        cout<<endl;
};

bool isInt(string &s){
    if(s.size()&&s[0]=='-'){
        for(int i=1;i<s.size();i++)
        if(!isdigit(s[i])) return false;
    }
    else{
        for(auto &i:s){
            if(!isdigit(i)) return false;
        }
    }
    return true;
}

// parsing the input from user into array of strings
void parse(string &s, vector<string>&res){
    string token="";
    bool quotes=false;
    for(char &c:s){
        if(c==' '){
            if(quotes){
                token+=c;
            }
            else if(token.size()){
                res.push_back(token);
                token.clear();
            }
        }
        else if(c=='\"'){
            if(quotes){
                if(token.size()){
                    res.push_back(token);
                    token.clear();
                }
                quotes=false;
            }
            else quotes=true;
        }
        else 
        token+=c;
    }
    if(token.size()) res.push_back(token);
}


// deserialize block string , return empty string in case of syntax error
string deserialize(string data)
{
    char first = data.front();
    string result="";
    size_t p = 0;
    switch (first)
    {
    case '+':
        p = data.find("\r\n", 1);
        result = data.substr(1, p - 1);
        break;
    
    case '-':
        p = data.find("\r\n", 1);
        result = "(error) ";
        result += data.substr(1, p - 1);
        break;
    
    case ':':
        p = data.find("\r\n", 1);
        result = "(integer) ";
        result += data.substr(1, p - 1);
        break;

    case '$':
        if(data=="$-1\r\n") return "(nil)";
        p = data.find("\r\n", 1);
        int size = stoi(data.substr(1, p - 1));
        p += 2;
        result = data.substr(p,size);
        p+=size;
        if (p!=data.size()-2||data[p]!='\r'||data[p+1]!='\n'){
            result = "";
        }
        break;
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

// type  0=simple, 1=block, -1=error , -2=empty
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

    case -2:
        result = "$-1\r\n";
        break;
    default:
        break;
    }
    return result;
}

string serialize(int data)
{
    string result = ":" + to_string(data) + "\r\n";
    return result;
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