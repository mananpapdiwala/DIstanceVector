#include <iostream>
#include <cstdlib>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEFAULT_PARAMS 7
#define HOST_NAME_MAX 50

using namespace std;
//Grapg data structure - 2D array
int** graph;

int number_of_nodes;

//IP - index map
typedef struct{
	char* host_name;
	bool isNeighbour;
} node;

node* graph_node_map;

typedef struct{
	char* destination;
	char* nextHop;
	int cost;
	int ttl;
} Route_entry;

Route_entry* RoutingTable;

typedef struct{
	char destination[HOST_NAME_MAX];
	int cost;
} route_message;

typedef struct{
	int socket_id;
	int period;
	int TTL;
	int infinity;
	int portNumber;
	pthread_mutex_t* routing_mutex;
} thread_parameter;

void freeMemory();
void freeSocket(int socket_id);
void error_handler(string message, bool callFreeMemory);
string getLocalHostName();
string getIPAddress(char *hostname);
void printGraph();
void printRoutingTable();
void set_graphNode(node& graph_node, string host_name, bool isNeighbour);
void read_config_file(string file_name, vector<string>& config_lines);
void create_2D_array(int** array, int row, int col);
void initializeGraph(vector<string>& config_lines, int infinity);
int getNodeIndex(char* ip);
void initializeRoutingTable(int TTL);
void initialize(string configFile, int portNumber, int TTL, int infinity, int period, int poisonReverse);
int createSocket(int portNumber);
void sendAdvertisement(int socket_id, int portNumber, pthread_mutex_t* routing_mutex);
int calculate_buffer_size();
void* updateHandler(void* parameter);
void* receiveHandler(void* parameter);
void initializeThreads(int socket_id, int period, int TTL, int infinity, int portNumber);
bool bellmanFord(int source_index, int infinity, int TTL, pthread_mutex_t* routing_mutex);


void error_handler(string message, bool callFreeMemory){
	cerr<<message<<endl;
	cerr<<"Shutting down"<<endl;
	if(callFreeMemory) freeMemory();
	exit(EXIT_FAILURE);
}

string getLocalHostName(){
	char hostname[HOST_NAME_MAX];
	if(gethostname(hostname, HOST_NAME_MAX) < 0){
		perror("Error:");
		error_handler("Failed to get hostname", false);
	}
	return string(hostname);
}

string getIPAddress(string hostname) {
	struct addrinfo pointers, *host;
	int returnval;
	memset(&pointers, 0, sizeof pointers);
	pointers.ai_family = AF_UNSPEC;
	pointers.ai_socktype = SOCK_DGRAM;
	if ((returnval = getaddrinfo(hostname.c_str(), NULL, &pointers, &host)) != 0) {
		error_handler("Failed to resolve ip address for host", true);
	}

	struct sockaddr_in *addr = (struct sockaddr_in *) host->ai_addr;
	return string(inet_ntoa((struct in_addr) addr->sin_addr));
}

int calculate_buffer_size(){
	return (number_of_nodes * sizeof(route_message));
}

int getNodeIndex(char* ip){
	for(int i=0; i<number_of_nodes; i++){
		if(strcmp(graph_node_map[i].host_name, ip)==0){
			return i;
		}
	}
	return -1;
}

