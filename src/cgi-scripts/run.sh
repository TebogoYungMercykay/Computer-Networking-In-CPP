#!/bin/bash

mkdir -p runn
mkdir -p runn/data

echo "2" > runn/data/timezone.txt
echo "South Africa" >> runn/data/timezone.txt
echo "Pretoria" >> runn/data/timezone.txt

g++ -o runn_showtime src/showtime.cpp
g++ -o runn_setsa src/setsa.cpp
g++ -o runn_setgh src/setgh.cpp

sed -i 's|/var/www/data/timezone.txt|runn/data/timezone.txt|g' runn_showtime
sed -i 's|/var/www/data/timezone.txt|runn/data/timezone.txt|g' runn_setsa
sed -i 's|/var/www/data/timezone.txt|runn/data/timezone.txt|g' runn_setgh

./runn_showtime > runn/output.html

echo "runn completed. The output from showtime.cgi has been saved to runn/output.html"
echo "Open this file in your browser to see the result."
