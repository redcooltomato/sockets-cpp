all:
	g++ server.cpp -o server -lws2_32
	g++ client.cpp -o client -lws2_32