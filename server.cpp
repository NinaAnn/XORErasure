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
#include <vector>
#include <map>

#define maxbuf 128

using namespace std;

string upload_file(const string filePath);

typedef class FileInfo{
	public:
        int fileLength;
        int dataPlace;
        string fileName;
        FileInfo(int len, int place, string name){
            this->fileLength = len;
            this->dataPlace = place;
            this->fileName = name;
	}
}FileInfo;

class NameNode{
private:
	int sock;

    struct sockaddr_in serverAdd;
    struct sockaddr_in clientAdd;
	int** encodingMatrix;
    map<string, int> dirHash;
public:
	int dataNNumber;
	int curBlock;
	int VerfyNNumber;
    int fileNumber;
	string* dataNIP;
    vector<FileInfo> files;
	NameNode(int dataNNumber, int VerfyNNumber);
	string do_save_instruction(string fileName, string file);
	int seperate_file_into_blocks(string file, vector<string> &dividedFiles);
	int save_file(string file, string fileName, int blocksUse);
    void do_listen();
    string reconstruct_file(int downFileOrder, vector<string> files);

	string do_read_instruction(string fileName);


};


NameNode::NameNode(int dataNNumber, int VerfyNNumber){
    this->sock = socket(AF_INET, SOCK_STREAM, 0);

	this->dataNNumber = dataNNumber;
	this->curBlock = 0;
	this->VerfyNNumber = VerfyNNumber;
    this->fileNumber = 0;

    
}

void NameNode::do_listen(){
    string IP("127.0.0.1");
    int port = 2333;
    this->serverAdd.sin_family = AF_INET;
    this->serverAdd.sin_addr.s_addr = inet_addr(IP.c_str());
    this->serverAdd.sin_port = htons(port);
    bind(this->sock, (struct sockaddr*)&(this->serverAdd), sizeof(this->serverAdd));

    listen(this->sock, 20);
    cout << "Listen thread initialized."<<endl;

    

    char buf[maxbuf];
    while(true){
        struct sockaddr_in clientAdd;
        socklen_t clientAddSize = sizeof(clientAdd);
        int clientSock = accept(this->sock, (struct sockaddr*)&clientAdd, &clientAddSize);
        cout << "accept " << clientSock << endl;

        memset(buf, 0, sizeof(char));
        read(clientSock, buf, sizeof(buf) - 1);
        char instruction = buf[0];
        string message(buf);
        string filePath = message.substr(1, message.length()-1);
        if(instruction == 'r'){
            string res;
            res = this->do_read_instruction(filePath);
            char resbuf[maxbuf];
            memset(resbuf, 0, sizeof(resbuf));
            strcpy(resbuf, res.c_str());
            write(clientSock, resbuf, sizeof(resbuf));
        }
        else if(instruction == 'w'){
            string str = upload_file(filePath);
            string res;
            res = this->do_save_instruction(filePath, str);
            char resbuf[maxbuf];
            memset(resbuf, 0, sizeof(resbuf));
            strcpy(resbuf, res.c_str());
            write(clientSock, resbuf, sizeof(resbuf));
        }
    }

}

string NameNode::do_save_instruction(string filePath, string file){
    int len=file.length();
    vector<string> dividedFiles(0);
	int blocks = seperate_file_into_blocks(file, dividedFiles);
    string verifyBlock(blocks,'0');
    int i,j;
    for(j=0; j<blocks; j++){
        char xorj = dividedFiles[0][j];
        for(i=1; i<this->dataNNumber; i++){
            char tmpi = dividedFiles[i][j];
            xorj = char(xorj^tmpi);
            verifyBlock[j] = xorj;
        }
    }
    dividedFiles.push_back(verifyBlock);

    for(i=0; i<=this->dataNNumber; i++){
        string saveName("out.txt");
        string nodeOrder = to_string(i);
        string fileName = nodeOrder.append(saveName);
        ofstream out(fileName, ios::app);  
        string curBlock = dividedFiles[i];
        if (out.is_open()){
            out << curBlock; 
            out.close();  
        }
        else{
            return("error opening file");
        }

    }
    FileInfo thisFile(len, this->curBlock, filePath);
    this->files.push_back(thisFile);
    this->dirHash[filePath] = this->fileNumber;
    this->fileNumber += 1;
    this->curBlock += blocks;
    return("File is successfully written");
}

