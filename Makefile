build: server subscriber

server: server.o
	g++ server.o -o server

subscriber: subscriber.o
	g++ subscriber.o -o subscriber

subscriber.o:
	g++ -c subscriber.cpp

server.o:
	g++ -c server.cpp