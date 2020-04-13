#include "whiteboard.h"



// creation
whiteboard* create_wb(whiteboard* w){
  // users
  if ((w->shmidus = shmget(USERS_KEY, sizeof(user)*MAX_USERS, IPC_CREAT | 0666)) < 0) {
      perror("shmget");
      exit(1);
  }

  // topics
  if ((w->shmidto = shmget(TOPICS_KEY, sizeof(topic)*MAX_TOPICS, IPC_CREAT | 0666)) < 0) {
      perror("shmget");
      exit(1);
  }
  w->topicshead=(topic*) shmat(w->shmidto, NULL, 0);
  time_t date;
  time(&date);
  *(w->topicshead)=*(new_topic(0, "admin\n", "First topic.\n", "Hello, world! This is my firs topic.\n", date));
  shmdt(w->topicshead);
  w->usershead=(user*) shmat(w->shmidus, NULL, 0);
  *(w->usershead)=*(new_user(0, "admin\n", "admin\n"));
  shmdt(w->usershead);
  return w;
}

comment* new_comment(int id, char* author, time_t timestamp, char* comm, int reply_id){
  comment* c = (comment*) malloc(sizeof(comment));
  c->id=id;
  c->timestamp=timestamp;

  strncpy(c->author, author, 32);
  c->author[strlen(c->author)-1]='\0';  // stringa troncata se troppo lunga

  strncpy(c->comm, comm, 32);
  c->comm[strlen(c->comm)-1]='\0';  // stringa troncata se troppo lunga

  c->next=NULL;
  c->in_reply_to=reply_id;

  memset(c->replies, -1, MAX_REPLIES*sizeof(int));
  return c;
}

topic* new_topic(int id, char* author, char* title, char* content, time_t timestamp){
  topic* t = (topic*) malloc(sizeof(topic));
  t->id=id;
  strncpy(t->author, author, 32);
  t->author[strlen(t->author)-1]='\0';  // stringa troncata se troppo lunga

  strncpy(t->title, title, 256);
  t->title[strlen(t->title)-1]='\0';  // stringa troncata se troppo lunga

  strncpy(t->content, content, 1024);
  t->content[strlen(t->content)-1]='\0';  // stringa troncata se troppo lunga

  t->timestamp=timestamp;
  if ((t->shmidcm = shmget(30000+(t->id*MAX_COMMENTS), sizeof(comment)*MAX_COMMENTS, IPC_CREAT | 0666)) < 0) {
      perror("shmget");
      exit(1);
  }
  t->commentshead= (comment*)shmat(t->shmidcm, NULL, 0);  //manage comments with shared memory  //posso creare la key dello shmidcm con una operazione -> 30000+(t->id*MAX_COMMENTS)
  time_t date;
  time(&date);
  *(t->commentshead)=*(new_comment(0, "admin\n", date, "Please, comment below.\n",-1));  //First comment per topic!
  shmdt(t->commentshead);
  t->next=NULL;
  memset(t->subscribers, -1, MAX_REPLIES*sizeof(int));
  return t;
}

user* new_user(int id, char* username, char* password){
  user* u = (user*) malloc(sizeof(user));
  u->id=id;

  strncpy(u->username, username, 32);
  u->username[strlen(u->username)]='\0';  // stringa troncata se troppo lunga

  strncpy(u->password, password, 32);
  u->password[strlen(u->password)]='\0';  // stringa troncata se troppo lunga

  u->next=NULL;
  return u;
}






// addition
void append_topic(topic* head, topic* t){
  if(head->next==NULL){
    *(head+1)=*t;
    // free(t);
    head->next=head+1;
  }
  else return append_topic(head->next, t);
}

void add_topic(whiteboard* w, topic* t){
  append_topic(w->topicshead, t);
}

void append_user(user* head, user* u){
  if(head->next==NULL){
    *(head+1)=*u;
    // free(u);
    head->next=head+1;
  }
  else return append_user(head->next, u);
}

void add_user(whiteboard* w, user* u){
  append_user(w->usershead, u);
  return;
}

