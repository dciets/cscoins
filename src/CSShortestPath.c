#include <CSShortestPath.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* CSShortestPath_solve(void* args)
{
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    unsigned char nonce[NONCE_MAX_SIZE];
    unsigned char path_str[256] = "";
    unsigned char prefix[HASH_PREFIX_SIZE];
    MT1993764_ctx prng_ctx;
    sha256_ctx sha_ctx;
    NodeFIFO fifo;
    Node* grid  = NULL;
    int size = threadArgs->challenge->grid_size;
    int nb_blockers = threadArgs->challenge->nb_blockers;
    unsigned int nlen = 0;

    // set up grid
    grid = malloc(sizeof(Node) * size * size);

    // pre-calculate the first hash midstate
    sha256_init(&sha_ctx);
    sha256_update(&sha_ctx, threadArgs->challenge->last_hash, SHA256_HASH_SIZE);

    while(!threadArgs->challenge->skip_challenge &&
          memcmp(prefix, threadArgs->challenge->hash_prefix, HASH_PREFIX_SIZE) != 0)
    {
        nlen = 0;
        Node* start = NULL;
        Node* end   = NULL;
        memset(grid, 0, sizeof(Node) * size * size);
        memset(&fifo, 0, sizeof(fifo));

        sha256_ctx sha2_ctx_copy = sha_ctx;
        setup_grid(grid, size, nb_blockers, threadArgs->counter++, &nlen,
			       &start, &end, nonce, &prng_ctx, &sha2_ctx_copy);
		nonce[nlen] = '\n';

        push_fifo(end, &fifo);
        while(start->path == NULL && fifo.head != NULL) {
            update_neighbors(&fifo, grid, size);
        }
        
        int nBytes = reverse_path(path_str, start);
        
        if (nBytes > 0) {
            sha256(path_str, nBytes, path_str);
            bin2hex(path_str, prefix, HASH_PREFIX_SIZE);
        }
	}
    
    pthread_mutex_lock(&mutex);
    if (!threadArgs->challenge->skip_challenge && //1)
	    memcmp(prefix, threadArgs->challenge->hash_prefix, HASH_PREFIX_SIZE) == 0)
    {
		printf("solved: %.*s %lld", 64, threadArgs->challenge->last_hash, threadArgs->counter-1);
	    threadArgs->challenge->skip_challenge = 1;
	    char result[SHA256_HASH_SIZE + NONCE_MAX_SIZE + 10];
		sprintf(result, "%.*s %lld", 64, threadArgs->challenge->last_hash, threadArgs->counter-1);
		mq_send(threadArgs->challenge->clientq, result, strlen(result), 0);
/*
	    threadArgs->challenge->skip_challenge = 1;
	    char lasthash[SHA256_HASH_SIZE+1];
	    memcpy(lasthash, threadArgs->challenge->last_hash, SHA256_HASH_SIZE);
        memcpy(result, lasthash, SHA256_HASH_SIZE);
        result[SHA256_HASH_SIZE] = ' ';
        memcpy(result+SHA256_HASH_SIZE+1, nonce, strlen(nonce));
        result[SHA256_HASH_SIZE+1+strlen(nonce)] = '\0';
        mq_send(threadArgs->challenge->clientq, result, strlen(result), 0);
*/
/*
        threadArgs->challenge->skip_challenge = 1;
	    char lasthash[SHA256_HASH_SIZE+1];
	    memcpy(lasthash, threadArgs->challenge->last_hash, SHA256_HASH_SIZE);
	    char result[SHA256_HASH_SIZE + NONCE_MAX_SIZE + 10];
        memcpy(result, lasthash, SHA256_HASH_SIZE);
        result[SHA256_HASH_SIZE] = ' ';
        memcpy(result+SHA256_HASH_SIZE+1, nonce, strlen(nonce));
        result[SHA256_HASH_SIZE+1+strlen(nonce)] = '\0';
        mq_send(threadArgs->challenge->clientq, result, strlen(result), 0);
        printf("solved: %lld\n", threadArgs->counter-1);
*/
    }
    pthread_mutex_unlock(&mutex);

    free(grid);
    return NULL;
}

