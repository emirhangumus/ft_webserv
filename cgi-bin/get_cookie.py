#! /usr/bin/python3

import os
from http import cookies
# Import modules for CGI handling 
import cgi, cgitb 

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
key = form.getvalue('key')
cookie = cookies.SimpleCookie()
if 'HTTP_COOKIE' in os.environ:
    cookie.load(os.environ["HTTP_COOKIE"])
if key in cookie:
    print("Content-Type: text/html\r\n", end='')
    print("\r\n\r\n", end='')
    print("The Value of Cookie", key, "is", cookie[key].value)
else:
    print("Content-Type: text/html\r\n", end='')
    print("\r\n\r\n", end='')
    print("Cookie was not found !")
