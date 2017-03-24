#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include <openssl/sha.h>

#include "solve_utils.hpp"

struct Position {
  uint32_t x, y;

  Position() : x(0), y(0) {
  }

  Position(uint32_t x, uint32_t y) : x(x), y(y) {
  }

  Position(Position const& p) : x(p.x), y(p.y) {
  }

  Position& operator=(Position const& p) {
    x = p.x;
    y = p.y;
    return *this;
  }

  uint32_t distance(Position const& b) const {
    return std::abs(b.x - x) + std::abs(b.y - y);
  }

  bool has_left() const {
    return x > 0;
  }

  bool has_right(uint32_t grid_size) const {
    return x + 1 < grid_size;
  }

  bool has_up() const {
    return y > 0;
  }

  bool has_down(uint32_t grid_size) const {
    return y + 1 < grid_size;
  }

  Position left() const {
    return Position(x - 1, y);
  }

  Position right() const {
    return Position(x + 1, y);
  }

  Position up() const {
    return Position(x, y - 1);
  }

  Position down() const {
    return Position(x, y + 1);
  }
};

bool operator==(Position const& a, Position const& b) {
  return a.x == b.x && a.y == b.y;
}

bool operator!=(Position const& a, Position const& b) {
  return !(a == b);
}

bool operator<(Position const& a, Position const& b) {
  if(a.x != b.x) {
    return a.x < b.x;
  } else {
    return a.y < b.y;
  }
}

std::ostream& operator<<(std::ostream& out, Position const& p) {
  out << "(" << p.x << ", " << p.y << ")";
  return out;
}

typedef std::vector<std::vector<char>> Grid;
typedef std::list<Position> Queue;
typedef std::map<Position, uint32_t> ScoreMap;
typedef std::map<Position, Position> ParentMap;

constexpr char WALL = 'x';
constexpr char START = 's';
constexpr char END = 'e';
constexpr char PATH = 'p';

uint32_t num_digits(uint32_t n) {
  if(n < 10) return 1;
  if(n < 100) return 2;
  if(n < 1000) return 3;
  if(n < 10000) return 4;
  if(n < 100000) return 5;
  if(n < 1000000) return 6;
  if(n < 10000000) return 7;
  if(n < 100000000) return 8;
  if(n < 1000000000) return 9;
  return 10;
}

void build_path(ParentMap const& came_from, Position const& current, char* str, size_t& last_write) {
  //snprintf will print a null byte over str[last_write], so save it and put it back in later
  char end_char = str[last_write];
  size_t no_chars = num_digits(current.y) + num_digits(current.x);
  size_t str_start = last_write - no_chars;
  snprintf(str + str_start, no_chars + 1, "%d%d", current.y, current.x);
  str[last_write] = end_char;
  last_write = str_start;

  auto it = came_from.find(current);
  if(it != came_from.end()) {
    build_path(came_from, it->second, str, last_write);
  }
}

void handle_next(Grid& grid, Position const& current, Position const& next, ScoreMap& cost_map, Queue& queue, ParentMap& came_from) {
  if(grid[next.y][next.x] == WALL) {
    return;
  }

  uint32_t next_cost = cost_map[current] + 1;

  auto it = cost_map.find(next);
  if(it == cost_map.end()) {
    cost_map[next] = next_cost;
  } else if(next_cost < it->second) {
    it->second = next_cost;
  } else {
    return;
  }

  if(std::find(queue.begin(), queue.end(), next) == queue.end()) {
    queue.push_back(next);
  }

  came_from[next] = current;
}

void dijkstra_search(Grid& grid, uint32_t grid_size, Position const& start, Position const& end, char* str, size_t& last_write) {
  Queue queue;
  ScoreMap cost_map;
  ParentMap came_from;

  queue.push_back(start);
  cost_map[start] = 0;

  while(!queue.empty()) {
    Position current = queue.front();
    queue.pop_front();

    if(current == end) {
      build_path(came_from, current, str, last_write);
      break;
    }

     if(current.has_up()) {
      handle_next(grid, current, current.up(), cost_map, queue, came_from);
    }

    if(current.has_left()) {
      handle_next(grid, current, current.left(), cost_map, queue, came_from);
    }

    if(current.has_right(grid_size)) {
      handle_next(grid, current, current.right(), cost_map, queue, came_from);
    }

    if(current.has_down(grid_size)) {
      handle_next(grid, current, current.down(), cost_map, queue, came_from);
    }

    queue.sort([&cost_map](Position const& a, Position const& b) {
      return cost_map.at(a) < cost_map.at(b);
    });
  }
}

void generate_coordinate(std::mt19937_64& prng, uint32_t grid_size, uint32_t& x, uint32_t& y) {
  //The server implementation generates the y coordinate first, then the x coordinate.
  y = prng() % grid_size;
  x = prng() % grid_size;
}

void print_grid(Grid const& grid, uint32_t grid_size) {
  for(uint32_t y = 0 ; y < grid_size ; y++) {
    for(uint32_t x = 0 ; x < grid_size ; x++) {
      std::cout << grid[y][x];
    }

    std::cout << "\n";
  }

  std::cout << std::flush;
}

