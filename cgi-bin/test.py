# Import modules for CGI handling
import cgi, cgitb
import os
from http import cookies
import uuid

# Create instance of FieldStorage
form = cgi.FieldStorage()

# Enable debugging
cgitb.enable()

# Initialize the session
cookie = cookies.SimpleCookie(os.environ.get('HTTP_COOKIE'))
session_id = cookie.get('session_id')

print("Content-type: text/html\n")
# print the cookies
print("Cookies: ")
print(form.getvalue('test'))