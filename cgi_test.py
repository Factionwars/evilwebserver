#!/usr/bin/Python

# Author: Phage@evilzone.org
# Project: Simple CGI test of POST requests.
# Date: 1/8/2014

import cgi, cgitb 

form = cgi.FieldStorage() 

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


