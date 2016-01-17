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

void logm(const char * str1, const char * str2)
{
    printf("[%s] %s\n", str1, str2);
}

void logsr(const char * str1, const char * str2, const char * str8current, const char * str8other)
{
    printf("[%s] %s ", str1, str2);
    if (str8current != NULL)
    {
        printf("s: <%.8s> ", str8current);
    }
    if (str8current != NULL)
    {
        printf("r: <%.8s> ", str8current);
    }
    printf("\n");
}

void logt(const char * str1, const char * str2, pthread_t thread)
{
    printf("[%s] %s t: <%d>\n", str1, str2, (int) thread);
}

/*!
 * Size of buffers used in this server (max length of message).
 */
#define BUFFER_SIZE (2048)

/*!
 * Sound is sent in blocks of this size.
 */
#define DATA_BLOCK_SIZE (512)

/*!
 * Message type field is that long.
 */
#define TYPE_FIELD_SIZE (8)

/*!
 * User name field is that long.
 */
#define NAME_FIELD_SIZE (8)

/*!
 * Base packet structure.
 */
struct base_packet
{

    char type[TYPE_FIELD_SIZE]; //! < Char array of TYPE_FIELD_SIZE indicating message's type.

    void * rest; //! < Rest of message depending on the type.

};

#define HELLO_TYPE "HELLOOOO"
#define BYE_TYPE "BYESSSSS"
/*!
 * Packets with this content are responsible for registering
 * user's name ("HELL") and saying goodbye in case of disconnection ("BYES").
 */
struct handshake_packet_rest
{

    char sender[NAME_FIELD_SIZE];

};

#define CONNECT_TYPE "CONNECTT"
/*!
 * Packets with this are sent to client when other user want 
 * to start transmission. The client responds with the same packet
 * type to receive call or ignores it. 
 */
struct connection_packet_rest
{

    char sender[NAME_FIELD_SIZE];

    char receiver[NAME_FIELD_SIZE];

};

#define TRANSMISSION_TYPE "TRANSMIT"
/*!
 * Packets of this type are sent during sound transmission.
 */
struct transmission_packet_rest
{

    char sender[NAME_FIELD_SIZE];

    char receiver[NAME_FIELD_SIZE];

    void * data_block;

};

/*!
 * This function returns expected message content size
 * based on message's type.
 */
size_t get_packet_size(struct base_packet * packet)
{
    if (strncmp(packet->type, HELLO_TYPE, TYPE_FIELD_SIZE) == 0 
            || strncmp(packet->type, BYE_TYPE, TYPE_FIELD_SIZE) == 0)
    {
        return (size_t) (TYPE_FIELD_SIZE + NAME_FIELD_SIZE);
    }
    else if (strncmp(packet->type, CONNECT_TYPE, TYPE_FIELD_SIZE) == 0)
    {
        return (size_t) (TYPE_FIELD_SIZE + NAME_FIELD_SIZE + NAME_FIELD_SIZE);
    }
    else if (strncmp(packet->type, TRANSMISSION_TYPE, TYPE_FIELD_SIZE) == 0)
    {
        return (size_t) (TYPE_FIELD_SIZE + NAME_FIELD_SIZE + NAME_FIELD_SIZE + DATA_BLOCK_SIZE);
    }
    return (size_t) 0;
}

/*!
 * This procedure creates HELL or BYES message with given sender name. 
 */
void create_handshake_packet(void * buffer, const char * type, char * sender)
{
    strncpy((char *)(buffer), type, TYPE_FIELD_SIZE);
    strncpy((char *)(buffer) + TYPE_FIELD_SIZE, sender, NAME_FIELD_SIZE);
}

/*!
 * This procedure creates CONN message with given sender and receiver name. 
 */
void create_connecton_packet(void * buffer, char * sender, char * receiver)
{
    strncpy((char *)(buffer), CONNECT_TYPE, TYPE_FIELD_SIZE);
    strncpy((char *)(buffer) + TYPE_FIELD_SIZE, sender, NAME_FIELD_SIZE);
    strncpy((char *)(buffer) + TYPE_FIELD_SIZE + NAME_FIELD_SIZE, receiver, NAME_FIELD_SIZE);
}