void setup_grid(Node grid[], int size, int nb_blockers, uint64_t nonce_seed,
                unsigned int* nlen, Node** start, Node** end, unsigned char* nonce,
				MT1993764_ctx* prng_ctx, sha256_ctx* sha_ctx)
{
    unsigned char digest[SHA256_HASH_SIZE];
    uint64_t* seed;
    int posX, posY;
    
    // initial set up
    int i = 0, j = 0;
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            if (i == 0 || i == size-1 || j == 0 || j == size-1) {
                grid[i * size + j].val = WALL;
            } else {
                grid[i * size + j].val = FREE;
                grid[i * size + j].y = i;
                grid[i * size + j].x = j;
            }
        }
    }

    // ***************/ set up all the elements ***************/
    *nlen = uint64_to_ascii(nonce_seed, nonce);
    sha256_update(sha_ctx, nonce, *nlen);
    sha256_final(sha_ctx, digest);
    seed = (uint64_t*)&digest[0];
    MT1993764_initialize(prng_ctx, *seed);

    posY = MT1993764_extractU64(prng_ctx) % size;
    posX = MT1993764_extractU64(prng_ctx) % size;
    while (grid[posY * size + posX].val != FREE) {
        posY = MT1993764_extractU64(prng_ctx) % size;
        posX = MT1993764_extractU64(prng_ctx) % size;
    }
    grid[posY * size + posX].val = START;
    *start = &(grid[posY * size + posX]);

    posY = MT1993764_extractU64(prng_ctx) % size;
    posX = MT1993764_extractU64(prng_ctx) % size;
    while (grid[posY * size + posX].val != FREE) {
        posY = MT1993764_extractU64(prng_ctx) % size;
        posX = MT1993764_extractU64(prng_ctx) % size;
    }
    grid[posY * size + posX].val = END;
    *end = &(grid[posY * size + posX]);

    for (i = 0; i < nb_blockers; i++) {
        posY = MT1993764_extractU64(prng_ctx) % size;
        posX = MT1993764_extractU64(prng_ctx) % size;
        if (grid[posY * size + posX].val == FREE) {
            grid[posY * size + posX].val = WALL;
        }
    }
}

int reverse_path(unsigned char* buffer, Node* node)
{
    int index = 0;
    while(node != NULL) {

        // DEBUG ONLY (REMOVE THIS ON PRODUCTION
        if (index != 0 && node->path != NULL)
            node->val = 'p';
        // =====================================

        index += uint64_to_ascii(node->y, &buffer[index]);
        index += uint64_to_ascii(node->x, &buffer[index]);

        node = node->path;
    }
    return index;
}

void print_grid(Node* grid, int size)
{
    int i = 0, j = 0;
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            printf("%c ", grid[i * size + j].val);
        }
        printf("\n");
    }
}

/* FIFO FUNCTIONS */
void push_fifo(Node* node, NodeFIFO* fifo)
{
    if (node->fifo_tail != NULL)
    {
        printf("BIG MISTAKE!\n");
    }

    if (fifo->head == NULL)
        fifo->head = node;

    if (fifo->tail != NULL)
        fifo->tail->fifo_tail = node;
    fifo->tail = node;
}

void update_node(Node* from, Node* to, NodeFIFO* fifo)
{
    if (to->val != WALL && to->val != END && to->path == NULL) {
        to->path = from;
        push_fifo(to, fifo);
    }
}

void pop_fifo(NodeFIFO* fifo)
{
    Node* tmp = fifo->head;
    fifo->head = fifo->head->fifo_tail;
    tmp->fifo_tail = NULL;
}

void update_neighbors(NodeFIFO* fifo, Node grid[], int size)
{
    int x = fifo->head->x;
    int y = fifo->head->y;

    update_node(fifo->head, &(grid[(y-1) * size + x]), fifo);
    update_node(fifo->head, &(grid[y * size + x - 1]), fifo);
    update_node(fifo->head, &(grid[y * size + x + 1]), fifo);
    update_node(fifo->head, &(grid[(y+1) * size + x]), fifo);

    pop_fifo(fifo);
}
