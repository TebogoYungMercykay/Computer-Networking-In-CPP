# docker-compose.yml
# version: '3.8'

# services:
#   ftp:
#     image: delfer/alpine-ftp-server
#     ports:
#       - "21:21"
#       - "21000-21010:21000-21010"
#     environment:
#       USERS: "****|*********"
#       ADDRESS: "localhost"
#     volumes:
#       - ./www:/ftp/www
  
#   web:
#     image: httpd:alpine
#     ports:
#       - "8080:80"
#     volumes:
#       - ./www:/usr/local/apache2/htdocs/

# # Settings for FTP file monitor
#	FTP_SERVER=localhost
#	FTP_PORT=21
#	FTP_USER=username
#	FTP_PASSWORD=password
#	LOCAL_FILE=/home/user/index.html
#	REMOTE_FILE=www/index.html
#	CHECK_INTERVAL=10

main:
	g++ -g *.cpp -std=c++17 -pedantic -Wall -Wextra -pthread -lstdc++fs -o main

clean:
	rm -f *.o *.gcov *.gcda *.gcno *.gz *.css readme.env output.txt coverage.txt
	rm -f main
	find . -type f -name "*.html" ! -name "index.html" -delete
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

# # Create the www directory if it doesn't exist
# mkdir -p www

# # Start the services
# docker-compose up -d
