#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include "socket.h"

#ifndef PORT
    #define PORT 59482
#endif

#define LISTEN_SIZE 5
#define WELCOME_MSG "Welcome to CSC209 Twitter! Enter your username:\r\n"
#define SEND_MSG "send"
#define SHOW_MSG "show"
#define FOLLOW_MSG "follow"
#define UNFOLLOW_MSG "unfollow"
#define BUF_SIZE 256
#define MSG_LIMIT 8
#define FOLLOW_LIMIT 5

struct client {
    int fd;
    int num_following;
    int num_followers;
    int num_msg;
    struct in_addr ipaddr;
    char username[BUF_SIZE];
    char message[MSG_LIMIT][BUF_SIZE];
    struct client *following[FOLLOW_LIMIT]; // Clients this user is following
    struct client *followers[FOLLOW_LIMIT]; // Clients who follow this user
    char inbuf[BUF_SIZE]; // Used to hold input from the client
    char *in_ptr; // A pointer into inbuf to help with partial reads
    struct client *next;
};


// Provided functions.
void add_client(struct client **clients, int fd, struct in_addr addr);
void remove_client(struct client **clients, int fd);

// These are some of the function prototypes that we used in our solution
// You are not required to write functions that match these prototypes, but
// you may find them helpful when thinking about operations in your program.

// Send the message in s to all clients in active_clients.
void announce(struct client *active_clients, char *s) {
    struct client *p;
    for (p = active_clients; p != NULL; p = p->next) {
      if (write(p->fd, s, strlen(s)) == -1) {
          fprintf(stderr, "Write to client failed\n");
      }
    }
}

// Move client c from new_clients list to active_clients list.
void activate_client(struct client *c,
    struct client **active_clients_ptr, struct client **new_clients_ptr){
    //add our client to active clients linked lists
    c->next = *active_clients_ptr;
    *active_clients_ptr = c;
    //client we are searching for is the head of the new clients linked list
    if ((*new_clients_ptr)->fd == c->fd){
      *new_clients_ptr = (*new_clients_ptr)->next;
    }

    //if the client we are searching for is not at the head of new clients
    else{
      struct client *p;
      for (p = *new_clients_ptr; p != NULL; p = p->next) {
        if ((p->next)->fd == c->fd) {
          p->next = (p->next) -> next;
          break;
        }
      }
    }
}

// The set of socket descriptors for select to monitor.
// This is a global variable because we need to remove socket descriptors
// from allset when a write to a socket fails.
fd_set allset;


int validate_username(struct client *c, struct client *active_clients) {
    int space_left = &(c->inbuf[BUF_SIZE]) - c->in_ptr;
    char *line_end_ptr = NULL;
    // it may take more than one read to get all of the data that was written
    do {
      int num_chars = read(c->fd, c->in_ptr, space_left);
      if (num_chars == 0){
        fprintf(stderr, "Client Unexpectedly Disconnected\n");
        return 2;
      }
      c->in_ptr += num_chars;
      *(c->in_ptr) = '\0';
      space_left -= num_chars;
    }  while((line_end_ptr = strstr(c->inbuf, "\r\n")) == NULL);
    int length = line_end_ptr - c->inbuf;

    //used to test validity of input
    char test_username[BUF_SIZE];
    strncpy(test_username, c->inbuf, length);
    test_username[length] = '\0';

    //shift over characters in buffer
    for (int i = length + 2; i < BUF_SIZE; i++){
      c->inbuf[i - (length + 2)] = c->inbuf[i];
    }
    c->in_ptr -= length + 2;
    if (length == 0){
      char *msg = "This username is not available, please choose another\r\n";
      if (write(c->fd, msg, strlen(msg)) == -1){
        fprintf(stderr, "Write to client failed\n");
      }
      return 0;
    }

    struct client *p;
    for (p = active_clients; p != NULL; p = p->next) {
      if (strncmp(test_username, p->username, length) == 0){
        char *msg = "This username is not available, please choose another\r\n";
        if (write(c->fd, msg, strlen(msg)) == -1){
          fprintf(stderr, "Write to client failed\n");
        }
        return 0;
      }
    }
    strncpy(c->username, test_username, length);
    c->username[length] = '\0';
    return 1;
}

/*
 * Create a new client, initialize it, and add it to the head of the linked
 * list.
 */
void add_client(struct client **clients, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));
    p->fd = fd;
    p->ipaddr = addr;
    p->username[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *clients;
    p->num_followers = 0;
    p->num_following = 0;
    p->num_msg = 0;
    // initialize messages to empty strings
    for (int i = 0; i < MSG_LIMIT; i++) {
        p->message[i][0] = '\0';
    }

    *clients = p;
}