bool bellmanFord(int source_index, int infinity, int TTL, pthread_mutex_t* routing_mutex){
	/*
	int distance[number_of_nodes];
  string previous[number_of_nodes];
  for(int i =0; i< number_of_nodes; i++){
    distance[i] = infinity;
    previous[i] = "";
  }
	*/
  //distance[0] = 0;

	/*
  for(int vertex =0; vertex< number_of_nodes; vertex++){
    for(int i =0; i< number_of_nodes; i++){
      for(int j =0; j< number_of_nodes; j++){
        if(graph[i][j] == 1){
          if (distance[j] > distance[i]+graph[i][j]){
            distance[j] = distance[i]+graph[i][j];
            previous[j] = graph_node_map[i].host_name;
          }
        }
      }
    }

  }
	*/
  //print distance
	/* -
  for(int i =0; i< number_of_nodes; i++)
    cout<<distance[i]<<"\t";
  cout<<endl;
  for(int i =0; i< number_of_nodes; i++)
    cout<<previous[i]<<"\t";
  cout<<endl;
	*/


	bool changed = false;
	int curSourceDistance = graph[0][source_index];
	pthread_mutex_lock(routing_mutex);
	for(int i = 0; i < number_of_nodes; i++){
		if(curSourceDistance + graph[source_index][i] < graph[0][i]){
			changed = true;
			graph[0][i] = curSourceDistance + graph[source_index][i];
			if(RoutingTable[i].nextHop) free(RoutingTable[i].nextHop);
			RoutingTable[i].nextHop = strdup(graph_node_map[source_index].host_name);
			RoutingTable[i].cost = graph[0][i];
			RoutingTable[i].ttl = TTL;
		}
	}
	pthread_mutex_unlock(routing_mutex);
	return changed;
}


void printGraph(){
	cout<<"Printing Graph"<<endl;
	for(int i = 0; i < number_of_nodes; i++){
		for(int j = 0; j < number_of_nodes; j++){
			cout<<graph[i][j]<<"\t";
		}
		cout<<endl<<endl;
	}

	cout<<"Printing Graph Node Map"<<endl;
	for(int i = 0; i < number_of_nodes; i++){
		cout<<i<<"\t"<<graph_node_map[i].host_name<<"\t"<<graph_node_map[i].isNeighbour<<endl;
	}
	cout<<endl<<endl;
}

void printRoutingTable(){
	cout<<"Printing Routing Table"<<endl;
	for(int i = 0; i < number_of_nodes; i++){
		cout<<RoutingTable[i].destination<<"\t";
		RoutingTable[i].nextHop ? cout<<RoutingTable[i].nextHop : cout<<"NULL";
		cout<<"\t";
		cout<<RoutingTable[i].cost<<"\t"<<RoutingTable[i].ttl<<endl;
	}
	cout<<endl<<endl;
}

void set_graphNode(node& graph_node, string host_name, bool isNeighbour){
	graph_node.host_name = strdup(host_name.c_str());
	graph_node.isNeighbour = isNeighbour;
}

void read_config_file(string file_name, vector<string>& config_lines){
	string line;
  ifstream myfile (file_name.c_str());
  if (myfile.is_open())
  {
    while ( getline (myfile,line) )
    {
			config_lines.push_back(line);
      //cout << line << '\n';
    }
    myfile.close();
  }
  else {
		error_handler("Invalid file", false);
	}
}

void create_2D_array(int** array, int row, int col){
	graph = (int**)calloc(row, sizeof(int*));
	for(int i =0; i<row; i++){
		graph[i] = (int*)calloc(col, sizeof(int));
	}
}

void initializeGraph(vector<string>& config_lines, int infinity){
	create_2D_array(graph, config_lines.size()+1, config_lines.size()+1);
	graph_node_map = (node*)calloc(config_lines.size()+1, sizeof(node));

	string hostname = getLocalHostName();
	string local_ip = getIPAddress(hostname);
	//Initialize itself to 0
	set_graphNode(graph_node_map[0], local_ip, true);
	graph[0][0] = 0;
	//Parse config file data and initialize graph

	for(int i =1; i <= config_lines.size(); i++){
		char delim = ' ';
		int pos = config_lines[i-1].find(delim);
		string name  = config_lines[i-1].substr(0, pos);
		int neighbour = stoi(config_lines[i-1].substr(pos+1));
		string ip = getIPAddress(name);
		set_graphNode(graph_node_map[i], ip, neighbour ? true : false);
		graph[0][i] = neighbour ? 1 : infinity;
		for(int j = 0; j < number_of_nodes; j++){
			graph[i][j] = infinity;
		}
	}

	printGraph();
}