void append_comment(comment* head, comment* c){
  if(head->next==NULL){
    *(head+1)=*c;
    // free(c);
    head->next=head+1;
  }
  else return append_comment(head->next, c);
}

void push_comment(topic* t, comment* c){
  append_comment(t->commentshead, c);
}

void add_subscriber(topic* t, int userid){
  int i;
  for(i=0;i<MAX_SUBSCRIBERS;i++){
    if(t->subscribers[i]==-1){
      t->subscribers[i]=userid;
      break;
    }
  }
}


void add_reply(comment* r,int id_comm){
  int i;
  for(i=0;i<MAX_REPLIES;i++){
    if(r->replies[i]==-1){
      r->replies[i]=id_comm;
      break;
    }
  }
  //print_replies(r);
}







// deletion
void del_tp(topic* head, int id_tp){
  if(head->next==NULL){
    return;
  }
  if((head->next)->id==id_tp){
    topic* tmp=head->next;
    head->next=tmp->next;
  }
  else return del_tp(head->next, id_tp);
}


void delete_topic(whiteboard* w, int id_tp){
  del_tp(w->topicshead, id_tp);
}


void del_cm(comment* head, int id_cm){
  if(head->next==NULL){
    return;
  }
  if((head->next)->id==id_cm){
    comment* tmp=head->next;
    head->next=tmp->next;
  }
  else return del_cm(head->next, id_cm);
}

void delete_comment(topic* t, int id_cm){
  del_cm(t->commentshead, id_cm);
}









// getters
topic* find_topic(topic* head, int id_topic){
  if(head->id == id_topic) return head;
  if (head->next==NULL) return NULL;
  return find_topic(head->next, id_topic);
}

topic* get_topic(whiteboard* w, int id_topic){
  return find_topic(w->topicshead, id_topic);
}

topic* get_last_t(topic* head){
  if (head->next==NULL) return head;
  return get_last_t(head->next);
}

topic* get_last_topic(whiteboard* w){
  return get_last_t(w->topicshead);
}


user* find_user(user* head, int id_user){
  if(head->id == id_user) return head;
  if (head->next==NULL) return NULL;
  return find_user(head->next, id_user);
}

user* get_user(whiteboard* w, int id_user){
  return find_user(w->usershead, id_user);
}


user* find_user_by_usname(user* head, char* username){
  //printf("curr_us: %s  -  u->usn:%s\n",username,head->username);     //DEBUG

  if(!strcmp(head->username, username)){
    return head;
  }
  if (head->next==NULL) return NULL;
  return find_user_by_usname(head->next, username);
}

user* get_user_by_usname(whiteboard* w, char* username){
  return find_user_by_usname(w->usershead, username);
}

user* get_last_u(user* head){
  if (head->next==NULL) return head;
  return get_last_u(head->next);
}

user* get_last_user(whiteboard* w){
  return get_last_u(w->usershead);
}


comment* find_comment(comment* head, int id_comment){
  if(head->id == id_comment) return head;
  if (head->next==NULL) return NULL;
  return find_comment(head->next, id_comment);
}

comment* get_comment(topic* t, int id_comment){
  return find_comment(t->commentshead, id_comment);
}

comment* get_last_c(comment* head){
  if (head->next==NULL) return head;
  return get_last_c(head->next);
}

comment* get_last_comment(topic* t){
  return get_last_c(t->commentshead);
}








// printers
void here_all_topics(topic* head){
  printf("%d. ", head->id);
  printf("%s\n", head->title);
  printf("    by %s\n\n", head->author);
  if(head->next==NULL) return;
  here_all_topics(head->next);
}

void print_wb(whiteboard* w){
  printf("Here the whiteboard with all topics:\n\n");
  here_all_topics(w->topicshead);
}

void here_all_comments(comment* head){
  printf("%d. ", head->id);
  printf("%s\n", head->comm);
  printf("by %s\n\n", head->author);
  if(head->next==NULL) return;
  here_all_comments(head->next);
}

