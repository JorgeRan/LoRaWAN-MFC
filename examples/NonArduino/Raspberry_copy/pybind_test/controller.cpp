#include <iostream>
#include <string>
#include <sstream>

using namespace std;

void splitString(string& input, char delimiter, string arr[], int& index){
	istringstream stream(input);
	
	string token;
	
	while(getline(stream, token, delimiter)) {
		arr[index++] = token;
	}
}


int main() {
	std::string line;
	std::cout << "C++ controller ready \n";
	char delimiter = '-';
	string arrayOfSubStr[100];
	int index = 0;
	
	
	
	while(std::getline(std::cin, line) {
		
		std::cout << "Received from Python: " << arrayOfSubStr[1] << std::endl;
	}
}
