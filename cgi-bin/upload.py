#!/usr/bin/python3
import cgi
import os
import cgitb
cgitb.enable()  # Enable detailed error messages

# print("Content-Type: text/html;charset=utf-8\n")  # Note the extra newline
try:
    form = cgi.FieldStorage()
    
    # Get file item directly - don't use getvalue()
    fileitem = form['filename'] if 'filename' in form else None
    
    # Test if the file was uploaded
    if fileitem.filename:
        # Make sure the directory exists
        upload_dir = os.path.join(os.getcwd(), 'cgi-bin', 'tmp')
        os.makedirs(upload_dir, exist_ok=True)
        
        # Create the full file path
        filepath = os.path.join(upload_dir, os.path.basename(fileitem.filename))
        
        # Write the file
        with open(filepath, 'wb') as f:
            f.write(fileitem.file.read())
        
        message = f'The file "{os.path.basename(fileitem.filename)}" was uploaded to {upload_dir}'
    else:
        message = 'No file was uploaded'
        print("<p>Form keys available:", list(form.keys()), "</p>")
        print("<p>Form contents:", form, "</p>")

except Exception as e:
    message = f'Error occurred: {str(e)}'
    # Print the form data for debugging
    print("<h2>Debug Information:</h2>")
    print("<pre>")
    print("Form keys:", list(form.keys()) if 'form' in locals() else "Form not created")
    print("Environment variables:")
    for key, value in os.environ.items():
        print(f"{key}: {value}")
    print("</pre>")

print("<h1>", message, "</h1>")