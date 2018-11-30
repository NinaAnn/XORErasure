#include <iostream>
#include <string>
#include <fstream> 
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <memory.h>
#include <thread>

#define maxbuf 128

using namespace std;

class Client{

	public:
		int sock;
		struct sockaddr_in serverAdd;
		Client(string serverIP, int serverPort);
		string upload_file(const string filePath);
		void send_request(string instruction, string data);
		void read_response();
		void do_request();
};
Client::Client(string serverIP, int port){
	this->sock = socket(AF_INET, SOCK_STREAM, 0);
	memset(&(this->serverAdd), 0, sizeof(this->serverAdd));
	this->serverAdd.sin_family = AF_INET;
	char* IP = new char;
	strcpy(IP, serverIP.c_str());
    this->serverAdd.sin_addr.s_addr = inet_addr(IP);
    this->serverAdd.sin_port = htons(port);
	delete IP;

}
string Client::upload_file(const string filePath){
	std::cout<<"enter";
	string s;
	ifstream in(filePath);
	if (! in.is_open())
        { std::cout << "Error opening file"; exit (1); }
    ostringstream buf;
	char ch;
	while(buf&&in.get(ch))
		buf.put(ch);
	return buf.str();
}

void Client::send_request(string instruction, string data){
	//std::cout<<"instruction is";
	string message = "";
	message = instruction + data;
	char buf[maxbuf];
	memset(buf, 0, sizeof(buf));
	strcpy(buf, message.c_str());
    write(this->sock, buf, sizeof(buf));

	memset(buf, 0, sizeof(char));
    read(this->sock, buf, sizeof(buf) - 1);
	string res(buf);
	if(instruction=="r"){
		cout<<"The file "<<data<<" is: "<<res<<endl;
	}
	else{
		cout<<data<<":"<<res<<endl;
	}
	

}

void Client::do_request(){
	
	string r = "r";
	string w = "w";
	string instruction;
	string filePath;
	std::cout<<"select an operation from: read, write, exit"<<endl<<">>";
	cin>>instruction;
	while(connect(this->sock, (struct sockaddr*)&(this->serverAdd), sizeof(this->serverAdd))!=0);

	if(instruction!="exit"){
		std::cout<<"input file path"<<endl<<">>";
		cin>>filePath;

		if(instruction == "read"){
			std::cout<<"start reading file "<<filePath<<endl;
			
			this->send_request(r, filePath);
		}
		else if(instruction == "write"){
			std::cout<<"start writing file "<<filePath<<endl;

			this->send_request(w, filePath);
		}
	}
	close(this->sock);
}

int main(){
	string serverIP("127.0.0.1");
	int serverPort=2333;
	
	Client* User = new Client(serverIP, serverPort);
	User->do_request();

	
	
	return 0;
}