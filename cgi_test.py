#!/usr/bin/Python

# Author: Phage
# Project: Simple CGI test of POST requests.
# Date: 1/7/2014

# Import modules for CGI handling 
import cgi, cgitb 

# Create instance of FieldStorage 
form = cgi.FieldStorage() 

# Get data from fields
username = form.getvalue('username')
password  = form.getvalue('password')

print "Content-type:text/html\r\n\r\n"
print "<html>"
print "<head>"
print "<title>Python CGI-test</title>"
print "</head>"
print "<body>"
print "<h1>Username: %s Password: %s</h1>" % (username, password)
print "</body>"
print "</html>"


