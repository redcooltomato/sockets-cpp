all:
	g++ -std=c++23 server.cpp -o server -lstdc++exp -O3 -Wall -Werror -Wunused
	g++ -std=c++23 client.cpp -o client -lstdc++exp -O3 -Wall -Werror -Wunused
dev:
	g++ -std=c++23 server.cpp -o server -lstdc++exp -O3 -Wall -Werror -Wunused -DDEV
	g++ -std=c++23 client.cpp -o client -lstdc++exp -O3 -Wall -Werror -Wunused -DDEV
win:
	g++ -std=c++23 server.cpp -o server -lws2_32 -lstdc++exp -O3 -Wall -Werror -Wunused -DWIN
	g++ -std=c++23 client.cpp -o client -lws2_32 -lstdc++exp -O3 -Wall -Werror -Wunused -DWIN
windev:
	g++ -std=c++23 server.cpp -o server -lws2_32 -lstdc++exp -O3 -Wall -Werror -Wunused -DDEV -DWIN
	g++ -std=c++23 client.cpp -o client -lws2_32 -lstdc++exp -O3 -Wall -Werror -Wunused -DDEV -DWIN