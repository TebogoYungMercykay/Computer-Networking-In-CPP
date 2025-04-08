# sudo dpkg-reconfigure slapd
#	 Omit OpenLDAP server configuration? Select "No"
#	 DNS domain name: Use something like "friends.local"
#	 Organization name: "Friends Directory"
#	 Admin password: Create a secure password
#	 Database backend: MDB (default)
#	 Remove database when slapd is purged? "No"
#	 Move old database? "Yes"
# See LDIF Below the Makefile Commands to Add Sample Friends
# ldapadd -x -D "cn=admin,dc=friends,dc=local" -W -f friends.ldif

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
 
# (LDIF) File For Adding Sample Friends
## Base structure
# 	dn: ou=Friends,dc=friends,dc=local
# 	objectClass: organizationalUnit
# 	ou: Friends

## Add some sample friends
# 	dn: cn=John Smith,ou=Friends,dc=friends,dc=local
# 	objectClass: inetOrgPerson
# 	cn: John Smith
# 	sn: Smith
# 	givenName: John
# 	telephoneNumber: +27-66-123-4567

# 	dn: cn=Jane Doe,ou=Friends,dc=friends,dc=local
# 	objectClass: inetOrgPerson
# 	cn: Jane Doe
# 	sn: Doe
# 	givenName: Jane
# 	telephoneNumber: +27-66-987-6543

# 	dn: cn=Alice Johnson,ou=Friends,dc=friends,dc=local
# 	objectClass: inetOrgPerson
# 	cn: Alice Johnson
# 	sn: Johnson
# 	givenName: Alice
# 	telephoneNumber: +27-66-456-7890

# 	dn: cn=Bob Williams,ou=Friends,dc=friends,dc=local
# 	objectClass: inetOrgPerson
# 	cn: Bob Williams
# 	sn: Williams
# 	givenName: Bob
# 	telephoneNumber: +27-66-789-0123
