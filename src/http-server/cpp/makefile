# HTTP Phone Book Server in C++11
# HTTP localhost 8080
# phone_directory.json
# MD5: e961d337d9a39e5d5cdad7ed1fb147a5

main:
	g++ -g *.cpp -std=c++17 -pedantic -lssl -lcrypto -o main

clean:
	rm -f *.o *.gcov *.gcda *.gcno *.gz *.html main *.css output.txt coverage.txt
	reset
	clear

run:
	valgrind --leak-check=full --show-leak-kinds=all ./main

tar:
	tar -cvz *.* makefile -f http.tar.gz

untar:
	tar -zxvf *.tar.gz

cov:
	make clean
	g++ -f profile-arcs -f test-coverage -O0 -g -Wall -Wextra -o main main.cpp
	./test
	gcov test-main.cpp -b -o .
