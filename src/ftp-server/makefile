# .env File
# 	HOST=
# 	PORT=
# 	SENDER_EMAIL=
# 	EMAIL_PASSWORD=
# 	RECEIVER_EMAIL=
# 	USE_SSL=

main:
	g++ -g *.cpp -std=c++17 -pedantic -Wall -Wextra -I/usr/include/openssl -L/usr/lib -lssl -lcrypto -o main

clean:
	rm -f *.o *.gcov *.gcda *.gcno *.gz *.html main *.css output.txt coverage.txt
	reset
	clear

run:
	valgrind --leak-check=full --show-leak-kinds=all ./main

tar:
	tar -cvz *.* makefile -f ftp.tar.gz

untar:
	tar -zxvf *.tar.gz

cov:
	make clean
	g++ -f profile-arcs -f test-coverage -O0 -g -Wall -Wextra -o main main.cpp
	./test
	gcov test-main.cpp -b -o .
