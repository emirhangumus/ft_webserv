import socket

def send_malformed_http_request(host, port, request_type):
    # Create a raw TCP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # Connect to the specified host and port
        s.connect((host, port))

        # Craft a malformed HTTP request based on the type
        if request_type == 1:
            malformed_request = "GETTT /index.html HTTP/1.1\r\n"  # Invalid HTTP method
        elif request_type == 2:
            malformed_request = "GET /http://example.com HTTP/1.1\r\n"  # Malformed URI XXXXXXXXXXXXXXXXXXXXX
        elif request_type == 3:
            malformed_request = "GET  /index.html HTTP/1.1\r\n"  # Extra space between method and URI
        elif request_type == 4:
            malformed_request = "GET /index.html HTTP/1.0\r\n"  # Invalid HTTP version
        elif request_type == 5:
            malformed_request = "GET /index.html HTTP/1.1\r\n"  # Missing Host header
        elif request_type == 6:
            malformed_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n"  # Empty headers XXXXXXXXXXXXXXXXXXXXX
        elif request_type == 7:
            malformed_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n"  # Multiple blank headers XXXXXXXXXXXXXXXXXXXXX
        elif request_type == 8:
            malformed_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nCookie: foo;=bar\r\n"  # Malformed cookie header
        elif request_type == 9:
            malformed_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nContent-Length: -1\r\n"  # Invalid Content-Length XXXXXXXXXXXXXXXXXXXXX
        elif request_type == 10:
            malformed_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nContent-Length: 100\r\nContent-Length: 200\r\n"  # Multiple Content-Length XXXXXXXXXXXXXXXXXXXXX
        elif request_type == 11:
            malformed_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nX-Header: value\r\nX-Header: value2\r\n"  # Invalid character in headers (newline)
        elif request_type == 12:
            malformed_request = "GET /index%zzzz.html HTTP/1.1\r\nHost: example.com\r\n"  # Invalid URL encoding XXXXXXXXXXXXXXXXXXXXX
        elif request_type == 13:
            malformed_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n\r\n"  # Extra newlines XXXXXXXXXXXXXXXXXXXXX
        elif request_type == 14:
            malformed_request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nCookie: badcookie==value\r\n"  # Invalid cookie syntax XXXXXXXXXXXXXXXXXXXXX
        elif request_type == 15:
            malformed_request = "GET /index.html?param==value HTTP/1.1\r\nHost: example.com\r\n"  # Invalid query parameter XXXXXXXXXXXXXXXXXXXXX
        else:
            malformed_request = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n"  # Default valid request

        # Construct headers
        # headers = "Host: {}\r\n".format(host)

        # Combine the malformed request line and headers
        request = malformed_request + ""
        request += "Connection: close\r\n\r\n"

        # Send the request
        s.sendall(request.encode())

        # Receive the response
        response = s.recv(4096)
        print(f"Response for request type {request_type}:")
        print(response.decode(errors="ignore"))
        
# Test 20 different malformed requests
# for i in range(1, 16):  # Adjust for testing 15 types of malformed requests
#     send_malformed_http_request("localhost", 8082, i)
send_malformed_http_request("localhost", 8082, 12)
