# TEST LINKS
# LINK 1: http://localhost/index.html
# LINK 2: http://localhost/cgi-bin/showtime.cgi

# RESTART SERVER: sudo systemctl restart apache2
# TROUBLESHOOTING: sudo tail -f /var/log/apache2/error.log

main:
	sudo g++ -Wall -g -std=c++11 -pedantic /usr/lib/cgi-bin/prac/showtime.cpp -o /usr/lib/cgi-bin/showtime.cgi
	sudo g++ -Wall -g -std=c++11 -pedantic /usr/lib/cgi-bin/prac/setsa.cpp -o /usr/lib/cgi-bin/setsa.cgi
	sudo g++ -Wall -g -std=c++11 -pedantic /usr/lib/cgi-bin/prac/setgh.cpp -o /usr/lib/cgi-bin/setgh.cgi

run:
	sudo chmod +x /usr/lib/cgi-bin/showtime.cgi
	sudo chmod +x /usr/lib/cgi-bin/setsa.cgi
	sudo chmod +x /usr/lib/cgi-bin/setgh.cgi

	sudo chown www-data:www-data /usr/lib/cgi-bin/showtime.cgi
	sudo chown www-data:www-data /usr/lib/cgi-bin/setsa.cgi
	sudo chown www-data:www-data /usr/lib/cgi-bin/setgh.cgi

	sudo mkdir -p /var/www/data
	sudo chown www-data:www-data /var/www/data
	sudo chmod 755 /var/www/data

	sudo cp www/index.html /var/www/html/index.html
	sudo cp www/data/timezone.txt /var/www/data/timezone.txt

	sudo chmod 666 /var/www/data/timezone.txt
	sudo chown www-data:www-data /var/www/data/timezone.txt
	sudo chmod 644 /var/www/html/index.html
	sudo chown www-data:www-data /var/www/html/index.html

clean:
	rm -f *.o *.gcov *.gcda *.gcno *.info *.gz *.html *.cgi test *.css output.txt coverage.txt
	reset
	clear

tar:
	tar -cvz main.cpp -f Code.tar.gz

untar:
	tar -zxvf *.tar.gz

untars:
	sudo tar -xzvf cgi_prac.tar.gz -C /usr/lib/cgi-bin/prac/

cov:
	make clean
	g++ -f profile-arcs -f test-coverage -O0 -g -Wall -Wextra -o main main.cpp
	./test
	gcov test-main.cpp -b -o .
