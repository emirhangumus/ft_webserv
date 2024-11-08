#include <iostream>
#include <string>
#include <map>
#include <stdexcept>
#include <sstream>
#include <cctype>

long long convertSizeToBytes(const std::string& sizeStr) {
    // Define conversion factors for each unit
    std::map<std::string, long long> unitMap;
    unitMap["KB"] = 1024LL;
    unitMap["MB"] = 1024LL * 1024;
    unitMap["GB"] = 1024LL * 1024 * 1024;
    unitMap["TB"] = 1024LL * 1024 * 1024 * 1024;

    // Find the first non-digit character to separate number and unit
    size_t i = 0;
    while (i < sizeStr.size() && (isdigit(sizeStr[i]) || sizeStr[i] == '.')) ++i;
    
    if (i == 0 || i == sizeStr.size()) {
        throw std::invalid_argument("Invalid size format");
    }

    // Extract numeric part and unit part
    std::istringstream numStream(sizeStr.substr(0, i));
    double number;
    numStream >> number;

    std::string unit = sizeStr.substr(i);
    for (size_t j = 0; j < unit.size(); ++j) {
        unit[j] = toupper(unit[j]);
    }

    // Find the unit in the map and apply the conversion factor
    std::map<std::string, long long>::iterator it = unitMap.find(unit);
    if (it == unitMap.end()) {
        throw std::invalid_argument("Invalid or unsupported unit");
    }

    // Calculate the result in bytes
    return static_cast<long long>(number * it->second);
}

int main() {
    try {
        std::string size = "200MB";
        std::cout << size << " = " << convertSizeToBytes(size) << " bytes" << std::endl;
        
        size = "2GB";
        std::cout << size << " = " << convertSizeToBytes(size) << " bytes" << std::endl;
        
        size = "5KB";
        std::cout << size << " = " << convertSizeToBytes(size) << " bytes" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    
    return 0;
}
