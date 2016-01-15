#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define HELLO_TYPE "HELL"

#define CONNECT_TYPE "CONN"

#define RECEIVE_TYPE "RECV"

#define TRANSMISSION_TYPE "TRAN"

#define DISCONNECT_TYPE "ENDT"

#define BYE_TYPE "BYES"

void log(const char * str1, const char * str2)
{
    printf("[%s] %s\n", str1, str2);
}

void log2(const char * str1, const char * str2, const char * str8c) {
    char name[9];
    name[8] = '\0';
    strncpy(name, str8c, 8);
    printf("[%s] %s %s\n", str1, str2, name);
}

struct base_packet
{
    char type[4];
    void * rest;
};

struct handshake_packet_rest
{
    char sender[8];
};

struct connection_packet_rest
{
    char sender[8];
    char receiver[8];
};

struct transmission_packet_rest
{
    char sender[8];
    char receiver[8];
    void * data_block;
};

size_t get_packet_size(struct base_packet * packet)
{

    if (strncmp(packet->type, HELLO_TYPE, 4) == 0 ||
            strncmp(packet->type, DISCONNECT_TYPE, 4) == 0 ||
            strncmp(packet->type, BYE_TYPE, 4) == 0)
        return (size_t) 12;
    else if (strncmp(packet->type, CONNECT_TYPE, 4) == 0)
        return (size_t) 20;
    else if (strncmp(packet->type, TRANSMISSION_TYPE, 4) == 0)
        return (size_t) (20 + 512);
    return (size_t) 0;
}

void create_handshake_packet(void * buffer, const char * type, char * sender)
{
    strncpy((char *)buffer, type, 4);
    strncpy((char *)buffer + 4, sender, 8);
}

void create_connecton_packet(void * buffer, char * sender, char * receiver)
{
    strncpy((char *)buffer, CONNECT_TYPE, 4);
    strncpy((char *)buffer + 4, sender, 8);
    strncpy((char *)buffer + 12, receiver, 8);
}

struct cln
{
    int cfd;
    struct sockaddr_in caddr;
    pthread_mutex_t write_mutex;
    struct client_list_item * client_list;
};

struct client_list_item
{
    char name[8];
    struct cln * client;
};

struct client_list_item * create_client_list()
{
    struct client_list_item * client_list = malloc(sizeof(struct client_list_item) * 128);

    int i;
    for (i = 0; i < 128; i++)
    {
        client_list[i].client = NULL;
    }
    return client_list;
}

void remove_client(struct client_list_item * client_list, const char * name)
{
    int i;
    for (i = 0; i < 128; i++)
    {
        if (strncmp(name, client_list[i].name, 8) == 0)
            client_list[i].client = NULL;
        return;
    }
}

void add_client(struct client_list_item * client_list, const char * name, struct cln * client)
{
    int i;
    for (i = 0; i < 128; i++)
    {
        if (client_list[i].client == NULL)
        {
            strncpy(client_list[i].name, name, 8);
            client_list[i].client = client;
            break;
        }
    }
    if (i == 128) {
        log("ADD_CLIENT", "Client not added");
    }
}

struct client_list_item * get_client(struct client_list_item * client_list, const char * name)
{
    int i;
    for (i = 0; i < 128; i++)
    {
        if (client_list[i].client != NULL && strncmp(name, client_list[i].name, 8) == 0)
        {
            return &client_list[i];
        }
    }
    log("GET_CLIENT", "Returned NULL");
    return NULL;
}

ssize_t write_mutexed(struct cln * c, const void * buffer, size_t count)
{
    if (c != NULL) {
        pthread_mutex_lock(&c->write_mutex);
        write(c->cfd, buffer, count);
        pthread_mutex_unlock(&c->write_mutex);
    }
}