/*
 * Remove client from the linked list and close its socket.
 * Also, remove socket descriptor from allset.
 */
void remove_client(struct client **clients, int fd) {
    struct client **p;

    for (p = clients; *p && (*p)->fd != fd; p = &(*p)->next)
        ;

    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        // TODO: Remove the client from other clients' following/followers
        // lists
        struct client *c;
        for (c = *clients; c != NULL; c = c->next){
          //Remove client from other clients' follower list
          for (int i = 0; i < c->num_followers; i++){
            struct client *curr_client = c->followers[i];
            if (strcmp(curr_client->username, (*p)->username) == 0){
              for(int l = curr_client->num_followers; l > i; l--){
                curr_client->followers[l-1] = curr_client->followers[l];
              }
              curr_client->num_followers -= 1;
              curr_client->followers[curr_client->num_followers] = NULL;
            }
          }
          //Remove client from other clients' following list
          for (int i = 0; i < c->num_following; i++){
            struct client *curr_client = c->following[i];
            if (strcmp(curr_client->username, (*p)->username) == 0){
              for(int l = curr_client->num_following; l > i; l--){
                curr_client->following[l-1] = curr_client->following[l];
              }
              curr_client->num_following -= 1;
              curr_client->following[curr_client->num_following] = NULL;
            }
          }
        }
        // Remove the client
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;
    } else {
        fprintf(stderr,
            "Trying to remove fd %d, but I don't know about it\n", fd);
    }
}

//executes instructions for when the client types a follow command
int handle_follow(struct client *c, struct client **active_clients, char *input){
    int handled = 0;
    struct client *p;
    for (p = *active_clients; p != NULL; p = p->next) {
      if (strncmp(p->username, &(input[7]), strlen(p->username)) == 0){
        if(p->num_followers < FOLLOW_LIMIT && c->num_following < FOLLOW_LIMIT){
          p->followers[p->num_followers] = c;
          c->following[c->num_following] = p;
          p->num_followers += 1;
          c->num_following += 1;
        }
        else{
          char *msg = "You cannot follow this individual\r\n";
          if (write(c->fd, msg, strlen(msg)) == -1){
            fprintf(stderr, "Write to client failed\n");
          }
        }
        handled = 1;
        return handled;
      }
    }
    return handled;
}

//executes instructions for when the client types an unfollow command
int handle_unfollow(struct client *c, struct client **active_clients, char *input){
    int handled = 0;
    for(int i = 0; i < c->num_following; i++){
      if (strncmp((c->following[i])->username, &(input[9]), strlen((c->following[i])->username)) == 0){
        struct client *other_user = c->following[i];
        for(int j = c->num_following; j > i; j--){
          c->following[j-1] = c->following[j];
        }
        c->num_following -= 1;
        c->following[c->num_following] = NULL;
        for(int k = 0; k < other_user->num_followers; k++){
          if (strncmp(c->username, (other_user->followers[i])->username, strlen(c->username)) == 0){
            for(int l = other_user->num_followers; l > k; l--){
              other_user->followers[l-1] = other_user->followers[l];
            }
            other_user->num_followers -= 1;
            other_user->followers[other_user->num_followers] = NULL;
          }
        }
        handled = 1;
      }
    }
    return handled;
}

//executes instructions for when the client types a show command
void handle_show(struct client *c, struct client **active_clients){
    for(int i = 0; i < c->num_following; i++) {
      for(int j = 0; j < (c->following[i])->num_msg; j++) {
        char message[BUF_SIZE * 2];
        sprintf(message, "%s wrote: %s\r\n", (c->following[i])->username, (c->following[i])->message[j]);
        if (write(c->fd, message, strlen(message)) == -1){
          fprintf(stderr, "Write to client failed\n");
        }
      }
    }
}

//executes instructions for when the client types a send command
int handle_send(struct client *c, struct client **active_clients, char *input){
    if (c->num_msg >= MSG_LIMIT){
      return 0;
    }
    else{
      strncpy(c->message[c->num_msg], &(input[5]), 140);
      char message[BUF_SIZE + 30];
      sprintf(message, "%s messages: %s\r\n", c->username, c->message[c->num_msg]);
      for (int i = 0; i < c->num_followers; i++){
        if (write((c->followers[i])->fd, message, strlen(message)) == -1){
          fprintf(stderr, "Write to client failed\n");
        }
      }
      c->num_msg += 1;
      return 1;
    }
}