//Thread synchronization variables
int winning_nonce = -1;
int winning_thread = -1;
std::condition_variable condition;
std::mutex mut;

void solve_shortest_path_single(const char* previous_hash, const unsigned char* prefix, size_t prefix_len, uint32_t grid_size, uint32_t nb_blockers, unsigned char hash[32], int thread_num) {
  srand(time(nullptr));

  std::mt19937_64 prng;
  char hash_buffer[84];
  strncpy(hash_buffer, previous_hash, 64);

  //A buffer size of 10 megabytes ought to be enough to contain our result string...
  constexpr size_t buffer_size = 10 * 1024 * 1024;
  char* buffer = new char[buffer_size];
  buffer[buffer_size - 1] = 0;

  //Make sure it gets deleted ;)
  std::unique_ptr<char> buffer_ptr(buffer);

  SHA256_CTX sha256;

  bool solution_found = false;
  bool need_exit = false;
  int nonce;

  do {
    nonce = 78043384;
    int nonce_length;
    snprintf(hash_buffer + 64, 20, "%d%n", nonce, &nonce_length);

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, hash_buffer, 64 + nonce_length);
    SHA256_Final(hash, &sha256);

    uint64_t seed = ((uint64_t*)hash)[0];
    prng.seed(seed);

    //[y][x]
    Grid grid(grid_size, std::vector<char>(grid_size, ' '));

    for(uint32_t i = 1 ; i < grid_size - 1 ; i++) {
      grid[0][i] = WALL;
      grid[grid_size - 1][i] = WALL;
      grid[i][0] = WALL;
      grid[i][grid_size - 1] = WALL;
    }

    grid[0][0] = WALL;
    grid[0][grid_size - 1] = WALL;
    grid[grid_size - 1][0] = WALL;
    grid[grid_size - 1][grid_size - 1] = WALL;

    uint32_t x, y;
    const char tokens[] = {START, END};
    Position positions[2];

    for(int i = 0 ; i < 2 ; i++) {
      do {
        generate_coordinate(prng, grid_size, x, y);
      } while(grid[y][x] != ' ');

      grid[y][x] = tokens[i];
      positions[i].x = x;
      positions[i].y = y;
    }

    for(uint32_t i = 0 ; i < nb_blockers ; i++) {
      generate_coordinate(prng, grid_size, x, y);

      if(grid[y][x] == ' ') {
        grid[y][x] = WALL;
      }
    }

    size_t last_write = buffer_size - 1;
    dijkstra_search(grid, grid_size, positions[0], positions[1], buffer, last_write);
    size_t solution_length = buffer_size - 1 - last_write;

    if(solution_length > 0) {
      SHA256_Init(&sha256);
      SHA256_Update(&sha256, buffer + last_write, solution_length);
      SHA256_Final(hash, &sha256);

      if(memcmp(hash, prefix, prefix_len) == 0) {
        solution_found = true;
      }
    }

    {
      std::lock_guard<std::mutex> lk(mut);
      need_exit = winning_thread != -1;
    }

  } while(!solution_found && !need_exit);

  if(solution_found) {
    {
      std::lock_guard<std::mutex> lk(mut);
      winning_nonce = nonce;
      winning_thread = thread_num;
    }

    condition.notify_all();
  }
}

extern "C" {
  int solve_shortest_path(const char* previous_hash, const unsigned char* prefix, size_t prefix_len, uint32_t grid_size, uint32_t nb_blockers, unsigned char* winning_hash) {
    constexpr size_t NO_THREADS = 8;
    std::thread threads[NO_THREADS];
    unsigned char hashes[NO_THREADS][32];

    for(size_t i = 0 ; i < NO_THREADS ; i++) {
      threads[i] = std::thread(solve_shortest_path_single, previous_hash, prefix, prefix_len, grid_size, nb_blockers, hashes[i], i);
    }

    {
      std::unique_lock<std::mutex> lk(mut);
      condition.wait(lk, []{return winning_thread != -1;});
    }

    for(size_t i = 0 ; i < NO_THREADS ; i++) {
      threads[i].join();
    }

    memcpy(winning_hash, hashes[winning_thread], 32);
    return winning_nonce;
  }
}

int main(int argc, char* argv[]) {
  uint32_t grid_size, nb_blockers;
  const char* previous_hash;
  const char* prefix;

  if(argc != 5) {
    return 1;
  }

  previous_hash = argv[1];
  prefix = argv[2];
  grid_size = std::atoi(argv[3]);
  nb_blockers = std::atoi(argv[4]);

  size_t prefix_len = strlen(prefix) / 2;
  unsigned char prefix_bytes[prefix_len];
  hex_to_bytes(prefix, prefix_bytes);

  unsigned char winning_hash[32];
  int nonce = solve_shortest_path(previous_hash, prefix_bytes, prefix_len, grid_size, nb_blockers, winning_hash);
  print_bytes_hex(winning_hash, 32);
  std::cout << "\n" << nonce << std::endl;

  return 0;
}
