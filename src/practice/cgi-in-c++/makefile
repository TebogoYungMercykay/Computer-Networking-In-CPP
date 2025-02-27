# VIEW HERE: http://localhost/cgi-bin/main.cgi
# RESTART SERVER: sudo systemctl restart apache2
# TROUBLESHOOTING: sudo tail -f /var/log/apache2/error.log

main:
	g++ -g main.cpp -std=c++11 -pedantic -o main.cgi
	chmod +x main.cgi

clean:
	rm -f *.o *.gcov *.gcda *.gcno *.info *.gz *.html *.cgi test *.css output.txt coverage.txt
	reset
	clear

run:
	sudo mv main.cgi /usr/lib/cgi-bin/
	sudo chmod +x /usr/lib/cgi-bin/main.cgi
	sudo chown www-data:www-data /usr/lib/cgi-bin/main.cgi

tar:
	tar -cvz main.cpp -f Code.tar.gz

untar:
	tar -zxvf *.tar.gz

cov:
	make clean
	g++ -f profile-arcs -f test-coverage -O0 -g -Wall -Wextra -o main main.cpp
	./test
	gcov test-main.cpp -b -o .