void print_tp(topic* t){
  printf("Here the topic you choose:\n\n");
  printf("%d. ", t->id);
  printf("%s\n", t->title);
  printf("%s\n", t->content);
  printf("by %s\n", t->author);
  here_all_comments(t->commentshead);
}


void here_all_users(user* head){  //DEBUG
  printf("%s, ", head->username);
  if(head->next==NULL){
    printf("\n");
    return;
  }
  here_all_users(head->next);
}

void print_users(whiteboard* w){  //DEBUG
    printf("USERS: ");
    here_all_users(w->usershead);

}

void print_replies(comment* r){
  int i;
  printf("replies: ");
  for(i=0;i<MAX_REPLIES && r->replies[i]!=-1;i++){
    printf("%d ",r->replies[i]);
  }
  printf("\n");
}

void print_arr(int* arr){
  int i;
  printf("arr: ");
  for(i=0;i<MAX_REPLIES && arr[i]!=-1;i++){
    printf("%d ",arr[i]);
  }
  printf("\n");
}




// to string
char* here_all_topics_to_string(topic* head, char* buf){
  int len=strlen(buf);
  len+=sprintf (buf+len, "%d. ",head->id);
  len+=sprintf(buf+len,"%s\n", head->title);
  len+=sprintf(buf+len,"    by %s\n\n", head->author);
  if(head->next==NULL){
    return buf;
  }
  return here_all_topics_to_string(head->next, buf);
}

char* wb_to_string(whiteboard* w){
  char buf[32768];
  strcpy(buf, "Here is the whiteboard with all topics:\n\n");
  return here_all_topics_to_string(w->topicshead, buf);
  
}


char* here_all_comments_to_string(topic* t, comment* head, char* buf, int* done){

  buf=cm_to_string(t, head, buf, done);
  // add status? 
  if(head->next==NULL){
    return buf;
  }
  return here_all_comments_to_string(t,head->next, buf, done);
}

char* tp_to_string(topic* t){
  char buf[32768];
  strcpy(buf, "Here is the chosen topic with all his comments:\n\n");
  int len=strlen(buf);
  len+=sprintf (buf+len, "%d. ",t->id);
  len+=sprintf(buf+len,"%s\n", t->title);
  len+=sprintf(buf+len,"    by %s\n\n", t->author);
  int done[MAX_COMMENTS];
  memset(done, -1, MAX_COMMENTS*sizeof(int));

  return here_all_comments_to_string(t,t->commentshead, buf, done);
}


char* child_to_string(topic* t, comment* child, char* buf, int* done, int level){
  int len=strlen(buf);

  int i;
  for(i=0;i<level;i++){
    len+=sprintf (buf+len, "\t|");   //adds tabs
  }

  len+=sprintf (buf+len, "\t%d. ",child->id);
  len+=sprintf(buf+len,"%s\n", child->comm);
  for(i=0;i<level;i++){
    len+=sprintf (buf+len, "\t|");   //adds tabs
  }

  len+=sprintf(buf+len,"\t    by %s\n\n", child->author);
  // add status? 
  // for child_to_string();
  done = add_to_arr(done, child->id);

  for(i=0;i<MAX_REPLIES && child->replies[i]!=-1;i++){
    child_to_string(t, get_comment(t,child->replies[i]), buf, done, level+1);
  }


  return buf;
}

char* cm_to_string(topic* t, comment* head, char* buf, int* done){
  if(int_in_arr(done, head->id)==1) return buf;

  int len=strlen(buf);
  len+=sprintf (buf+len, "\t%d. ",head->id);
  len+=sprintf(buf+len,"%s\n", head->comm);
  len+=sprintf(buf+len,"\t    by %s\n\n", head->author);
  // add status? 
  // for child_to_string();
  //print_arr(done);    //DEBUG
  done = add_to_arr(done, head->id);
  //print_arr(done);    //DEBUG


  int i;
  for(i=0;i<MAX_REPLIES && head->replies[i]!=-1;i++){
    child_to_string(t,get_comment(t,head->replies[i]),buf, done,1);
  }

  return buf;
}


