/*!
 * This structure holds single client's connection data. 
 */
struct cln
{

    int cfd; //! < Connection's file descriptor.

    struct sockaddr_in caddr; //! < Remote client's address.

    pthread_mutex_t write_mutex; //! < Mutex used when writing to cfd.

    struct client_list_item * client_list; //! < Pointer to list of clients (needed in thread).

};

/*!
 * Simple structure for mapping user's name with his connection data.
 * Used with:
 *   - struct client_list_item * create_client_list()
 *   - void remove_client(struct client_list_item * client_list, const char * name)
 *   - int add_client(struct client_list_item * client_list, const char * name, struct cln * client)
 *   - struct client_list_item * get_client(struct client_list_item * client_list, const char * name)
 */
struct client_list_item
{
    char name[NAME_FIELD_SIZE];
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
        if (strncmp(name, client_list[i].name, NAME_FIELD_SIZE) == 0) {
            client_list[i].client = NULL;
		    strncpy(client_list[i].name, "        ", NAME_FIELD_SIZE);
            return;
		}
    }
}

int add_client(struct client_list_item * client_list, const char * name, struct cln * client)
{
    int i;
    for (i = 0; i < 128; i++)
    {
        if (strncmp(client_list[i].name, name, NAME_FIELD_SIZE) == 0)
        {
            return 0;
        }
    }
    for (i = 0; i < 128; i++)
    {
        if (client_list[i].client == NULL)
        {
            strncpy(client_list[i].name, name, NAME_FIELD_SIZE);
            client_list[i].client = client;
            return 1;
        }
    }
    return 0;
}

struct client_list_item * get_client(struct client_list_item * client_list, const char * name)
{
    int i;
    for (i = 0; i < 128; i++)
    {
        if (client_list[i].client != NULL && strncmp(name, client_list[i].name, NAME_FIELD_SIZE) == 0)
        {
            return &client_list[i];
        }
    }
    logsr("get_client", "Returned NULL", name, NULL);
    return NULL;
}

/*!
 * Procedure can be used to write to clients from thread.
 * Sending data should be thread-safe this way.
 */
void write_mutexed(struct cln * c, const void * buffer, size_t count)
{
    if (c != NULL)
    {
        pthread_mutex_lock(&c->write_mutex);
        write(c->cfd, buffer, count);
        pthread_mutex_unlock(&c->write_mutex);
    }
}

void * cthread(void * arg)
{
    struct cln* c = (struct cln *) arg;

    char buffer[BUFFER_SIZE];
    char sender[NAME_FIELD_SIZE];
    char receiver[NAME_FIELD_SIZE];

    struct base_packet * packet = (struct base_packet *) buffer;
    void * rest = &packet->rest;

    logt("THREAD", "Waiting for HELL packet", pthread_self());
    read(c->cfd, buffer, BUFFER_SIZE);
    if (strncmp(buffer, HELLO_TYPE, TYPE_FIELD_SIZE) == 0)
    {
        struct handshake_packet_rest * hs = (struct handshake_packet_rest *) rest;
        strncpy(sender, hs->sender, NAME_FIELD_SIZE);
        logsr("THREAD", "HELL packet received", sender, NULL);
        if (add_client(c->client_list, sender, c))
        {
	    // responding to HELL message 
            write_mutexed(c, buffer, get_packet_size(packet));

            logsr("THREAD", "Waiting for CONN packet", sender, NULL);
            read(c->cfd, buffer, BUFFER_SIZE);
            if (strncmp(buffer, CONNECT_TYPE, TYPE_FIELD_SIZE) == 0)
            {
                struct connection_packet_rest * cr = (struct connection_packet_rest *) rest;
                strncpy(receiver, cr->receiver, NAME_FIELD_SIZE);
                logsr("THREAD", "CONN packet received", sender, receiver);
                struct client_list_item * recv = get_client(c->client_list, receiver);
                if (recv != NULL)
                {
                    // transfering CONN message
                    write_mutexed(recv->client, buffer, get_packet_size(packet));

                    logsr("THREAD", "Starting transmission", sender, receiver);
                    while (1)
                    {
                        // receiving transmission message
                        read(c->cfd, buffer, BUFFER_SIZE);
                        if (strncmp(packet->type, BYE_TYPE, TYPE_FIELD_SIZE) == 0)
                        {
                            break;
                        }
                        if (strncmp(packet->type, TRANSMISSION_TYPE, TYPE_FIELD_SIZE) != 0)
                        {
                            continue;
                        }
			// transfering transmission message
                        write_mutexed(recv->client, buffer, get_packet_size(packet));
                    }
                    logsr("THREAD", "Transmission ended", sender, receiver);
                }
		else
                {
					logsr("THREAD", "Couldn't find requested user", sender, receiver);
                }
            }
            logsr("THREAD", "Removing client from user list", sender, NULL);
            remove_client(c->client_list, sender);
        }
        logsr("THREAD", "Sending BYES", sender, NULL);
        create_handshake_packet(buffer, BYE_TYPE, sender);
        write_mutexed(c, buffer, get_packet_size((struct base_packet *) buffer));
    }
    
    close(c->cfd);
    free(c);
    logt("THREAD", "Thread closed", pthread_self());
    return NULL;
}

