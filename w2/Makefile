all:
	g++ lobby.cpp -lenet -std=c++17 -o lobby && ./lobby
	g++ client.cpp -lenet -std=c++17 -o client -pthread && ./client
	g++ server.cpp -lenet -std=c++17 -o server && ./server