// authenticator
int validate_user(whiteboard*w, char* us, char* pw){
  printf("%s\n", us);     //DEBUG
  user* u=get_user_by_usname(w, us);
  if(u && !strcmp(u->password, pw)) return 0;
  return -1;
}


char* Auth(int shmidwb, int socket_desc){
  char* s="Please insert credentials.\nUsername: \0";
  send(socket_desc, s, 40, 0);

  // attach to shared memory
  whiteboard* w = shmat(shmidwb, NULL, 0);
  w->usershead=(user*) shmat(w->shmidus, NULL, 0);

  char* username=(char*)malloc(32*sizeof(char));
  size_t b_len = 32;
  recv(socket_desc, username, b_len, 0);
  replace_char(username, '\n', '\0');
  s="Password: \0";
  send(socket_desc, s, 13, 0);

  char* password=(char*)malloc(32*sizeof(char));
  recv(socket_desc, password, b_len, 0);    // gestire gli errori di TUTTE le recv
  replace_char(password, '\n', '\0');

  int ret=validate_user(w, username, password);
  print_users(w);     //DEBUG
  if(ret==0){
    printf("%s\n",username);     //DEBUG
    shmdt(w->usershead);
    shmdt(w);
    printf("Authentication successful.\n");
    return username;
  }
  shmdt(w->usershead);
  shmdt(w);
  return NULL;
}


char* Register(int shmidwb, int socket_desc){
  char* s="Please insert credentials.\nUsername: \0";
  send(socket_desc, s, 40, 0);

  // attach to shared memory
  whiteboard* w = shmat(shmidwb, NULL, 0);
  w->usershead=(user*) shmat(w->shmidus, NULL, 0);


  char* username=(char*)malloc(sizeof(char)*32);
  size_t b_len = 32;
  recv(socket_desc, username, b_len, 0);
  replace_char(username, '\n', '\0');
  char end='\0';    //????
  strncat(username, &end, 1);
  if(!strcmp(username, "")){
    printf("You didn't insert anything.\n");
    return NULL;
  }
  printf("Username: %s\n", username);

  s="Password: \0";
  send(socket_desc, s, 13, 0);

  char* password=(char*)malloc(sizeof(char)*32);
  recv(socket_desc, password, b_len, 0);
  replace_char(password, '\n', '\0');
  strncat(password, &end, 1);
  printf("Password: %s\n", password);
  // printf("lens: %d - %d\n", strlen(username) ,strlen(password));   // DEBUG


  user* us=get_user_by_usname(w, username);
  if(us==NULL){
    user* last=get_last_user(w);
    user* u;
    print_users(w);     //DEBUG
    if(last==NULL){
      u=new_user(0, username, password);
    }
    else{
      u=new_user(last->id + 1, username, password);
    }
    add_user(w, u); // check return?
    printf("Registration done.\n");
    print_users(w);     //DEBUG
    free(username);
    free(password);
    shmdt(w->usershead);
    shmdt(w);
    return u->username;
  }
  printf("Username already taken.\n");
  shmdt(w->usershead);
  shmdt(w);
  //free(username);
  //free(password);
  return NULL;
}






// string management
char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}






// utils
int int_in_arr(int* arr, int i){
  //print_arr(arr);     //DEBUG
  int j;
  for(j=0;arr[j]!=-1;j++){
    //printf("%d -- %d",arr[j],i);     //DEBUG
    if(arr[j]==i) return 1;
  }
  return 0;
}

int* add_to_arr(int* arr, int i){
  int j;
  for(j=0;;j++){
    if(arr[j]==-1){
      arr[j]=i;
      
      break;
    }
  }
  return arr;
}




/*
void main(){    // WARNING: no shared memory
  whiteboard* w = create_wb();
  time_t date;
  time(&date);
  topic* t = new_topic(0, "me", "hello", "hello world!", date);
  add_topic(w, t);
  comment* c = new_comment(0, "me", date, "this is a comment",-1);
  push_comment(t, c);
  print_wb(w);
  return;
}
*/
