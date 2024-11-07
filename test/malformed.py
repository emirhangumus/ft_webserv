import socket

def send_malformed_http_request(host, port):
    # Create a raw TCP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # Connect to the specified host and port
        s.connect((host, port))

        # Craft a malformed HTTP request line (e.g., using an invalid HTTP method or malformed URI)
        malformed_request = "GET /index.html HTTP/1.1\r\n"  # Invalid method "GETTT"
        # headers = "Host: {}\r\n\r\n".format(host)
        

        # Combine the malformed request line and headers
        request = malformed_request + ""
        request += "Connection: close\r\n\r\n"

        # Send the request
        s.sendall(request.encode())

        # Receive the response
        response = s.recv(4096)
        print("Response from server:")
        print(response.decode(errors="ignore"))

def send_normal_http_request(host, port):
    # Create a raw TCP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # Connect to the specified host and port
        s.connect((host, port))

        # Craft a normal HTTP request line
        normal_request = "GET /index.html HTTP/1.1\r\n"
        headers = "Host: {}\r\n".format(host)
        headers += "Connection: close\r\n\r\n"  # Close connection after response

        # Combine the request line and headers
        request = normal_request + headers

        # Send the request
        s.sendall(request.encode())

        # Receive the response
        response = s.recv(4096)
        print("Response from server:")
        print(response.decode(errors="ignore"))

# Example usage
send_malformed_http_request("localhost", 8082)
send_normal_http_request("localhost", 8082)
