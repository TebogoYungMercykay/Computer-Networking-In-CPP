# Telnet Phone Book Server in C++11
# telnet localhost 8080
# phonebook.txt
# MD5: e961d337d9a39e5d5cdad7ed1fb147a5

main:
	g++ -g *.cpp -std=c++11 -pedantic -o main

clean:
	rm -f *.o *.gcov *.gcda *.gcno *.gz *.html main *.css output.txt coverage.txt
	reset
	clear

run:
	valgrind --leak-check=full ./main

tar:
	tar -cvz *.* makefile -f telnet.tar.gz

untar:
	tar -zxvf *.tar.gz

cov:
	make clean
	g++ -f profile-arcs -f test-coverage -O0 -g -Wall -Wextra -o main main.cpp
	./test
	gcov test-main.cpp -b -o .
