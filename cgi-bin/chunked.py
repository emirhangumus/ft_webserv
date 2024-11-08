#!/usr/bin/env python3
import os
import sys

def main():
    # Print CGI headers
    print("Content-Type: text/plain")
    print()  # Blank line separates headers from the body

    # Retrieve environment variables
    request_method = os.getenv("REQUEST_METHOD", "GET")
    query_string = os.getenv("QUERY_STRING", "")
    script_name = os.getenv("SCRIPT_NAME", "")

    print(f"Request Method: {request_method}")
    print(f"Script Name: {script_name}")
    print(f"Query String: {query_string}")
    print("Received Body:")

    # Read from stdin (body data sent by POST request)
    content_length = os.getenv("CONTENT_LENGTH", None)
    if content_length:
        # Read the specified content length
        body = sys.stdin.read(int(content_length))
    else:
        # Read until EOF
        body = sys.stdin.read()

    print(body)  # Output the body to the response

if __name__ == "__main__":
    main()
