# VIEW HERE: http://localhost/cgi-bin/hello.cgi
# RESTART SERVER: sudo systemctl restart apache2
# TROUBLESHOOTING: sudo tail -f /var/log/apache2/error.log

main:
	g++ -g hello.cpp -std=c++11 -pedantic -o hello.cgi
	chmod +x hello.cgi

clean:
	rm -f *.o *.gcov *.gcda *.gcno *.info *.gz *.html *.cgi test *.css output.txt coverage.txt
	reset
	clear

run:
	sudo mv hello.cgi /usr/lib/cgi-bin/
	sudo chmod +x /usr/lib/cgi-bin/hello.cgi
	sudo chown www-data:www-data /usr/lib/cgi-bin/hello.cgi

tar:
	tar -cvz hello.cpp -f Code.tar.gz

untar:
	tar -zxvf *.tar.gz

cov:
	make clean
	g++ -f profile-arcs -f test-coverage -O0 -g -Wall -Wextra -o hello hello.cpp
	./test
	gcov test-hello.cpp -b -o .
