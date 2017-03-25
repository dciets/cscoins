#include <pthread.h>
#include <mqueue.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <CSChallenge.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void ipc_callback(union sigval challenge_ptr);
int read_server_msg(char* buffer, mqd_t serverq);
void setup_challenge(CSChallenge* challenge);
void setup_ipc(CSChallenge* challenge, struct sigevent* notif);

#include <CSShortestPath.h>
int main(int argc, char** argv)
{
    
    // temp data
    //unsigned char last_hash[SHA256_HASH_SIZE] = "79c61671b2453153282ed4db092d87716582b9acf5752b5a08bcf2b9d961d234";
    //unsigned char hash_prefix[HASH_PREFIX_SIZE] = "02d7";
    //int nb_blockers = 80;
    //int size = 25;
    //int64_t counter = 7144700827;
    //CSChallenge challenge;

    /*
    // easy
    setup_challenge(&challenge);
    challenge.skip_challenge = 0;
    strcpy(challenge.server_msg, "27 shortest_path df9a9e6d2c54b16c02366640ff7467316b639c571ba76285f2c80b2217d42236 dd75 25 80");
    parse_challenge_data(&challenge);
    ThreadArgs args;
    args.counter = 13323;
    args.challenge = &challenge;
    CSShortestPath_solve(&args);
    /*

    /*
    // unknown
    setup_challenge(&challenge);
    challenge.skip_challenge = 0;
    strcpy(challenge.server_msg, "471 shortest_path d5c1763c420b47632d60c2cd651ea59ed19de2a9a0cda11f7daefb2c4c2b32d0 285d 25 80");
    parse_challenge_data(&challenge);
    ThreadArgs args;
    args.counter = 1;
    args.challenge = &challenge;
    CSShortestPath_solve(&args);
    */
    /*
    // unknown
    //CSShortestPath_solve(NULL);
    setup_challenge(&challenge);
    challenge.skip_challenge = 0;
    strcpy(challenge.server_msg, "471 shortest_path d5c1763c420b47632d60c2cd651ea59ed19de2a9a0cda11f7daefb2c4c2b32d0 285d 25 80");
    parse_challenge_data(&challenge);

    solve_challenge(&challenge);
    */
    /*
    // unknown
    //CSShortestPath_solve(NULL);
    setup_challenge(&challenge);
    challenge.skip_challenge = 0;
    strcpy(challenge.server_msg, "472 shortest_path 285d3c67370aea3f5ad4c9aaa015b9724210fe52245ec763d7df1e1ed7bdd73e 71d0 25 80");
    parse_challenge_data(&challenge);

    solve_challenge(&challenge);
    */

    
    CSChallenge challenge;
    struct sigevent notification;
    
    // general setup
    setup_challenge(&challenge);
    setup_ipc(&challenge, &notification);

    while(1)
	{
		// check for new challenge and update struct accordingly.
		pthread_mutex_lock(&mutex);
		if (challenge.server_msg[0] != '\0')
		{
			challenge.skip_challenge = 0;
            parse_challenge_data(&challenge);
			challenge.server_msg[0] = '\0';
		}
		pthread_mutex_unlock(&mutex);

		// run challange solver
        if (challenge.id != -1)
            solve_challenge(&challenge);	
        challenge.id = -1;
	}
}

void setup_challenge(CSChallenge* challenge)
{
    challenge->id = -1;
    challenge->server_msg[0] = '\0';
    challenge->clientq = mq_open(CLIENT_QUEUE_NAME, O_WRONLY | O_NONBLOCK);
    challenge->serverq = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_NONBLOCK);
}

void setup_ipc(CSChallenge* challenge, struct sigevent* notif)
{
    notif->sigev_notify = SIGEV_THREAD;
    notif->sigev_notify_function = &ipc_callback;
    notif->sigev_value.sival_ptr = challenge;
    challenge->notif_event = notif;

    ipc_callback(notif->sigev_value);
}

int read_server_msg(char* buffer, mqd_t serverq)
{
	printf("received server msg\n");
    int read = 0;
    while(1)
    {
        int temp_read = mq_receive(serverq, buffer, SERVER_MSG_SIZE, NULL);
		printf("temp_read: %d\n", temp_read);
        if (temp_read <= 0)
            return read;
        read = temp_read;
    }
}

void ipc_callback(union sigval challenge_ptr)
{
    char buffer[SERVER_MSG_SIZE];
	int bytesRead = 0;
    CSChallenge* challenge = (CSChallenge*)challenge_ptr.sival_ptr;

    memset(buffer, 0, SERVER_MSG_SIZE);
    mq_notify(challenge->serverq, challenge->notif_event);

    bytesRead = read_server_msg(buffer, challenge->serverq);
    if (bytesRead > 0)
    {
		pthread_mutex_lock(&mutex);
		challenge->skip_challenge = 1;
        memcpy(challenge->server_msg, buffer, bytesRead);
        challenge->server_msg[bytesRead] = '\0';
		printf("%s\n", challenge->server_msg);
		pthread_mutex_unlock(&mutex);
    }
}