void * cthread(void * arg)
{
    struct cln* c = (struct cln *) arg;
    char buffer[2048];
    char sender[8];
    char receiver[8];
    struct base_packet * packet = (struct base_packet *) buffer;
    void * rest = &packet->rest;
    log("THREAD", "Waiting for HELL packet...");
    read(c->cfd, buffer, 2048);
    if (strncmp(buffer, HELLO_TYPE, 4) == 0)
    {
        struct handshake_packet_rest * hs = (struct handshake_packet_rest *) rest;
        strncpy(sender, hs->sender, 8);
        log2("THREAD", "Client name is", sender);
        add_client(c->client_list, sender, c);
        write_mutexed(c, buffer, get_packet_size(packet));

        log("THREAD", "Waiting for CONN packet...");
        read(c->cfd, buffer, 2048);
        if (strncmp(buffer, CONNECT_TYPE, 4) == 0)
        {
            struct connection_packet_rest * cr = (struct connection_packet_rest *) rest;
            strncpy(receiver, cr->receiver, 8);
            log2("THREAD", "Receiver name is", receiver);
            struct client_list_item * recv = get_client(c->client_list, receiver);
            write_mutexed(recv->client, buffer, get_packet_size(buffer));

            log("THREAD", "Starting transmission...");
            while (1)
            {
                read(c->cfd, buffer, 2048);
                if (strncmp(packet->type, TRANSMISSION_TYPE, 4) != 0)
                {
                    continue;
                }
                write_mutexed(recv->client, buffer, get_packet_size(buffer));
            }
            log("THREAD", "Transmission ended.");

        }
    }
    log("THREAD", "Sending BYES...");
    create_handshake_packet(buffer, BYE_TYPE, sender);
    write_mutexed(c, buffer, get_packet_size((struct base_packet *) buffer));
    log("THREAD", "Removing client...");
    remove_client(c->client_list, sender);
    close(c->cfd);
    free(c);
    log("THREAD", "Thread closed.");
}

int main(int argc, char** argv)
{
    log("MAIN", "Tests");
    test_packet_connection();
    test_client_list();

    socklen_t slt;

    struct client_list_item * client_list = create_client_list();

    int fd = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in a, b;

    a.sin_family = PF_INET;
    a.sin_port = htons(1234);
    a.sin_addr.s_addr = inet_addr("0.0.0.0");

    bind(fd, (struct sockaddr*)&a, sizeof(a));
    if (fd == 0)
    {
        log("MAIN", "Cannot create socket");
        exit(1);
    }

    pthread_t tid;

    listen(fd, 1);

    log("MAIN", "Entering accept loop");
    while(1)
    {
        struct cln * c = malloc(sizeof(struct cln));
        slt = sizeof(c->caddr);

        log("MAIN", "Waiting for connection (accept)...");
        c->cfd = accept(fd, (struct sockaddr*)&c->caddr, &slt);
        c->client_list = client_list;

        log("MAIN", "Initializing write_mutex");
        pthread_mutex_init(&c->write_mutex, NULL);

        log("MAIN", "Creating thread...");
        pthread_create(&tid, NULL, &cthread, c);
        pthread_detach(tid);
        log("MAIN", "Thread created.");
    }

    close (fd);
    return 0;
}

void test_packet_connection()
{
    printf("[TEST] packet creation - connection packet...\n");

    char buffer[128];
    char s[9] = "users   ";
    char r[9] = "userr   ";
    create_connecton_packet(buffer, s, r);

    struct base_packet * packet = (struct base_packet *) buffer;

    if (strncmp(packet->type, CONNECT_TYPE, 4) != 0)
    {
        printf("FAILED! bad packet type\n");
        exit(1);
    }

    if (get_packet_size(buffer) != (size_t) 20)
    {
        printf("FAILED! bad size\n");
        exit(1);
    }

    struct connection_packet_rest * rest = (struct connection_packet_rest *) &packet->rest;

    if (strncmp(rest->sender, "users   ", 8) != 0)
    {
        printf("FAILED! bad sender\n");
        exit(1);
    }
    if (strncmp(rest->receiver, "userr   ", 8) != 0)
    {
        printf("FAILED! bad receiver\n");
        exit(1);
    }
    printf("PASSED!\n");
}

void test_client_list()
{

}