int NameNode::seperate_file_into_blocks(string file, vector<string> &dividedFiles){
	int len = file.length();
    cout<<"file length:"<<len<<endl;
	// block number in each dataNode
    int blocks;
    int divide = len/this->dataNNumber;
    int rest = len-divide*this->dataNNumber;
    int empty = 0;
    if(rest>0){
        blocks = divide + 1;
        empty = blocks*this->dataNNumber - len;
    }
	else{
        blocks = divide;
    }

    int i;
    for(i=0; i<this->dataNNumber; i++){
        string tmpS;
        if(i== (this->dataNNumber - 1) && rest>0){
            tmpS = file.substr(0 + i*blocks, blocks-rest);
            string s2(empty, '0');
            tmpS.append(s2);
        }
        else{
            tmpS = file.substr(0 + i*blocks, blocks);
        }        
        dividedFiles.push_back(tmpS);
    }
    return blocks;
	
}

string NameNode::do_read_instruction(string filePath){
    vector<string> nodeFiles(0);
    if(this->fileNumber==0){
        cout<<"no file is stored now";
        return "none";
    }

    int fileOrder = this->dirHash[filePath];
    int fileStart = this->files[fileOrder].dataPlace;
    int fileLength = this->files[fileOrder].fileLength;

    int i,j;
    int down = -1;

    for(j=0; j<=this->dataNNumber; j++){
        string saveName("out.txt");
        string nodeOrder = to_string(j);
        string filePath = nodeOrder.append(saveName);
        ifstream in(filePath);
        string s;
        if (! in.is_open())
            {
                cout<<j<<"th node is down"<<endl;
                if(down==-1){
                    down = j;
                    continue;
                }
                else{
                    cout<<"two nodes are down, unable to recover!"<<endl;
                    exit (1);
                }
            }
        ostringstream buf;
        char ch;
        while(buf&&in.get(ch)){
            buf.put(ch);
        }
        nodeFiles.push_back(buf.str());
    }

    if(down!= -1){
        string recovered = this->reconstruct_file(down, nodeFiles);
        if(down != this->dataNNumber){
            nodeFiles[this->dataNNumber - 1]= recovered;
        }
    }

    int order = fileStart + i;
    int blocks;
    int divide = fileLength/this->dataNNumber;
    int rest = fileLength-divide*this->dataNNumber;
    if(rest>0){
        blocks = divide + 1;
    }
    else{
        blocks = divide;
    }

    string file="";
    for(i=0; i<this->dataNNumber; i++){
        string newFile;
        if(i == this->dataNNumber-1 && rest>0){
            int empty = blocks*this->dataNNumber - fileLength;
            newFile = nodeFiles[i].substr(fileStart, blocks-empty);
        }
        else{
            newFile = nodeFiles[i].substr(fileStart, blocks);
        }
        file.append(newFile);
        
    }
    cout<<file<<endl;
    return file;
}

string NameNode::reconstruct_file(int downFileOrder, vector<string> files){
    int i,j;
    int fileLen = files[0].length();
    string missingFile(fileLen,'0');
    for(j=0; j<fileLen; j++){
        char xorj = files[0][j];
        for(i=1; i<this->dataNNumber; i++){
            char tmpi = files[i][j];
            xorj = char(xorj^tmpi);
            missingFile[j] = xorj;
        }
    }
    string saveName("out.txt");
    string nodeOrder = to_string(downFileOrder);
    string fileName = nodeOrder.append(saveName);
    ofstream out(fileName);  
    if (out.is_open()){
        out << missingFile; 
        out.close();  
    }
    return missingFile;
}



string upload_file(const string filePath){
	string s;
	ifstream in(filePath);
	if (! in.is_open())
        { cout << "Error opening file"; exit (1); }
    ostringstream buf;
	char ch;
	while(buf&&in.get(ch))
		buf.put(ch);
	return buf.str();
}

int main(){
    int dataNNumber;
    cout << "set distribute node number:";
    cin >> dataNNumber;
    int verifyNNumber = 1;
    NameNode server(dataNNumber, verifyNNumber);
    server.do_listen();

    //test
    // string filePath = "./test.txt";
    // string filePath2 = "./test2.txt";
	// string str = upload_file(filePath);
    // string str2 = upload_file(filePath2);
    // server.do_save_instruction(filePath, str);
    // server.do_save_instruction(filePath2, str2);
    // if(remove(filePath2.c_str())==0){
    //     cout<<"test2.txt is deleted"<<endl;
    // }
    // server.do_read_instruction(filePath2);


}