void initializeRoutingTable(int TTL){
	RoutingTable = (Route_entry*)calloc(number_of_nodes, sizeof(Route_entry));
	for(int i = 0; i < number_of_nodes; i++){
		RoutingTable[i].destination = strdup(graph_node_map[i].host_name);
		RoutingTable[i].nextHop = graph_node_map[i].isNeighbour ? strdup(graph_node_map[i].host_name) : NULL;
		RoutingTable[i].cost = graph[0][i];
		RoutingTable[i].ttl = TTL;
	}

	printRoutingTable();
}

void initialize(string configFile, int portNumber, int TTL, int infinity, int period, int poisonReverse){
	vector<string> config_lines;
	read_config_file(configFile, config_lines);
	number_of_nodes = config_lines.size()+1;
	initializeGraph(config_lines, infinity);
	initializeRoutingTable(TTL);
}

void initializeThreads(int socket_id, int period, int TTL, int infinity, int portNumber){
	pthread_mutex_t routing_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_t update_thread;
	pthread_t receive_thread;
	thread_parameter input;
	input.socket_id = socket_id;
	input.period = period;
	input.TTL = TTL;
	input.infinity = infinity;
	input.portNumber = portNumber;
	input.routing_mutex = &routing_mutex;
	int thread_id;
	if(thread_id = pthread_create(&update_thread, NULL, updateHandler, (void*)&input)){
			freeSocket(socket_id);
			error_handler("Update Thread Creation Failed.", true);
	}

	//Preparing receive thread -
	thread_parameter receive_input;
	receive_input.socket_id = socket_id;
	receive_input.portNumber = portNumber;
	receive_input.TTL = TTL;
	receive_input.infinity = infinity;
	receive_input.period = period;
	receive_input.routing_mutex = &routing_mutex;
	int recv_thread_id;
	if(recv_thread_id = pthread_create(&receive_thread, NULL, receiveHandler, (void*)&receive_input)){
			freeSocket(socket_id);
			error_handler("Receive Thread Creation Failed.", true);
	}

}

