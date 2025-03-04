#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root or with sudo"
  exit 1
fi

# Install dependencies
apt-get update
apt-get install -y apache2 build-essential

a2enmod cgi
systemctl restart apache2

# Create necessary directories
mkdir -p /var/www/data
mkdir -p /usr/lib/cgi-bin/prac

# Compile C++ CGI scripts
sudo g++ -Wall -g -std=c++11 -pedantic /usr/lib/cgi-bin/prac/showtime.cpp -o /usr/lib/cgi-bin/showtime.cgi
sudo g++ -Wall -g -std=c++11 -pedantic /usr/lib/cgi-bin/prac/setsa.cpp -o /usr/lib/cgi-bin/setsa.cgi
sudo g++ -Wall -g -std=c++11 -pedantic /usr/lib/cgi-bin/prac/setgh.cpp -o /usr/lib/cgi-bin/setgh.cgi

# Set permissions
sudo chmod +x /usr/lib/cgi-bin/showtime.cgi
sudo chmod +x /usr/lib/cgi-bin/setsa.cgi
sudo chmod +x /usr/lib/cgi-bin/setgh.cgi

sudo chown www-data:www-data /usr/lib/cgi-bin/showtime.cgi
sudo chown www-data:www-data /usr/lib/cgi-bin/setsa.cgi
sudo chown www-data:www-data /usr/lib/cgi-bin/setgh.cgi

# Set up timezone data
sudo echo "2" > /var/www/data/timezone.txt
sudo echo "South Africa" >> /var/www/data/timezone.txt
sudo echo "Pretoria" >> /var/www/data/timezone.txt

sudo chmod 666 /var/www/data/timezone.txt
sudo chown www-data:www-data /var/www/data/timezone.txt

# Copy index.html to web root
sudo cp www/index.html /var/www/html/index.html
sudo chmod 644 /var/www/html/index.html
sudo chown www-data:www-data /var/www/html/index.html

IP_ADDRESS=$(hostname -I | awk '{print $1}')

echo "Setup completed successfully."
echo "You can access your application at: http://$IP_ADDRESS/"
echo "Or directly access the CGI script at: http://$IP_ADDRESS/cgi-bin/showtime.cgi"
