#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <unistd.h>

#define HOST_NAME_MAX 50

#define BUFFSIZE 10000000

using namespace std;

typedef struct{
	char destination[HOST_NAME_MAX];
	int cost;
} route_message;

int main(int argc, char const* argv[]){
	if(argc != 2){
		cout<<"The program takes only parameter which is the port number."<<endl;
		exit(EXIT_FAILURE);
	}

	int portNumber = atoi(argv[1]);

	int server_fd;
	int opt = 1;
	if((server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		cout<<"Socket creating failed."<<endl;
		exit(EXIT_FAILURE);
	}


	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		cout<<"Error Assigning port"<<endl;
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address;
	memset((char*)&address, 0, sizeof(address));

	int addlen = sizeof(address);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(portNumber);

	if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
		cout<<"Binding Failed"<<endl;
		exit(EXIT_FAILURE);
	}

	int recvlen;

	route_message buffer[4];

	struct sockaddr_in remaddr;
	socklen_t raddrlen = sizeof(remaddr);
	while(1){
		cout<<"Receiving on port: "<<portNumber<<endl;
		recvlen = recvfrom(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, &raddrlen);

		cout<<"Message Received"<<endl;
		for(int i = 0; i < 4; i++){
			cout<<buffer[i].destination<<"\t"<<buffer[i].cost<<endl;
		}
		break;
	}
	close(server_fd);


	return 0;
}