int createSocket(int portNumber){
	int socket_id;
	if((socket_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		error_handler("Socket creating failed.", true);
	}

	int opt = 1;
	if(setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		error_handler("Error Assigning port.", true);
	}

	struct sockaddr_in address;
	memset((char*)&address, 0, sizeof(address));

	int addlen = sizeof(address);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(portNumber);

	if(bind(socket_id, (struct sockaddr*)&address, sizeof(address)) < 0){
		error_handler("Binding Failed.", true);
	}

	return socket_id;
}

void sendAdvertisement(int socket_id, int portNumber, pthread_mutex_t* routing_mutex){
	route_message message[number_of_nodes];
	pthread_mutex_lock(routing_mutex);
	for(int i = 0; i < number_of_nodes; i++){
		strncpy(message[i].destination, RoutingTable[i].destination, HOST_NAME_MAX);
		message[i].cost = RoutingTable[i].cost;
	}
	pthread_mutex_unlock(routing_mutex);

	int buffsize = calculate_buffer_size();
	for(int i = 1; i < number_of_nodes; i++){
		if(graph_node_map[i].isNeighbour){
			struct sockaddr_in server_address;
		  socklen_t addrlen = sizeof(server_address);

		  memset((char*)&server_address, 0, sizeof(server_address));
		  server_address.sin_family = AF_INET;
		  server_address.sin_port = htons(portNumber);

		  if(inet_aton(graph_node_map[i].host_name, &server_address.sin_addr) < 0){
				freeSocket(socket_id);
				error_handler("Invalid address. This address is not supported.", true);
		  }

			//cout<<"Sending to destination: "<<graph_node_map[i].host_name<<endl;
			if(sendto(socket_id, message, buffsize, 0, (struct sockaddr*)&server_address, addrlen) <= 0){
				perror("Error:");
				freeSocket(socket_id);
				error_handler("Sending Failed", true);
			}

		}
	}


}

void* updateHandler(void* parameter){
	thread_parameter* input = (thread_parameter*)parameter;
	while(1){
		sleep(input->period);
		bool flag = false;
		pthread_mutex_lock(input->routing_mutex);
		for(int i = 1; i < number_of_nodes; i++){
			if(RoutingTable[i].ttl > 0){
				if(RoutingTable[i].cost != input->infinity) RoutingTable[i].ttl -= input->period;
				if(RoutingTable[i].ttl <= 0){
					flag = true;
					if(RoutingTable[i].nextHop) free(RoutingTable[i].nextHop);
					RoutingTable[i].nextHop = NULL;
					RoutingTable[i].cost = input->infinity;
					//RoutingTable[i].ttl = input->TTL;
					graph[0][i] = input->infinity;
					//graph_node_map[i].isNeighbour = false;
				}
			}

		}
		pthread_mutex_unlock(input->routing_mutex);
		sendAdvertisement(input->socket_id, input->portNumber, input->routing_mutex);
		//Call this once recieved message is implemented
		/*
		if(flag){
				cout<<"Printing Updated "<<endl;
		}
		*/
		printRoutingTable();

	}
}


void* receiveHandler(void* parameter){
	thread_parameter* input = (thread_parameter*)parameter;
	int socket_id = input->socket_id;
	route_message buffer[number_of_nodes];
	int recvlen;
	int buffsize = calculate_buffer_size();
	cout<<"Receive thread started handler-------->"<<endl;
	while(1){
		struct sockaddr_in remaddr;
		socklen_t raddrlen = sizeof(remaddr);
		recvlen = recvfrom(socket_id, buffer, buffsize, 0, (struct sockaddr*)&remaddr, &raddrlen);
		char* source_ip = strdup(inet_ntoa(remaddr.sin_addr));
		cout<<"Received Message from---------------->"<<source_ip<<endl;

		int source_index = getNodeIndex(source_ip);
		if (source_index == -1){
			freeSocket(socket_id);
			error_handler("Source IP index not found", true);
		}
		for(int i=0; i<number_of_nodes; i++){
			int dest_index = getNodeIndex(buffer[i].destination);
			if (dest_index == -1){
				freeSocket(socket_id);
				error_handler("Destination IP index not found", true);
			}
			graph[source_index][dest_index] = buffer[i].cost;
		}

		cout<<"Starting bellmanFord------->"<<endl;
		if(bellmanFord(source_index, input->infinity, input->TTL, input->routing_mutex)){
				cout<<"Started Send Advertisement------------->"<<endl;
				sendAdvertisement(socket_id, input->portNumber, input->routing_mutex);
		}


		free(source_ip);

	}
}



void freeMemory(){
	for(int i = 0; i < number_of_nodes; i++){
		free(graph[i]);
		free(graph_node_map[i].host_name);
		free(RoutingTable[i].destination);
		if(RoutingTable[i].nextHop) free(RoutingTable[i].nextHop);
	}
	free(graph);
	free(graph_node_map);
	free(RoutingTable);
}

void freeSocket(int socket_id){
	close(socket_id);
}

int main(int argc, char const* argv[]){
	if(argc != DEFAULT_PARAMS){
		cout<<"The program takes 7 arguments."<<endl;
		cout<<"Please run the program using format:"<<endl;
		cout<<"DistanceVectorRouting <config> <portnumber> <TTL> <infinity> <Period> <Poison Reverse>"<<endl;
		error_handler("Invalid number of arguments", false);
	}

	string configFile = string(argv[1]);
	int portNumber = atoi(argv[2]);
	int TTL = atoi(argv[3]);
	int infinity = atoi(argv[4]);
	int period = atoi(argv[5]);
	int poisonReverse = atoi(argv[6]);
	initialize(configFile, portNumber, TTL, infinity, period, poisonReverse);
	int socket_id = createSocket(portNumber);
	//sendAdvertisement(socket_id, portNumber);
	initializeThreads(socket_id, period, TTL, infinity, portNumber);

	while(1);


	freeMemory();
	freeSocket(socket_id);

	return 0;
}
