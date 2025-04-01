#!/bin/bash

mkdir -p runn/data

echo "2" > runn/data/timezone.txt
echo "South Africa" >> runn/data/timezone.txt
echo "Pretoria" >> runn/data/timezone.txt

# Compile C++ CGI scripts
g++ -Wall -g -std=c++11 -pedantic src/showtime.cpp -o runn/showtime.cgi
g++ -Wall -g -std=c++11 -pedantic src/setsa.cpp -o runn/setsa.cgi
g++ -Wall -g -std=c++11 -pedantic src/setgh.cpp -o runn/setgh.cgi

# Replace hardcoded path with the relative runn/data path
sed -i 's|/var/www/data/timezone.txt|runn/data/timezone.txt|g' runn/showtime.cgi
sed -i 's|/var/www/data/timezone.txt|runn/data/timezone.txt|g' runn/setsa.cgi
sed -i 's|/var/www/data/timezone.txt|runn/data/timezone.txt|g' runn/setgh.cgi

# Execute showtime.cgi and save the output
./runn/showtime.cgi > runn/output.html

echo "runn completed. The output from showtime.cgi has been saved to runn/output.html"
echo "Open this file in your browser to see the result."
