#include <iostream>
#include <cstdlib>
#include <fstream>
#include<stdio.h>
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

//IP - index map
typedef struct{
	char* ip_address;
	bool isNeighbour;
} node;
node* graph_node_map;

void error_handler(string message){
	cerr<<message<<endl;
	cerr<<"Shutting down"<<endl;
	exit(EXIT_FAILURE);
};
void set_graphNode(node& graph_node, string ip, bool isNeighbour){
	graph_node.ip_address = (char*)calloc(ip.size()+1, sizeof(char));
	strcpy(graph_node.ip_address, ip.c_str());
	//graph_node.ip_address = ip;
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
      cout << line << '\n';
    }
    myfile.close();
  }
  else {
		error_handler("Invalid file");
	}
}

void create_2D_array(int** graph, int row, int col){
	graph = (int**)calloc(row, sizeof(int*));
	for(int i =0; i<row; i++){
		graph[i] = (int*)calloc(col, sizeof(int));
	}
}

void initialize(string configFile, int portNumber, int TTL, int infinity, int period, int poisonReverse){
	vector<string> config_lines;
	read_config_file(configFile, config_lines);
	create_2D_array(graph, config_lines.size()+1, config_lines.size()+1);
	graph_node_map = (node*)calloc(config_lines.size()+1, sizeof(node));
	//Initialize itself to 0
	set_graphNode(graph_node_map[0], "A", true);
	for(int i =0; i <config_lines.size(); i++){
	}
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

	return 0;
}
