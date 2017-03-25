#ifndef CSCHALLENGE_H_INCLUDED
#define CSCHALLENGE_H_INCLUDED

#include <config.h>
#include <mqueue.h>
#include <stdint.h>

#define CHALLENGE_NAME_SIZE     20
#define SHA256_HASH_SIZE        64          // 256 bits in hex => 64 characters + 1 for the null terminator
#define HASH_PREFIX_SIZE        4
#define NONCE_MAX_SIZE          10          // maximum number of bytes the nonce can have
#define NONCE_MAX               0x5F5E0FF   // maximum value the nonce can have

#define SORTED_LIST     "sorted_list"
#define REVERSE_LIST    "reverse_sorted_list"
#define SHORTEST_PATH   "shortest_path"

typedef struct
{
	// challenge data
	int id;
	char name[CHALLENGE_NAME_SIZE];
	char last_hash[SHA256_HASH_SIZE];
	char hash_prefix[HASH_PREFIX_SIZE];
	int nb_elements;
    int grid_size;
    int nb_blockers;

	// communication with server
	char server_msg[SERVER_MSG_SIZE];
	int skip_challenge;
    mqd_t serverq;
    mqd_t clientq;
    struct sigevent* notif_event;
} CSChallenge;

typedef struct
{
    uint64_t counter;
    CSChallenge* challenge;
} ThreadArgs;

void solve_challenge(CSChallenge* challenge);
void parse_challenge_data(CSChallenge* challenge);

#endif // CSCHALLENGE_H_INCLUDED
