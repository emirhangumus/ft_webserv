#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>

std::string unchunkRequest(const std::string& chunkedData) {
    std::istringstream stream(chunkedData);
    std::ostringstream bodyStream;
    std::string line;

    while (true) {
        // Read the chunk size line
        std::getline(stream, line);
        int chunkSize;
        std::istringstream(line) >> std::hex >> chunkSize;

        // Stop if the chunk size is zero (end of chunks)
        if (chunkSize == 0) {
            // Consume the trailing CRLF after the last chunk
            std::getline(stream, line); // Final CRLF
            break;
        }

        // Read exactly `chunkSize` bytes for the current chunk
        char* buffer = new char[chunkSize];
        stream.read(buffer, chunkSize);
        bodyStream.write(buffer, chunkSize);
        delete[] buffer;

        // Consume the trailing CRLF after the chunk
        stream.ignore(2); // Skip "\r\n"
    }

    return bodyStream.str();
}

int main() {
    std::string chunkedRequest = 
        "7\r\nHello, \r\n"
        "4\r\nthis\r\n"
        "6\r\n is a \r\n"
        "4\r\ntest\r\n"
        "0\r\n\r\n";

    std::string unchunkedBody = unchunkRequest(chunkedRequest);
    std::cout << "Unchunked body:\n" << unchunkedBody << std::endl;

    return 0;
}
