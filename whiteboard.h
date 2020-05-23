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
#include <sys/sem.h>

#include <errno.h>

#include "global.h"



#define MAX_USERS 128
#define MAX_TOPICS 128
#define MAX_COMMENTS 128
#define MAX_LINKS 128

#define MAX_SUBSCRIBERS 128
#define MAX_REPLIES 64


#define USERS_KEY 19000
#define TOPICS_KEY 20000



// COMMENT'S STATUS:
//    - O: Sent (initial state)
//    - K: Published
//    - KK: Seen by all subscribers


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO:
//      - check ret value! for everything!
//      - check semaphores for choose topic, register, link//      - DONE: edit each function to interact with shared memories - which shmid should each field have? which size?
//      - check if MAX_SIZE is exceeded for each buffer -> if so, operation not permitted
//
//      - DONE: free malloc after adding.
//      - DONE: test.c (automated client that does things) -> at the end do more things and user_test name random (e.g. user_test75583)
//      - DONE: implement delete topic
//      - DONE: do SYSV semaphores in main for each function or before&after shmat&shmdt
//      - DONE: if create topic, then visualize it
//      - DONE: check if user input blocks other's executions -> DONES: add thread, reply, create topic, Register, Auth
//      - DONE: to add comment, check if subscribed
//      - DONE: choose topic: fai leggere il contenuto, perchè non lo stampa
//      - DONE: a subscribe, dopo stampa il topic a cui si è sottoscritto
//      - DONE: notifica per ogni topic nella pool in cui il current_user non è contenuto in viewers
//      - DONE: topic's viewers (lista di subscribers che hanno letto i commenti - riazzerarla ad ogni nuovo commento)
//      - DONE: subscribers pool (tabella utente|lista di topic a cui è sottoscritto)
//                implementazione subscribers_pool: struct con campi *userid* e *lista_topics_id_subscribed* (e magari anche *next*)
//      - DONE: seen: altra lista di interi per ogni commento degli utenti che lo hanno visualizzato 
//            (usando choose topic dopo essersi sottoscritti) al fine di dare un senso allo status del commento
//      - DONE: during subscription CHECK if already subscribed
//      - DONE: se il post è mio, sono automaticamente un subscriber (e un viewer)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




///////////////// STRUCTURES /////////////////

typedef struct sp{
  int userid;
  int list_subid[MAX_REPLIES];
  struct sp* next;
} subscribers_pool;


typedef struct cm{
  int id;
  int in_reply_to;    // -1 if none (isThread)
  int replies[MAX_REPLIES];
  int seen[MAX_SUBSCRIBERS];
  time_t timestamp;
  char status[4];
  char author[32];
  char comm[1024];
  struct cm* next;
} comment;

typedef struct ln{
  int id;
  int topic_id;
  int thread_id;
  struct ln* next;
} linkt;

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
  linkt linkshead[MAX_LINKS];
  char author[32];
  char title[256];
  char content[1024];
  int subscribers[MAX_SUBSCRIBERS];
  int viewers[MAX_SUBSCRIBERS];
  struct tp* next;
} topic;

typedef struct wbadmin{
  int shmidto;
  int shmidus;
  topic* topicshead;
  user* usershead;
  subscribers_pool pool[MAX_USERS];
} whiteboard;



///////////////// FUNCTIONS /////////////////


// creation     //for each creation, I have to define it's shmat first and shmdt then
whiteboard* create_wb(whiteboard* w);
comment* new_comment(int id, char* author, time_t timestamp, char* comment, int reply_id);
linkt* new_link(int id, int topic_id, int thread_id);
topic* new_topic(int id, char* author, char* title, char* content, time_t timestamp);
user* new_user(int id, char* username, char* password);
subscribers_pool* new_entry_pool(int uid);

// addition   //for each addition, I have to define it's shmat first and shmdt then
void append_topic(topic* head, topic* t);     // appends a topic to a linked list of topics
void add_topic(whiteboard* w, topic* t);

void append_user(user* head, user* u);        // appends a user to a linked list of users
void add_user(whiteboard* w, user* u);

void append_comment(comment* head, comment* c);
void push_comment(topic* t, comment* c);    // whiteboard modified? -> update_topic

void append_link(linkt* head, linkt* l);
void add_link(topic* t, linkt* l);

void add_subscriber(topic* t, int userid);
void add_viewer(topic* t,int uid);
void add_reply(comment* r,int id_comm);
void add_seen(comment* c,int uid);
void add_all_seen(comment* head,int uid);
void autp(subscribers_pool* head, int uid);
void add_user_to_pool(whiteboard* w, int uid);
void add_subscription_entry(whiteboard* w, int uid, int tid);



// deletion
void del_us(user* head, int id_us);
void delete_user(whiteboard* w, int id_us);

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

int* find_list_from_pool(subscribers_pool* head, int uid);
int* get_list_from_pool(whiteboard* w, int uid);


// printers
void here_all_topics(topic* head);
void print_wb(whiteboard* w);

void here_all_comments(comment* head);
void print_tp(topic* t);

void here_all_users(user* head);  //DEBUG
void print_users(whiteboard* w);  //DEBUG

void print_replies(comment* r);
void print_arr(int* arr);
void print_pool(whiteboard* w);




// to string
char* here_all_topics_to_string(topic* head, char* buf);
char* wb_to_string(whiteboard* w);

char* here_all_users_to_string(user* head, char* buf);
char* us_to_string(whiteboard* w);

char* here_all_comments_to_string(topic* t, comment* head, char* buf, int* done, int subscribed);
char* tp_to_string(topic* t, int subscribed);

char* child_to_string(topic* t, comment* child, char* buf, int* done, int level);
char* cm_to_string(topic* t, comment* head, char* buf, int* done);

linkt* find_link(linkt* head, int id);
char* ln_to_string(whiteboard* w, topic* t, int id, char* buf);




// authenticator
int validate_user(whiteboard*w, char* us, char* pw);
char* Auth(int shmidwb, int socket_desc, int mutex);
char* Register(int shmidwb, int socket_desc, int mutex);




// string management
char* replace_char(char* str, char find, char replace);


//utils
int int_in_arr(int* arr, int i);
int* add_to_arr(int* arr, int i, int max_size);



// checks
int check_seen_by_all(int* subscribers, int* seen);   //can be generalized: check A subset of B
void check_all_seen_by_all(int* subscribers, comment* head);










// semaphores

int initsem (key_t semkey);
void semclean(key_t semkey);

int Pwait (int semid);
int Vpost (int semid);
