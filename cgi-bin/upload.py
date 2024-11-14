#!/usr/bin/python3
import cgi
import os
import cgitb
cgitb.enable()  # Enable detailed error messages

print("<html><body>")

try:
    # Initialize FieldStorage
    form = cgi.FieldStorage()
    print("<h2>Form Keys Available:</h2>", list(form.keys()))

    # Check for the uploaded file item
    fileitem = form['filename'] if 'filename' in form else None
    
    # Check if fileitem is valid and has a filename attribute
    if fileitem is not None and fileitem.file and fileitem.filename:
        # Ensure the upload directory exists
        upload_dir = os.path.join(os.getcwd(), 'cgi-bin', 'tmp')
        os.makedirs(upload_dir, exist_ok=True)

        # Set the full file path
        filepath = os.path.join(upload_dir, os.path.basename(fileitem.filename))

        # Write the uploaded file content to disk
        with open(filepath, 'wb') as f:
            f.write(fileitem.file.read())
        
        message = f"The file '{fileitem.filename}' was uploaded successfully to {upload_dir}."
    else:
        message = "No file was uploaded or 'filename' field is missing."
    
    print("<h1>", message, "</h1>")

except Exception as e:
    print("<h2>Debug Information:</h2>")
    print("<pre>")
    print("Error occurred:", str(e))
    print("</pre>")

print("</body></html>")
