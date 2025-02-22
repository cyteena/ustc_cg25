#include "PolynomialList.h"
#include <list>
#include <iostream>
#include <filesystem>


int main(int argc, char** argv) {

	std::cout << "Current Path: "<< std::filesystem::current_path() << std::endl;
	std::string current_path = std::filesystem::current_path().string();
	PolynomialList p1, p2;
	if (current_path.substr(current_path.length() - 7) == "project"){
		p1 = PolynomialList("./data/P3.txt");
		p2 = PolynomialList("./data/P4.txt");
	}
	if (current_path.substr(current_path.length() - 3) == "bin"){
		p1 = PolynomialList("../data/P3.txt");
		p2 = PolynomialList("../data/P4.txt");
	}
	
	PolynomialList p3;
	p1.Print();
	p2.Print();

	p3 = p1 + p2;
	p3.Print();
	p3 = p1 - p2;
	p3.Print();

	p3 = p1 * p2;
	p3.Print();

	return 0;
}