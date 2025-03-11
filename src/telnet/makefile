# Telnet Phone Book Server in C++11
# telnet localhost 8080
# phonebook.txt

main:
	g++ -g *.cpp -std=c++11 -pedantic -o main

clean:
	rm -f *.o *.gcov *.gcda *.gcno *.gz *.html main *.css output.txt coverage.txt
	reset
	clear

run:
	valgrind --leak-check=full ./main

tar:
	tar -cvz *.* -f Code.tar.gz

untar:
	tar -zxvf *.tar.gz

cov:
	make clean
	g++ -f profile-arcs -f test-coverage -O0 -g -Wall -Wextra -o main main.cpp
	./test
	gcov test-main.cpp -b -o .
