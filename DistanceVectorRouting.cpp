#include <iostream>
#include <cstdlib>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PARAMS 7
#define HOST_NAME_MAX 100

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

void freeMemory();
void freeSocket(int socket_id);
void error_handler(string message, bool callFreeMemory);
string getLocalHostName();
void printGraph();
void printRoutingTable();
void set_graphNode(node& graph_node, string host_name, bool isNeighbour);
void read_config_file(string file_name, vector<string>& config_lines);
void create_2D_array(int** array, int row, int col);
void initializeGraph(vector<string>& config_lines, int infinity);
void initializeRoutingTable(int TTL);
void initialize(string configFile, int portNumber, int TTL, int infinity, int period, int poisonReverse);
int createSocket(int portNumber);

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
	string hostname = getLocalHostName();
	create_2D_array(graph, config_lines.size()+1, config_lines.size()+1);
	graph_node_map = (node*)calloc(config_lines.size()+1, sizeof(node));
	//Initialize itself to 0
	set_graphNode(graph_node_map[0], hostname, true);
	graph[0][0] = 0;
	//Parse config file data and initialize graph

	for(int i =1; i <= config_lines.size(); i++){
		char delim = ' ';
		int pos = config_lines[i-1].find(delim);
		string name  = config_lines[i-1].substr(0, pos);
		int neighbour = stoi(config_lines[i-1].substr(pos+1));
		set_graphNode(graph_node_map[i], name, neighbour ? true : false);
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

	freeMemory();
	freeSocket(socket_id);

	return 0;
}
