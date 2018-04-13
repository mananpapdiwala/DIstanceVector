#include <iostream>
#include <cstdlib>

#define DEFAULT_PARAMS 7

using namespace std;

void error_handler(string message){
	cerr<<message<<endl;
	cerr<<"Shutting down"<<endl;
	exit(EXIT_FAILURE);
};

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


	cout<<configFile<<endl;
	cout<<portNumber<<endl;
	cout<<TTL<<endl;
	cout<<infinity<<endl;
	cout<<period<<endl;
	cout<<poisonReverse<<endl;

	return 0;
}
