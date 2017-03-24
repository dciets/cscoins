CC = g++
LIB_FILES = solve_utils.cpp
ARGS = -O3 -L/lib/x86_64-linux-gnu/ -I/usr/include/openssl --std=c++17 $(FILES) -lssl -lcrypto -lpthread -Wall

all: sorted_list shortest_path

sorted_list: solve_sorted_list.cpp
	$(CC) solve_sorted_list.cpp $(LIB_FILES) -o sorted_list.solver $(ARGS)
	chmod +x sorted_list.solver

shortest_path: solve_shortest_path.cpp
	$(CC) solve_shortest_path.cpp $(LIB_FILES) -o shortest_path.solver $(ARGS)
	chmod +x shortest_path.solver
