#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root or with sudo"
  exit 1
fi

apt-get update
apt-get install -y apache2 build-essential

a2enmod cgi

systemctl restart apache2

g++ -o showtime.cgi src/showtime.cpp
g++ -o setsa.cgi src/setsa.cpp
g++ -o setgh.cgi src/setgh.cpp

mkdir -p /var/www/data
mkdir -p /usr/lib/cgi-bin

cp showtime.cgi /usr/lib/cgi-bin/
cp setsa.cgi /usr/lib/cgi-bin/
cp setgh.cgi /usr/lib/cgi-bin/

chmod 755 /usr/lib/cgi-bin/showtime.cgi
chmod 755 /usr/lib/cgi-bin/setsa.cgi
chmod 755 /usr/lib/cgi-bin/setgh.cgi

echo "2" > /var/www/data/timezone.txt
echo "South Africa" >> /var/www/data/timezone.txt
echo "Pretoria" >> /var/www/data/timezone.txt

chmod 666 /var/www/data/timezone.txt

cp www/index.html /var/www/html/

IP_ADDRESS=$(hostname -I | awk '{print $1}')

echo "Setup completed successfully."
echo "You can access your application at: http://$IP_ADDRESS/"
echo "Or directly access the CGI script at: http://$IP_ADDRESS/cgi-bin/showtime.cgi"
