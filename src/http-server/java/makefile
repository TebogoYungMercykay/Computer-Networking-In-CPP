default:
	javac -cp .:json-20231013.jar *.java

run:
	java -cp .:json-20231013.jar PhoneDirectoryServer

clean:
	rm -f *.class
	reset
	clear

tar:
	tar -cvz *.java -f Code.tar.gz

untar:
	tar -zxvf *.tar.gz
