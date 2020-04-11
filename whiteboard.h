#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <errno.h>



#define MAX_USERS 100
#define MAX_TOPICS 100
#define MAX_COMMENTS 100

#define USERS_KEY 19000
#define TOPICS_KEY 20000


///////////////////////////////// SHARED MEMORY LINKED LIST /////////////////////////////////
/*
https://stackoverflow.com/questions/16655563/pointers-and-linked-list-with-shared-memory
*/



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO:
//      - edit each function to interact with shared memories - which shmid should each field have? which size?
//      - free malloc after adding.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




///////////////// STRUCTURES /////////////////

typedef struct cm{
  int id;
  //int in_reply_to;    // -1 if none
  //int replies[50];
  time_t timestamp;
  char author[32];
  char comm[1024];
  struct cm* next;
} comment;

typedef struct usr{
  int id;
  char username[32];
  char password[32];
  struct usr* next;
} user;

typedef struct tp{
  int id;
  int shmidcm;
  time_t timestamp;
  comment* commentshead;
  char author[32];
  char title[256];
  char content[1024];
  int subscribers[128];
  struct tp* next;
} topic;

typedef struct wbadmin{
  int shmidto;
  int shmidus;
  topic* topicshead;
  user* usershead;
} whiteboard;



///////////////// FUNCTIONS /////////////////
// TODO: semaphores in main for each function or before&after shmat&shmdt
// TODO: subscribers_pool (lista di topic aggiornati ordinata per userid) -> struct con campi *userid* e *lista_topics_id_subscribed* (e magari anche *next*)


// creation     //for each creation, I have to define it's shmat first and shmdt then
whiteboard* create_wb(whiteboard* w);
comment* new_comment(int id, char* author, time_t timestamp, char* comment);
topic* new_topic(int id, char* author, char* title, char* content, time_t timestamp);
user* new_user(int id, char* username, char* password);

// addition   //for each addition, I have to define it's shmat first and shmdt then
void append_topic(topic* head, topic* t);     // appends a topic to a linked list of topics
void add_topic(whiteboard* w, topic* t);

void append_user(user* head, user* u);        // appends a user to a linked list of users
void add_user(whiteboard* w, user* u);

void append_comment(comment* head, comment* c);
void push_comment(topic* t, comment* c);    // whiteboard modified? -> update_topic

void add_subscriber(topic* t, int userid);

// deletion
void del_tp(topic* head, int id_tp);
void delete_topic(whiteboard* w, int id_tp);

void del_cm(comment* head, int id_cm);
void delete_comment(topic* t, int id_cm);


// getters
topic* find_topic(topic* head, int id_topic);
topic* get_topic(whiteboard* w, int id_topic);

topic* get_last_t(topic* head);
topic* get_last_topic(whiteboard* w);

user* find_user(user* head, int id_user);
user* get_user(whiteboard* w, int id_user);

user* find_user_by_usname(user* head, char* username);
user* get_user_by_usname(whiteboard* w, char* username);

user* get_last_u(user* head);
user* get_last_user(whiteboard* w);

comment* find_comment(comment* head, int id_comment);
comment* get_comment(topic* t, int id_comment);

comment* get_last_c(comment* head);
comment* get_last_comment(topic* t);


// printers
void here_all_topics(topic* head);
void print_wb(whiteboard* w);

void here_all_comments(comment* head);
void print_tp(topic* t);

void here_all_users(user* head);  //DEBUG
void print_users(whiteboard* w);  //DEBUG


// to string
char* here_all_topics_to_string(topic* head, char* buf);
char* wb_to_string(whiteboard* w);

char* here_all_comments_to_string(comment* head, char* buf);
char* tp_to_string(topic* t);




// authenticator
int validate_user(whiteboard*w, char* us, char* pw);
char* Auth(int shmidwb, int socket_desc);
char* Register(int shmidwb, int socket_desc);




// string management
char* replace_char(char* str, char find, char replace);