//executes instructions to read client input
int handle_input(struct client *c, struct client **active_clients){
    int space_left = &(c->inbuf[BUF_SIZE]) - c->in_ptr;
    char *line_end_ptr = NULL;
    // it may take more than one read to get all of the data that was written
    while((line_end_ptr = strstr(c->inbuf, "\r\n")) == NULL) {
      int num_chars = read(c->fd, c->in_ptr, space_left);
      if (num_chars == 0){
        fprintf(stderr, "Client Unexpectedly Disconnected\n");
        return 1;
      }
      c->in_ptr += num_chars;
      *(c->in_ptr) = '\0';
      space_left -= num_chars;
    }
    int length = line_end_ptr - c->inbuf;

    //used to test input cases
    char test_input[BUF_SIZE];
    strncpy(test_input, c->inbuf, length);
    test_input[length] = '\0';

    //shift over characters in buffer
    for (int i = length + 2; i < BUF_SIZE; i++){
      c->inbuf[i - (length + 2)] = c->inbuf[i];
    }
    c->in_ptr -= length + 2;

    if (strncmp(test_input, "follow ", 7) == 0){
      if (handle_follow(c, active_clients, test_input) == 0){
        char *msg = "Invalid username. Cannot follow.\r\n";
        if (write(c->fd, msg, strlen(msg)) == -1){
          fprintf(stderr, "Write to client failed\n");
        }
      }
    }
    else if (strncmp(test_input, "unfollow ", 9) == 0){
      if (handle_unfollow(c, active_clients, test_input) == 0){
        char *msg = "Cannot unfollow user with this username.\r\n";
        if (write(c->fd, msg, strlen(msg)) == -1){
          fprintf(stderr, "Write to client failed\n");
        }
      }
    }
    else if (strncmp(test_input, "show", 4) == 0 && strlen(test_input) == 4){
      handle_show(c, active_clients);
    }
    else if (strncmp(test_input, "send ", 5) == 0){
      if (handle_send(c, active_clients, test_input) == 0){
        char *msg = "you cannot send any more messages\r\n";
        if (write(c->fd, msg, strlen(msg)) == -1){
          fprintf(stderr, "Write to client failed\n");
        }
      }
    }
    else if (strncmp(test_input, "quit", 4) == 0 && strlen(test_input) == 4){
        remove_client(active_clients, c->fd);
    }
    else{
      char *msg = "Invalid command\r\n";
      write(c->fd, msg, strlen(msg));
    }
    return 0;
}


int main (int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;

    // If the server writes to a socket that has been closed, the SIGPIPE
    // signal is sent and the process is terminated. To prevent the server
    // from terminating, ignore the SIGPIPE signal.
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // A list of active clients (who have already entered their names).
    struct client *active_clients = NULL;

    // A list of clients who have not yet entered their names. This list is
    // kept separate from the list of active clients, because until a client
    // has entered their name, they should not issue commands or
    // or receive announcements.
    struct client *new_clients = NULL;

    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, LISTEN_SIZE);
    free(server);
    // Initialize allset and add listenfd to the set of file descriptors
    // passed into select
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            exit(1);
        } else if (nready == 0) {
            continue;
        }

        // check if a new client is connecting
        if (FD_ISSET(listenfd, &rset)) {
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd, &q);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_client(&new_clients, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if (write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr,
                    "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_client(&new_clients, clientfd);
            }
        }

        // Check which other socket descriptors have something ready to read.
        // The reason we iterate over the rset descriptors at the top level and
        // search through the two lists of clients each time is that it is
        // possible that a client will be removed in the middle of one of the
        // operations. This is also why we call break after handling the input.
        // If a client has been removed, the loop variables may no longer be
        // valid.
        int cur_fd, handled;
        for (cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
            if (FD_ISSET(cur_fd, &rset)) {
                handled = 0;

                // Check if any new clients are entering their names
                for (p = new_clients; p != NULL; p = p->next) {
                    if (cur_fd == p->fd) {
                        int handler = 0;
                        handler = validate_username(p, active_clients);
                        if (handler == 1){
                          activate_client(p, &active_clients, &new_clients);
                          char announcement[BUF_SIZE * 2];
                          sprintf(announcement, "%s has just joined.\r\n", p->username);
                          announce(active_clients, announcement);
                        }
                        else if(handler == 2){
                          remove_client(&new_clients, p->fd);
                        }
                        handled = 1;
                        break;
                    }
                }

                if (!handled) {
                    // Check if this socket descriptor is an active client
                    for (p = active_clients; p != NULL; p = p->next) {
                        if (cur_fd == p->fd) {
                            if (handle_input(p, &active_clients) == 1){
                              remove_client(&active_clients, p->fd);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
