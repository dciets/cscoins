#include <CSSortedList.h>
#include <MT19937-64.h>
#include <sha2.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
    JE DONNES 20$ À CELUI QUI PEUT M'EXPLIQUER POURQUOI QUAND JE SEED MT19937-64 AVEC
    UNE VALEURE ALÉATOIRE (DEV/URANDOM), LE SOLVER ARRIVE PAS À TROUVER UNE SOLUTION,
    MAIS QUAND JE SEED AVEC UNE VALEURE ITÉRATIVE À PARTIR DE 0, LE SOLVER TROUVE
    UNE SOLUTION EN 5 SECONDES.
*/
void* CSSortedList_solve(void* args)
{
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    char prefix[HASH_PREFIX_SIZE] = "";
    unsigned char nonce[NONCE_MAX_SIZE];
    int nb_elements = threadArgs->challenge->nb_elements;
    uint64_t* numbers = malloc(sizeof(uint64_t) * nb_elements);
    unsigned char* final_hash = malloc(nb_elements * 20); // maximum possible final_hash
    unsigned char digest[SHA256_HASH_SIZE];
    int nonce_len = 0;
    uint64_t* seed = malloc(sizeof(uint64_t));
    MT1993764_ctx prng_ctx;
    sha256_ctx sha_ctx;

    // pre-calculate the first hash midstate
    sha256_init(&sha_ctx);
    sha256_update(&sha_ctx, threadArgs->challenge->last_hash, SHA256_HASH_SIZE);

    while(!threadArgs->challenge->skip_challenge &&
          memcmp(prefix, threadArgs->challenge->hash_prefix, HASH_PREFIX_SIZE) != 0)
    {
        sha256_ctx sha2_ctx_copy = sha_ctx;

        nonce_len = uint64_to_ascii(threadArgs->counter++, nonce);
        sha256_update(&sha2_ctx_copy, nonce, nonce_len);
        sha256_final(&sha2_ctx_copy, digest);
        seed = (uint64_t*)&digest[0];
        MT1993764_initialize(&prng_ctx, *seed);

        int i = 0;
        for (i = 0; i < nb_elements; ++i)
            numbers[i] = MT1993764_extractU64(&prng_ctx);
        sort_uint64_array(numbers, nb_elements);

        int index = 0;
        for (i = 0; i < nb_elements; ++i)
            index += uint64_to_ascii(numbers[i], &final_hash[index]);
        sha256(final_hash, index, final_hash);
        bin2hex(final_hash, prefix, HASH_PREFIX_SIZE);
    }

    pthread_mutex_lock(&mutex);
    if (!threadArgs->challenge->skip_challenge && //1)
	    memcmp(prefix, threadArgs->challenge->hash_prefix, HASH_PREFIX_SIZE) == 0)
    {
        threadArgs->challenge->skip_challenge = 1;
	    char lasthash[SHA256_HASH_SIZE+1];
	    memcpy(lasthash, threadArgs->challenge->last_hash, SHA256_HASH_SIZE);
	    char result[SHA256_HASH_SIZE + NONCE_MAX_SIZE + 10];
        memcpy(result, lasthash, SHA256_HASH_SIZE);
        result[SHA256_HASH_SIZE] = ' ';
        memcpy(result+SHA256_HASH_SIZE+1, nonce, nonce_len);
        result[SHA256_HASH_SIZE+1+nonce_len] = '\0';
        mq_send(threadArgs->challenge->clientq, result, strlen(result), 0);
    }
    pthread_mutex_unlock(&mutex);

    free(numbers);
    free(final_hash);
    return NULL;
}

int validate(uint64_t nonce, unsigned char* last_hash, unsigned char* prefix, int nb_elements)
{
    int nonce_len = 0;
    unsigned char nonce_str[NONCE_MAX_SIZE];
    unsigned char digest[SHA256_HASH_SIZE];
    uint64_t* numbers = malloc(sizeof(uint64_t) * nb_elements);
    unsigned char* final_hash = malloc(nb_elements * 20); // maximum possible final_hash
    unsigned char result[HASH_PREFIX_SIZE];
    MT1993764_ctx prng_ctx;
    uint64_t* seed;
    
    sha256_ctx sha_ctx;
    sha256_init(&sha_ctx);
    sha256_update(&sha_ctx, last_hash, SHA256_HASH_SIZE);

    nonce_len = uint64_to_ascii(nonce, nonce_str);
    sha256_update(&sha_ctx, nonce_str, nonce_len);
    sha256_final(&sha_ctx, digest);
    seed = (uint64_t*)&digest[0];
    MT1993764_initialize(&prng_ctx, *seed);

    int i = 0;
    for (i = 0; i < nb_elements; ++i)
        numbers[i] = MT1993764_extractU64(&prng_ctx);
    sort_uint64_array(numbers, nb_elements);

    int index = 0;
    for (i = 0; i < nb_elements; ++i)
        index += uint64_to_ascii(numbers[i], &final_hash[index]);
    sha256(final_hash, index, final_hash);
    bin2hex(final_hash, result, HASH_PREFIX_SIZE);

    return strncmp(prefix, result, HASH_PREFIX_SIZE) == 0;
}



/*
FILE* urandom = fopen("/dev/urandom", "r");
if (!urandom) {
    printf("could not open urandom\n");
    return NULL;
}
size_t bytes_read = fread(seed, 1, sizeof(uint64_t), urandom);
if (bytes_read != sizeof(uint64_t)) {
    printf("bytes_read do not match: %d\n", bytes_read);
    fclose(urandom);
    return NULL;
}
*/
