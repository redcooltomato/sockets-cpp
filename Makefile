all:
	g++ -std=c++23 server.cpp -o server -lws2_32
	g++ -std=c++23 client.cpp -o client -lws2_32 -lstdc++exp -O2 