#include <random>
#include <vector>
#include <openssl/sha.h>
#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <thread>
#include <unistd.h>

#define NB_THREADS 8

struct greater
{
    template<class T>
    bool operator()(T const &a, T const &b) const { return a > b; }
};


uint64_t seed_from_hash(const char *previous_hash, uint64_t nonce) {
  unsigned char hash[32];
  std::stringstream concat;
  concat << std::string(previous_hash);
  concat << nonce;

  auto str = concat.str();
  const char *buf = str.c_str();

  SHA256_CTX sha256;

  SHA256_Init(&sha256);
  SHA256_Update(&sha256, buf, str.size());
  SHA256_Final(hash, &sha256);

  return ((uint64_t*)hash)[0];
}


extern "C" {
  int solve_sorted_list_single(const char * previous_hash, int nb_elements, const unsigned char *prefix, int prefix_len, bool asc, unsigned char *winning_hash, bool *done, uint64_t *found_nonce) {
    std::mt19937_64 prng;
    int nonce = rand() & 134217727;
    uint64_t seed;
    int i;
    int n;
    std::string buf;
    SHA256_CTX sha256;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    std::vector<uint64_t> elements;
    std::string s;

    elements.resize(nb_elements);

    while(!*done) {
      seed = seed_from_hash(previous_hash, nonce);

      prng.seed(seed);

      SHA256_Init(&sha256);

      for(auto it = elements.begin(); it != elements.end(); it++) {
        *it = prng();
      }

      if(asc) {
        std::sort(elements.begin(), elements.end());
      } else {
        std::sort(elements.begin(), elements.end(), greater());
      }


      for(auto it = elements.begin(); it != elements.end(); it++) {
        s = std::to_string(*it).c_str();
        SHA256_Update(&sha256, s.c_str(), s.size());
      }

      SHA256_Final(hash, &sha256);

      if(memcmp(prefix, hash, prefix_len) == 0) {
        if(!*done) {
          *done = true;
          *found_nonce = nonce;
          memcpy(winning_hash, hash, SHA256_DIGEST_LENGTH);
        }

        return nonce;
      }

      nonce = rand() & 134217727;
    }

  }

  int solve_sorted_list(const char * previous_hash, int nb_elements, const unsigned char *prefix, int prefix_len, bool asc, unsigned char *winning_hash) {
    std::thread threads[NB_THREADS];
    bool done = false;
    uint64_t nonce;

    for(int i = 0; i < NB_THREADS; i++) {
      threads[i] = std::thread(solve_sorted_list_single, previous_hash, nb_elements, prefix, prefix_len, asc, winning_hash, &done, &nonce);
    }

    while(!done) {
      usleep(1);
    }

    for(int i = 0; i < NB_THREADS; i++) {
      threads[i].join();
    }

    return nonce;
  }
}
