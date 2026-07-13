all:
	g++ -std=c++23 server.cpp -o server -lws2_32 -lstdc++exp -O3 -Wall -Werror
	g++ -std=c++23 client.cpp -o client -lws2_32 -lstdc++exp -O3 -Wall -Werror