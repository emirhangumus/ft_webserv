# Import modules for CGI handling
import cgi, cgitb
import os
from http import cookies
import uuid
import json

# Create instance of FieldStorage
form = cgi.FieldStorage()

# Enable debugging
cgitb.enable()

# Initialize the session
cookie = cookies.SimpleCookie(os.environ.get('HTTP_COOKIE'))
session_id = cookie.get('session_id')

# File to store user data
user_db_file = 'cgi-bin/db/user_db.json'

# File to store session data
session_db_file = 'cgi-bin/db/session_db.json'

# Function to read user data from file
def read_user_db():
    if os.path.exists(user_db_file):
        with open(user_db_file, 'r') as file:
            return json.load(file)
    return {}

# Function to write user data to file
def write_user_db(user_db):
    with open(user_db_file, 'w') as file:
        json.dump(user_db, file)

# Function to read session data from file
def read_session_db():
    if os.path.exists(session_db_file):
        with open(session_db_file, 'r') as file:
            return json.load(file)
    return {}

# Function to write session data to file
def write_session_db(session_db):
    with open(session_db_file, 'w') as file:
        json.dump(session_db, file)

# Load user data from file
user_db = read_user_db()

# Load session data from file
session_db = read_session_db()

# Function to register a new user
def register(username, password):
    if username in user_db:
        return "Username already exists."
    user_db[username] = password
    write_user_db(user_db)
    return "Registration successful."

# Function to login a user
def login(username, password):
    if username not in user_db:
        return "Username does not exist."
    if user_db[username] != password:
        return "Incorrect password."
    session_db[session_id] = {'username': username}
    write_session_db(session_db)
    return "Login successful."

def signout():
    if session_id in session_db:
        del session_db[session_id]
        write_session_db(session_db)
        return "Sign out successful."
    return "No active session."

# Check if session exists
if session_id is None:
    # Create a new session
    session_id = str(uuid.uuid4())
    cookie['session_id'] = session_id
    print(cookie)
else:
    # Retrieve existing session
    session_id = session_id.value

# Retrieve session data
user_data = session_db.get(session_id, {})

# Handle form data for registration and login
action = form.getvalue('action')
username = form.getvalue('username')
password = form.getvalue('password')

if action == 'register':
    message = register(username, password)
elif action == 'login':
    message = login(username, password)
elif action == 'signout':
    message = signout()
else:
    message = ""

# Display user information or login/register form
if 'username' in user_data:
    print(f"<h1>Welcome, {user_data['username']}!</h1>")
    print(f"<p>Session ID: {session_id}</p>")
    print("""
    <form method="post" action="">
        <button type="submit" name="action" value="signout">Sign out</button>
    </form>
    """)
else:
    print("""
    <form method="post" action="">
        <input type="text" name="username" placeholder="Username" required><br>
        <input type="password" name="password" placeholder="Password" required><br>
        <button type="submit" name="action" value="register">Register</button>
        <button type="submit" name="action" value="login">Login</button>
    </form>
    """)
print(f"<p>{message}</p>")