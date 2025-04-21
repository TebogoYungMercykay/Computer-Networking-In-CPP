# .env File
# 	HOST=smtp.example.com
# 	PORT=587
# 	SENDER_EMAIL=sender-email@example.com
# 	EMAIL_PASSWORD=sender-password
# 	RECEIVER_EMAIL=rcpt@example.com
# 	USE_SSL=true

main:
	g++ -g *.cpp -std=c++17 -pedantic -lssl -lcrypto -o main

clean:
	rm -f *.o *.gcov *.gcda *.gcno *.gz *.html main *.css output.txt coverage.txt
	reset
	clear

run:
	valgrind --leak-check=full --show-leak-kinds=all ./main

tar:
	tar -cvz *.* makefile -f ldap.tar.gz

untar:
	tar -zxvf *.tar.gz

cov:
	make clean
	g++ -f profile-arcs -f test-coverage -O0 -g -Wall -Wextra -o main main.cpp
	./test
	gcov test-main.cpp -b -o .
