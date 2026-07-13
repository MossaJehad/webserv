#include <string>
#include <iostream>
#include <cctype>
#include <algorithm>
#include <vector>

#include "ConfigParser.hpp"

int main() {
	

std::string semicolonStr = "Hello, World";
    
    if (!semicolonStr.empty() && semicolonStr.back() == ';') {
        // semicolonStr.pop_back(); // Removes the ';' from the end of the string
    }
    
    std::cout << "Stripped string: '" << semicolonStr.back() << "'" << std::endl;
	return 0;
}