void test_packet_connection()
{
    printf("TEST - packet creation - connection packet...\n");

    char buffer[BUFFER_SIZE];
    char s[] = "userssss";
    char r[] = "userrrrr";
    create_connecton_packet(buffer, s, r);

    struct base_packet * packet = (struct base_packet *) buffer;

    if (strncmp(packet->type, CONNECT_TYPE, TYPE_FIELD_SIZE) != 0)
    {
        printf("FAILED! bad packet type\n");
        exit(1);
    }

    if (get_packet_size(packet) != (size_t)(TYPE_FIELD_SIZE + NAME_FIELD_SIZE + NAME_FIELD_SIZE))
    {
        printf("FAILED! bad size\n");
        exit(1);
    }

    struct connection_packet_rest * rest = (struct connection_packet_rest *) &(packet->rest);

    if (strncmp(rest->sender, s, NAME_FIELD_SIZE) != 0)
    {
        printf("FAILED! bad sender\n");
        printf("\n%.30s;\n%.8s;\n%.8s;\n%.20s;\n", buffer, rest->sender, rest->receiver, (char*)&packet->rest);
        exit(1);
    }
    if (strncmp(rest->receiver, r, NAME_FIELD_SIZE) != 0)
    {
        printf("FAILED! bad receiver\n");
        exit(1);
    }
    printf("PASSED!\n");
}

int main(int argc, char** argv)
{
    logm("MAIN", "Tests");
    test_packet_connection();

    socklen_t slt;

    struct client_list_item * client_list = create_client_list();

    int fd = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in a;
    a.sin_family = PF_INET;
    a.sin_port = htons(1234);
    a.sin_addr.s_addr = inet_addr("0.0.0.0");

    bind(fd, (struct sockaddr*)&a, sizeof(a));
    if (fd == 0)
    {
        logm("MAIN", "Cannot create socket");
        exit(1);
    }

    pthread_t tid;

    listen(fd, 1);

    logm("MAIN", "Entering accept loop");
    while(1)
    {
        struct cln * c = malloc(sizeof(struct cln));
        slt = sizeof(c->caddr);

        logm("MAIN", "Waiting for connection (accept)...");
        c->cfd = accept(fd, (struct sockaddr*)&c->caddr, &slt);
        c->client_list = client_list;

        logm("MAIN", "Initializing write_mutex");
        pthread_mutex_init(&c->write_mutex, NULL);

        logm("MAIN", "Creating thread...");
        pthread_create(&tid, NULL, &cthread, c);
        pthread_detach(tid);
        logm("MAIN", "Thread created.");
    }

    close (fd);
    return 0;
}
