FILES = solve_sorted_list.cpp

solve.so: $(FILES)
	g++ -O3 -L/lib/x86_64-linux-gnu/ -I/usr/include/openssl -shared -fPIC --std=c++17 $(FILES) -o solve.so -lssl -lcrypto
