#ifndef CSSHORTESTPATH_H_INCLUDED
#define CSSHORTESTPATH_H_INCLUDED

#include <CSChallenge.h>
#include <utilities.h>
#include <sha2.h>
#include <MT19937-64.h>
#include <pthread.h>

#define WALL    'x'
#define FREE    ' '
#define START   's'
#define END     'e'

typedef struct Node Node;

struct Node
{
    int x;
    int y;
    char val;
    Node* path;
    Node* fifo_tail;
};

typedef struct
{
    Node* head;
    Node* tail;
} NodeFIFO;

void* CSShortestPath_solve(void* args);

void setup_grid(Node grid[], int size, int nb_blockers, uint64_t nonce_seed, unsigned int* len,
				Node** start, Node** end, unsigned char* nonce,
				MT1993764_ctx* prng_ctx, sha256_ctx* sha_ctx);
int reverse_path(unsigned char* buffer, Node* node);
void print_grid(Node grid[], int size);
void free_grid(Node** grid, int size);

void update_neighbors(NodeFIFO* fifo, Node grid[], int size);
void pop_fifo(NodeFIFO* fifo);
void update_node(Node* from, Node* to, NodeFIFO* fifo);
void push_fifo(Node* node, NodeFIFO* fifo);

#endif
