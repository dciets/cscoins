#include <CSChallenge.h>
#include <CSSortedList.h>
#include <CSReverseList.h>
#include <CSShortestPath.h>
#include <pthread.h>
#include <string.h>

void solve_challenge(CSChallenge* challenge)
{
    void* (*solve_func)(void*);
    pthread_t threads[NB_THREADS];
    int i = 0;


    printf("%d\n", challenge->grid_size);
    printf("%d\n", challenge->nb_blockers);
    printf("%.*s\n", 64, challenge->last_hash);
    printf("%.*s\n", 4, challenge->hash_prefix);

    if (strcmp(challenge->name, SORTED_LIST) == 0)
        solve_func = &CSSortedList_solve;
    else if (strcmp(challenge->name, REVERSE_LIST) == 0)
        solve_func = &CSReverseList_solve;
    else if (strcmp(challenge->name, SHORTEST_PATH) == 0)
        solve_func = &CSShortestPath_solve;

    // start threads
    for (i = 0; i < NB_THREADS; i++)
    {
        ThreadArgs args;
        args.counter = NONCE_MAX / NB_THREADS * i;
        args.challenge = challenge;
        pthread_create(&threads[i], NULL, solve_func, &args);
    }

    // clean up
    for (i = 0; i < NB_THREADS; i++)
        pthread_join(threads[i], NULL);
}

void parse_challenge_data(CSChallenge* challenge)
{
    int i = 0;
    char* pch;
    pch = strtok (challenge->server_msg, " ");
    while (pch != NULL)
    {
        if (i == 0) {
            challenge->id = atoi(pch);
        } else if (i == 1) {
            strcpy(challenge->name, pch);
        } else if (i == 2) {
            strcpy(challenge->last_hash, pch);
        } else if (i == 3) {
            strcpy(challenge->hash_prefix, pch);
        } else if (i == 4 && strcmp(challenge->name, SORTED_LIST) == 0) {
            challenge->nb_elements = atoi(pch);
        } else if (i == 4 && strcmp(challenge->name, REVERSE_LIST) == 0) {
            challenge->nb_elements = atoi(pch);
        } else if (i == 4 && strcmp(challenge->name, SHORTEST_PATH) == 0) {
            challenge->grid_size = atoi(pch);
        } else if (i == 5 && strcmp(challenge->name, SHORTEST_PATH) == 0) {
            challenge->nb_blockers = atoi(pch);
        } 
            
        i++;
        pch = strtok (NULL, " ");
    }
}
