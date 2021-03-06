#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>     // work with directories
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h> 

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
//      - DONE: handle all recv/send's ERRORS within witeboard.c
//      - DONE: status message: print date
//      - DONE: check ret value! for everything!
//      - DONE: check semaphores for choose topic, register, link//      - DONE: edit each function to interact with shared memories - which shmid should each field have? which size?
//      - DONE: check if MAX_SIZE is exceeded for each buffer -> if so, operation not permitted
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

typedef struct sp{              // to understand a user is subscribed to which topics 
  int userid;
  int list_subid[MAX_REPLIES];
  struct sp* next;
} subscribers_pool;


typedef struct cm{              // comment
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

typedef struct ln{      // link to another thread
  int id;
  int topic_id;
  int thread_id;
  struct ln* next;
} linkt;

typedef struct usr{              // user
  int id;
  char username[32];
  char password[32];
  struct usr* next;
} user;

typedef struct tp{              // topic
  int id;
  int shmidcm;
  time_t timestamp;
  comment commentshead[MAX_COMMENTS];
  linkt linkshead[MAX_LINKS];
  char author[32];
  char title[256];
  char content[1024];
  int subscribers[MAX_SUBSCRIBERS];
  int viewers[MAX_SUBSCRIBERS];
  struct tp* next;
} topic;

typedef struct wbadmin{              // whiteboard
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
topic* new_topic(int id, char* author, char* title, char* content);
user* new_user(int id, char* username, char* password);
subscribers_pool* new_entry_pool(int uid);

// addition   //for each addition, I had to define it's shmat first and shmdt then
void append_topic(topic* head, topic* t);     // appends a topic to a linked list of topics
void add_topic(whiteboard* w, topic* t);

void append_user(user* head, user* u);        // appends a user to a linked list of users
void add_user(whiteboard* w, user* u);

void append_comment(comment* head, comment* c);
void push_comment(topic* t, comment* c);   // adds a new comment to a topic

void append_link(linkt* head, linkt* l);
void add_link(topic* t, linkt* l);   // adds a new link to a topic

void add_subscriber(topic* t, int userid);    // add a user to subscribers list
void add_viewer(topic* t,int uid);            // a user have seen all messages of this topics
void add_reply(comment* r,int id_comm);       // to fill the list of messages that reply to a comment
void add_seen(comment* c,int uid);            // a user have seen that comment
void add_all_seen(comment* head,int uid);
void autp(subscribers_pool* head, int uid);
void add_user_to_pool(whiteboard* w, int uid);    // add a user to subscribers pool
void add_subscription_entry(whiteboard* w, int uid, int tid);   // add a topic to the topic list inside a subscribers pool entry



// deletion
void del_us(user* head, int id_us);
void delete_user(whiteboard* w, int id_us);

void del_tp(topic* head, int id_tp);
void delete_topic(whiteboard* w, int id_tp);

void del_cm(comment* head, int id_cm);
void delete_comment(topic* t, int id_cm);


// getters
topic* find_topic(topic* head, int id_topic);   // find by id
topic* get_topic(whiteboard* w, int id_topic);

topic* get_last_t(topic* head);         // get topic with highest id
topic* get_last_topic(whiteboard* w);

user* find_user(user* head, int id_user);   // find by id
user* get_user(whiteboard* w, int id_user);

user* find_user_by_usname(user* head, char* username);   // find by username
user* get_user_by_usname(whiteboard* w, char* username);

user* get_last_u(user* head);           // get user with highest id
user* get_last_user(whiteboard* w);

comment* find_comment(comment* head, int id_comment);   // find by id
comment* get_comment(topic* t, int id_comment);

comment* get_last_c(comment* head);     // get comment with highest id
comment* get_last_comment(topic* t);

int* find_list_from_pool(subscribers_pool* head, int uid);    // get topic list from sub. pool (by user id)
int* get_list_from_pool(whiteboard* w, int uid);


// printers
void here_all_topics(topic* head);
void print_wb(whiteboard* w);       // prints all topics

void here_all_comments(comment* head);
void print_tp(topic* t);            // prints all comments

void here_all_users(user* head);  //DEBUG
void print_users(whiteboard* w);  //DEBUG   // prints all users

void print_replies(comment* r);   // prints all reply IDs about a comment
void print_arr(int* arr);         // prints an array of integers
void print_pool(whiteboard* w);   // prints subscribers pool 




// to string          // used to send to the client the output of the command
char* here_all_topics_to_string(topic* head, char* buf);
char* wb_to_string(whiteboard* w);      // preparing all topics to be sent to the client

char* here_all_users_to_string(user* head, char* buf);
char* us_to_string(whiteboard* w);      // preparing all users to be sent to the client

char* here_all_comments_to_string(topic* t, comment* head, char* buf, int* done, int subscribed);
char* tp_to_string(topic* t, int subscribed);      // preparing all comments of a topic to be sent to the client

char* child_to_string(topic* t, comment* child, char* buf, int* done, int level);
char* cm_to_string(topic* t, comment* head, char* buf, int* done);      // preparing a comment to be sent to the client

linkt* find_link(linkt* head, int id);
char* ln_to_string(whiteboard* w, topic* t, int id, char* buf);      // preparing a links to be sent to the client




// authenticator
int validate_user(whiteboard*w, char* us, char* pw);        // existing user? right credentials?
char* Auth(whiteboard* w, int socket_desc, int mutex);      // Authentication
char* Register(whiteboard* w, int socket_desc, int mutex);  // Registration




// string management
char* replace_char(char* str, char find, char replace);   // replaces the first occurrence of a character with another inside a string


//utils
int int_in_arr(int* arr, int i);      // check if an integer is contained in the array
int* add_to_arr(int* arr, int i, int max_size);     // add an integer to an array
int get_digit(char *buf, int i);        // get the number inside the command (when needed)



// checks
int check_seen_by_all(int* subscribers, int* seen);         // can be generalized: check A subset of B
void check_all_seen_by_all(int* subscribers, comment* head);










// semaphores
int initsem (key_t semkey, int size);
void semclean(key_t semkey);

int Pwait (int semid);    // wait
int Vpost (int semid);    // post





// encryption/decryption    (this affects a lot performances)
void encrypt(char* filename);   // encrypts a file (PGP)
void decrypt(char* filename);   // decrypts a PGP file

void encryptall(char* folder);  // encrypts all files inside a folder
void decryptall(char* folder);  // decrypts all files inside a folder

void rmenc(char* folder);       // removes all encrypted files within a folder
void rmdec(char* folder);       // removes all unencrypted files within a folder



// SAVE and LOAD
void write_arr(int *arr, FILE * file);
void write_comments(comment *head, FILE * file);
void write_links(linkt *head, FILE * file);
void write_topics(topic *head, FILE * file);
void write_users(user *head, FILE * file);
void save_wb(whiteboard* w);      // writes the whiteboard inside files


int* read_arr(FILE* file, int* arr);
void read_comments(comment *head, FILE * file);
void read_links(linkt *head, FILE * file);
void read_topics(topic *head, FILE * file);
void load_wb(whiteboard* w);      // reads files to reload the whiteboard

void SAVE(whiteboard* w);     // high level function that uses encryption/decryption
void LOAD(whiteboard* w);     // high level function that uses encryption/decryption