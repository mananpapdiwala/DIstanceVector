#include <iostream>
#include <cstdlib>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <vector>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define DEFAULT_PARAMS 7

using namespace std;
//Grapg data structure - 2D array
int** graph;

int number_of_nodes;

//IP - index map
typedef struct{
	char* ip_address;
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

void error_handler(string message){
	cerr<<message<<endl;
	cerr<<"Shutting down"<<endl;
	exit(EXIT_FAILURE);
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
		cout<<i<<"\t"<<graph_node_map[i].ip_address<<"\t"<<graph_node_map[i].isNeighbour<<endl;
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

void set_graphNode(node& graph_node, string ip, bool isNeighbour){
	graph_node.ip_address = strdup(ip.c_str());
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
		error_handler("Invalid file");
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
	//Initialize itself to 0

	set_graphNode(graph_node_map[0], "A", true);
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
		RoutingTable[i].destination = strdup(graph_node_map[i].ip_address);
		RoutingTable[i].nextHop = graph_node_map[i].isNeighbour ? strdup(graph_node_map[i].ip_address) : NULL;
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

void freeMemory(){
	for(int i = 0; i < number_of_nodes; i++){
		free(graph[i]);
		free(graph_node_map[i].ip_address);
		free(RoutingTable[i].destination);
		if(RoutingTable[i].nextHop) free(RoutingTable[i].nextHop);
	}
	free(graph);
	free(graph_node_map);
	free(RoutingTable);
}

int main(int argc, char const* argv[]){
	if(argc != DEFAULT_PARAMS){
		cout<<"The program takes 7 arguments."<<endl;
		cout<<"Please run the program using format:"<<endl;
		cout<<"DistanceVectorRouting <config> <portnumber> <TTL> <infinity> <Period> <Poison Reverse>"<<endl;
		error_handler("Invalid number of arguments");
	}

	string configFile = string(argv[1]);
	int portNumber = atoi(argv[2]);
	int TTL = atoi(argv[3]);
	int infinity = atoi(argv[4]);
	int period = atoi(argv[5]);
	int poisonReverse = atoi(argv[6]);
	initialize(configFile, portNumber, TTL, infinity, period, poisonReverse);

	freeMemory();

	return 0;
}
