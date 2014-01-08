#!/usr/bin/Python

# Author: Phage
# Project: Simple CGI test of POST requests.
# Date: 1/7/2014

import cgi, cgitb

form = cgi.FieldStorage()

username = form.getValue('username')
password = form.getValue('password')

print "Content-type:text/html\r\n\r\n"
print "<html>"
print "<head>"
print "<title>CGI test for Evilwebserver</title>"
print "</head>"
print "<body>"
print "<h2>Username: %s Password: %s</h2>" % (username, password)
print "</body>"
print "</html>"



