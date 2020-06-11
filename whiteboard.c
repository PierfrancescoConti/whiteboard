#include "whiteboard.h"

// creation
whiteboard *create_wb(whiteboard *w) {
  // users
  if ((w->shmidus = shmget(USERS_KEY, sizeof(user) * MAX_USERS, IPC_CREAT | 0666)) < 0) {
    perror("shmget");
    exit(1);
  }

  // topics
  if ((w->shmidto = shmget(TOPICS_KEY, sizeof(topic) * MAX_TOPICS, IPC_CREAT | 0666)) < 0) {
    perror("shmget");
    exit(1);
  }
  w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
  *(w->topicshead) =
      *(new_topic(0, "admin\0", "First topic.\n",
                  "Hello, world! This is my first topic.\n"));
  shmdt(w->topicshead);
  w->usershead = (user *)shmat(w->shmidus, NULL, 0);
  *(w->usershead) = *(new_user(0, "admin\0", "admin\0"));
  shmdt(w->usershead);
  w->pool->userid = 0;
  memset(w->pool->list_subid, -1, MAX_SUBSCRIBERS * sizeof(int));
  w->pool->list_subid[0] = 0;
  w->pool->next = NULL;
  return w;
}

comment *new_comment(int id, char *author, time_t timestamp, char *comm,
                     int reply_id) {
  comment *c = (comment *)malloc(sizeof(comment));  //freed after addition
  MALLOC_ERROR_HELPER(c, "Malloc Error.");
  c->id = id;
  c->timestamp = timestamp;
  strcpy(c->status, "O\0"); // Sent

  strncpy(c->author, author, 32);

  strncpy(c->comm, comm, 1024);

  c->next = NULL;
  c->in_reply_to = reply_id;

  memset(c->replies, -1, MAX_REPLIES * sizeof(int));
  memset(c->seen, -1, MAX_SUBSCRIBERS * sizeof(int));
  return c;
}

linkt *new_link(int id, int topic_id, int thread_id) {
  linkt *l = (linkt *)malloc(sizeof(linkt));  //freed after addition
  MALLOC_ERROR_HELPER(l, "Malloc Error.");
  l->id = id;
  l->topic_id = topic_id;
  l->thread_id = thread_id;

  return l;
}

topic *new_topic(int id, char *author, char *title, char *content) {
  topic *t = (topic *)malloc(sizeof(topic));  //freed after addition
  MALLOC_ERROR_HELPER(t, "Malloc Error.");
  t->id = id;
  strncpy(t->author, author, 32);

  strncpy(t->title, title, 256);

  strncpy(t->content, content, 1024);

  time_t date;
  time(&date);
  *(t->commentshead) = *(new_comment(0, "admin\0", date, "Please, comment below.\n",
                    -1)); // First comment is mine!
  t->next = NULL;
  memset(t->subscribers, -1, MAX_REPLIES * sizeof(int));
  memset(t->viewers, -1, MAX_REPLIES * sizeof(int));
  return t;
}

user *new_user(int id, char *username, char *password) {
  user *u = (user *)malloc(sizeof(user));  //freed after addition
  MALLOC_ERROR_HELPER(u, "Malloc Error.");
  u->id = id;

  strncpy(u->username, username, 31);
  strncpy(u->password, password, 31);

  u->next = NULL;
  return u;
}

subscribers_pool *new_entry_pool(int uid) {
  subscribers_pool *p = (subscribers_pool *)malloc(sizeof(subscribers_pool));   //freed after addition
  MALLOC_ERROR_HELPER(p, "Malloc Error.");
  p->userid = uid;
  memset(p->list_subid, -1, MAX_SUBSCRIBERS * sizeof(int));
  p->next = NULL;
  return p;
}

// addition
void append_topic(topic *head, topic *t) {
  if (head->next == NULL) {
    *(head + 1) = *t;
    free(t);
    head->next = head + 1;
  } else
    return append_topic(head->next, t);
}

void add_topic(whiteboard *w, topic *t) { append_topic(w->topicshead, t); }

void append_user(user *head, user *u) {
  if (head->next == NULL) {
    *(head + 1) = *u;
    //free(u);
    head->next = head + 1;
  } else
    return append_user(head->next, u);
}

void add_user(whiteboard *w, user *u) {
  add_user_to_pool(w, u->id);
  // print_pool(w);
  append_user(w->usershead, u);
  return;
}

void append_comment(comment *head, comment *c) {
  if (head->next == NULL) {
    strcpy(c->status, "K\0");
    *(head + 1) = *c;
    //free(c);  //can't free
    head->next = head + 1;
  } else
    return append_comment(head->next, c);
}

void push_comment(topic *t, comment *c) { append_comment(t->commentshead, c); }

void append_link(linkt *head, linkt *l) {
  if (head->next == NULL) {
    *(head + 1) = *l;
    //free(l);  //can't free
    head->next = head + 1;
  } else
    return append_link(head->next, l);
}

void add_link(topic *t, linkt *l) { 
  if(l->id >= 128 || l->id < 0){
    return;
  }
  return append_link(t->linkshead, l); 
}

void add_subscriber(topic *t, int userid) {
  if (int_in_arr(t->subscribers, userid))
    return;
  int i;
  for (i = 0; i < MAX_SUBSCRIBERS; i++) {
    if (t->subscribers[i] == -1) {
      t->subscribers[i] = userid;
      break;
    }
  }
}

void add_viewer(topic *t, int uid) {
  if (int_in_arr(t->viewers, uid))
    return;
  add_to_arr(t->viewers, uid, MAX_SUBSCRIBERS);

}

void add_reply(comment *r, int id_comm) {
  if (int_in_arr(r->replies, id_comm))
    return;
  add_to_arr(r->replies, id_comm, MAX_REPLIES);

}

void add_seen(comment *c, int uid) {
  if (int_in_arr(c->seen, uid))
    return;
  add_to_arr(c->seen, uid, MAX_SUBSCRIBERS);

}

void add_all_seen(comment *head, int uid) {
  if (!int_in_arr(head->seen, uid))
    add_seen(head, uid);
  if (head->next == NULL) {
    return;
  }
  add_all_seen(head->next, uid);
}

void autp(subscribers_pool *head, int uid) {
  
  if (uid >= MAX_USERS) {
    printf("Too much users.\n");
    return;
  }
  if (head->next == NULL) {
    subscribers_pool *p = new_entry_pool(uid);
    *(head + 1) = *p;
    head->next = head + 1;
    free(p);
    return;
  }
  autp(head->next, uid);
}

void add_user_to_pool(whiteboard *w, int uid) { autp(w->pool, uid); }

void add_subscription_entry(whiteboard *w, int uid, int tid) {
  int i;
  for (i = 0; i < MAX_USERS; i++) {
    if ((w->pool + i)->userid == uid) {
      if (int_in_arr((w->pool + i)->list_subid, tid))
        return;
      int j;
      for (j = 0; j < MAX_SUBSCRIBERS; j++) {
        if ((w->pool + i)->list_subid[j] == -1) {
          (w->pool + i)->list_subid[j] = tid;
          return;
        }
      }
    }
  }
}

// deletion
void del_us(user *head, int id_us) {
  if (head->next == NULL) {
    return;
  }
  if ((head->next)->id == id_us) {
    user *tmp = head->next;
    head->next = tmp->next;
  } else
    return del_us(head->next, id_us);
}

void delete_user(whiteboard *w, int id_us) { del_us(w->usershead, id_us); }

void del_tp(topic *head, int id_tp) {
  if (head->next == NULL) {
    return;
  }
  if ((head->next)->id == id_tp) {
    topic *tmp = head->next;
    head->next = tmp->next;
  } else
    return del_tp(head->next, id_tp);
}

void delete_topic(whiteboard *w, int id_tp) { del_tp(w->topicshead, id_tp); }

void del_cm(comment *head, int id_cm) {
  if (head->next == NULL) {
    return;
  }
  if ((head->next)->id == id_cm) {
    comment *tmp = head->next;
    head->next = tmp->next;
  } else
    return del_cm(head->next, id_cm);
}

void delete_comment(topic *t, int id_cm) { del_cm(t->commentshead, id_cm); }

// getters
topic *find_topic(topic *head, int id_topic) {
  if (head->id == id_topic)
    return head;
  if (head->next == NULL)
    return NULL;
  return find_topic(head->next, id_topic);
}

topic *get_topic(whiteboard *w, int id_topic) {
  return find_topic(w->topicshead, id_topic);
}

topic *get_last_t(topic *head) {
  if (head->next == NULL)
    return head;
  return get_last_t(head->next);
}

topic *get_last_topic(whiteboard *w) { return get_last_t(w->topicshead); }

user *find_user(user *head, int id_user) {
  if (head->id == id_user)
    return head;
  if (head->next == NULL)
    return NULL;
  return find_user(head->next, id_user);
}

user *get_user(whiteboard *w, int id_user) {
  return find_user(w->usershead, id_user);
}

user *find_user_by_usname(user *head, char *username) {

  if (!strcmp(head->username, username)) {
    return head;
  }
  if (head->next == NULL)
    return NULL;
  return find_user_by_usname(head->next, username);
}

user *get_user_by_usname(whiteboard *w, char *username) {
  return find_user_by_usname(w->usershead, username);
}

user *get_last_u(user *head) {
  if (head->next == NULL)
    return head;
  return get_last_u(head->next);
}

user *get_last_user(whiteboard *w) { return get_last_u(w->usershead); }

comment *find_comment(comment *head, int id_comment) {
  if (head->id == id_comment)
    return head;
  if (head->next == NULL)
    return NULL;
  return find_comment(head->next, id_comment);
}

comment *get_comment(topic *t, int id_comment) {
  return find_comment(t->commentshead, id_comment);
}

comment *get_last_c(comment *head) {
  if (head->next == NULL)
    return head;
  return get_last_c(head->next);
}

comment *get_last_comment(topic *t) { return get_last_c(t->commentshead); }

int *find_list_from_pool(subscribers_pool *head, int uid) {
  if (head->userid == uid)
    return head->list_subid;
  if (head->next == NULL) {
    return NULL;
  }
  return find_list_from_pool(head->next, uid);
}

int *get_list_from_pool(whiteboard *w, int uid) {
  return find_list_from_pool(w->pool, uid);
}

// printers
void here_all_topics(topic *head) {
  printf("%d. ", head->id);
  printf("%s\n", head->title);
  printf("    by %s\n\n", head->author);
  if (head->next == NULL)
    return;
  here_all_topics(head->next);
}

void print_wb(whiteboard *w) {
  printf("Here the whiteboard with all topics:\n\n");
  here_all_topics(w->topicshead);
}

void here_all_comments(comment *head) {
  printf("%d. ", head->id);
  printf("%s\n", head->comm);
  printf("by %s\n\n", head->author);
  if (head->next == NULL)
    return;
  here_all_comments(head->next);
}

void print_tp(topic *t) {
  printf("Here the topic you choose:\n\n");
  printf("%d. ", t->id);
  printf("%s\n", t->title);
  printf("%s\n", t->content);
  printf("by %s\n", t->author);
  here_all_comments(t->commentshead);
}

void here_all_users(user *head) { // DEBUG
  printf("%s, ", head->username);
  if (head->next == NULL) {
    printf("\n");
    return;
  }
  here_all_users(head->next);
}

void print_users(whiteboard *w) { // DEBUG
  printf("USERS: ");
  here_all_users(w->usershead);
}

void print_replies(comment *r) {
  int i;
  printf("replies: ");
  for (i = 0; i < MAX_REPLIES && r->replies[i] != -1; i++) {
    printf("%d ", r->replies[i]);
  }
  printf("\n");
}

void print_arr(int *arr) {
  int i;
  printf("arr: ");
  for (i = 0; i < MAX_REPLIES && arr[i] != -1; i++) {
    printf("%d ", arr[i]);
  }
  printf("\n");
}

void print_pool(whiteboard *w) {
  int i, j;
  for (i = 0; i < MAX_USERS; i++) {
    printf("Utente: %d\n", (w->pool + i)->userid);
    printf("Topics:");
    for (j = 0; j < MAX_SUBSCRIBERS && (w->pool + i)->list_subid[j] != -1;
         j++) {
      printf(" %d", (w->pool + i)->list_subid[j]);
    }
    if ((w->pool + i)->next == NULL) {
      break;
    }
  }
}

// to string
char *here_all_topics_to_string(topic *head, char *buf) {
  int len = strlen(buf);
  len += sprintf(buf + len, "\033[33;1m%d\033[0m. ", head->id);
  len += sprintf(buf + len, "\033[1m%s\033[0m\n", head->title);
  len += sprintf(buf + len, "\t    by %s\n\n", head->author);
  strcat(buf, "----------------------------------------------------------------"
              "----------------------------------------\n");
  if (head->next == NULL) {
    return buf;
  }
  return here_all_topics_to_string(head->next, buf);
}

char *wb_to_string(whiteboard *w) {
  char buf[32768];
  strcpy(buf,
         "\n\033[44;1m   Here is the whiteboard with all topics:               "
         "                                               \033[0m\n\n");
  return here_all_topics_to_string(w->topicshead, buf);
}

char *here_all_users_to_string(user *head, char *buf) {
  int len = strlen(buf);
  len += sprintf(buf + len, "\033[33;1m%d\033[0m. ", head->id);
  len += sprintf(buf + len, "\033[1m%s\033[0m\n", head->username);
  if (head->next == NULL) {
    return buf;
  }
  return here_all_users_to_string(head->next, buf);
}

char *us_to_string(whiteboard *w) {
  char buf[32768];
  strcpy(buf,
         "\n\033[44;1m   Users:                                                "
         "                                               \033[0m\n\n");
  return here_all_users_to_string(w->usershead, buf);
}

char *here_all_comments_to_string(topic *t, comment *head, char *buf, int *done,
                                  int subscribed) {
  if (subscribed == 0) {
    strcat(buf,
           "\033[41;1m   Wait! To see comments, you must subscribe to this "
           "topic! (usage: subscribe)                          \033[0m");
    return buf;
  }
  buf = cm_to_string(t, head, buf, done);
  // add status?
  
  if (head->next == NULL) {
    return buf;
  }
  return here_all_comments_to_string(t, head->next, buf, done, subscribed);
}

char *tp_to_string(topic *t, int subscribed) {
  char buf[32768];
  strcpy(buf,
         "\n\033[44;1m   Here is the chosen topic with all his comments:       "
         "                                               \033[0m\n\n");
  int len = strlen(buf);
  len += sprintf(buf + len, "\033[33;1m%d\033[0m. ", t->id);
  len += sprintf(buf + len, "\033[1m%s\033[0m\n", t->title);
  len += sprintf(buf + len, "%s\n", t->content);
  len += sprintf(buf + len, "\t    by %s\n\n", t->author);
  strcat(buf, "----------------------------------------------------------------"
              "----------------------------------------\n");

  int done[MAX_COMMENTS];
  memset(done, -1, MAX_COMMENTS * sizeof(int));

  return here_all_comments_to_string(t, t->commentshead, buf, done, subscribed);
}

char *child_to_string(topic *t, comment *child, char *buf, int *done,
                      int level) {
  int len = strlen(buf);

  int i;
  for (i = 0; i < level; i++) {
    len += sprintf(buf + len, "\t|"); // adds tabs
  }

  len += sprintf(buf + len, "\t\033[33;1m%d\033[0m. ", child->id);
  len += sprintf(buf + len, "\033[1m%s\033[0m", child->comm);
  for (i = 0; i < level; i++) {
    len += sprintf(buf + len, "\t|"); // adds tabs
  }
  len += sprintf(buf + len, "\n");
  for (i = 0; i < level; i++) {
    len += sprintf(buf + len, "\t|"); // adds tabs
  }

  len += sprintf(buf + len, "\t\t    by %s", child->author);
  len += sprintf(buf + len, "\t\033[1;34m%s\033[0m\n", child->status);
  for (i = 0; i < level; i++) {
    len += sprintf(buf + len, "\t|"); // adds tabs
  }
  len += sprintf(buf + len, "\n");
  strcat(buf, "----------------------------------------------------------------"
              "----------------------------------------\n");
  // add status?
  // for child_to_string();
  done = add_to_arr(done, child->id, MAX_COMMENTS);

  for (i = 0; i < MAX_REPLIES && child->replies[i] != -1; i++) {
    child_to_string(t, get_comment(t, child->replies[i]), buf, done, level + 1);
  }

  return buf;
}

char *cm_to_string(topic *t, comment *head, char *buf, int *done) {
  if (int_in_arr(done, head->id) == 1)
    return buf;

  int len = strlen(buf);
  len += sprintf(buf + len, "\t\033[33;1m%d\033[0m. ", head->id);
  len += sprintf(buf + len, "\033[1m%s\033[0m\n", head->comm);
  len += sprintf(buf + len, "\t\t    by %s", head->author);
  len += sprintf(buf + len, "\t\033[1;34m%s\033[0m\n\n", head->status);
  strcat(buf, "----------------------------------------------------------------"
              "----------------------------------------\n");

  done = add_to_arr(done, head->id, MAX_COMMENTS);

  int i;
  for (i = 0; i < MAX_REPLIES && head->replies[i] != -1; i++) {
    child_to_string(t, get_comment(t, head->replies[i]), buf, done, 1);
  }

  return buf;
}

linkt *find_link(linkt *head, int id) {
  if (head->id == id)
    return head;
  if (head->next == NULL)
    return NULL;
  return find_link(head->next, id);
}

char *ln_to_string(whiteboard *w, topic *t, int id, char *buf) {
  linkt *l = find_link(t->linkshead, id);
  if (l == NULL) {
    strcpy(buf,
           "\033[41;1m   Invalid link                                          "
           "                                               \033[0m\0");
  } else {
    topic *gt = get_topic(w, l->topic_id);
    comment *gc = get_comment(gt, l->thread_id);
    int done[MAX_COMMENTS];
    memset(done, -1, MAX_COMMENTS * sizeof(int));
    buf = cm_to_string(gt, gc, buf, done);
  }
  return buf;
}














// authenticator
int validate_user(whiteboard *w, char *us, char *pw) {
  user *u = get_user_by_usname(w, us);
  if (u && !strcmp(u->password, pw))
    return 0;
  return -1;
}

char *Auth(whiteboard* w, int socket_desc, int mutex) {
  char *s = "Please insert credentials.\nUsername: \0";
  send(socket_desc, s, 40, 0);

  char *username = (char *)malloc(32 * sizeof(char));
  MALLOC_ERROR_HELPER(username, "Malloc Error.");
  size_t b_len = 32;

  memset(username, 0, b_len); // FLUSH
  int ret=recv(socket_desc, username, b_len, 0);
  ERROR_HELPER(ret, "Recv Error");

  replace_char(username, '\n', '\0');
  if (!strcmp(username, "")) {
    exit(1);
  }
  s = "Password: \0";
  send(socket_desc, s, 13, 0);

  char *password = (char *)malloc(32 * sizeof(char));
  MALLOC_ERROR_HELPER(password, "Malloc Error.");
  memset(password, 0, b_len); // FLUSH
  ret=recv(socket_desc, password, b_len, 0);
  ERROR_HELPER(ret, "Recv Error");

  replace_char(password, '\n', '\0');
  if (!strcmp(password, "")) {
    exit(1);
  }

  // attach to shared memory
  ret = Pwait(mutex);
  ERROR_HELPER(ret, "Pwait Error");

  ret = validate_user(w, username, password);
  if (ret == 0) {
    printf("\n\033[32mAuthenticated user: \033[33;1m%s\033[0m\033[32m.\033[0m\n", username);
    ret = Vpost(mutex);
    ERROR_HELPER(ret, "Vpost Error");
    
    return username;
  }
  ret = Vpost(mutex);
  ERROR_HELPER(ret, "Vpost Error");
  return NULL;
}

char *Register(whiteboard* w, int socket_desc, int mutex) {
  char *s = "Please insert credentials.\nUsername: \0";
  send(socket_desc, s, 40, 0);

  char *username = (char *)malloc(sizeof(char) * 32);
  MALLOC_ERROR_HELPER(username, "Malloc Error.");
  size_t b_len = 32;

  memset(username, 0, b_len); // FLUSH
  
  int ret=recv(socket_desc, username, b_len, 0);
  ERROR_HELPER(ret, "Recv Error");

  replace_char(username, '\n', '\0');
  if (!strcmp(username, "")) {
    exit(1);
  }

  s = "Password: \0";
  send(socket_desc, s, 13, 0);
  char *password = (char *)malloc(sizeof(char) * 32);
  MALLOC_ERROR_HELPER(password, "Malloc Error.");

  memset(password, 0, b_len); // FLUSH
  ret=recv(socket_desc, password, b_len, 0);
  ERROR_HELPER(ret, "Recv Error");

  replace_char(password, '\n', '\0');
  if (!strcmp(password, "")) {
    exit(1);
  }

  // attach to shared memory
  ret = Pwait(mutex);
  ERROR_HELPER(ret, "Pwait Error");

  user *us = get_user_by_usname(w, username);
  if (us == NULL) {
    user *last = get_last_user(w);
    user *u;
    if (last == NULL) {
      u = new_user(0, username, password);
    } else if(last->id>=MAX_USERS-1){
      free(username);
      free(password);
      ret = Vpost(mutex);
      ERROR_HELPER(ret, "Vpost Error");
      return "Too many!";
    } else {
      u = new_user(last->id + 1, username, password);
    }
    add_user(w, u); // check return?
    //printf("\033[32mRegistered user: \033[33;1m%s\033[0m\033[32m.\033[0m\n", username);
    free(username);
    free(password);
    ret = Vpost(mutex);
    ERROR_HELPER(ret, "Vpost Error");
    return u->username;
  }
  ret = Vpost(mutex);
  ERROR_HELPER(ret, "Vpost Error");
  free(username);
  free(password);
  return "NULL";
}









// string management
char *replace_char(char *str, char find, char replace) {
  char *current_pos = strchr(str, find);
  while (current_pos) {
    *current_pos = replace;
    current_pos = strchr(current_pos, find);
  }
  return str;
}




// utils
int int_in_arr(int *arr, int i) {
  int j;
  for (j = 0; arr[j] != -1; j++) {
    if (arr[j] == i)
      return 1;
  }
  return 0;
}

int *add_to_arr(int *arr, int i, int max_size) {
  int j;
  for (j = 0; j < max_size; j++) {
    if (arr[j] == -1) {
      arr[j] = i;

      break;
    }
  }
  return arr;
}

int get_digit(char *buf, int i) { // i is the num's starting index
  char d = 'a';
  d = buf[i];
  if (!isdigit(d))
    return -1;
  int number = 0;
  while (d != '\0') {
    d = buf[i];
    if (!isdigit(d))
      break;

    int digit = d - '0';
    number = number * 10 + digit;
    i++;
  }
  return number;
}




// checks
int check_seen_by_all(int *subscribers, int *seen) {
  int i;
  for (i = 0; i < MAX_SUBSCRIBERS; i++) {
    if (subscribers[i] == -1)
      break;
    if (!int_in_arr(seen, subscribers[i])) {
      return 0;
    }
  }
  return 1;
}

void check_all_seen_by_all(int *subscribers, comment *head) {
  if (check_seen_by_all(subscribers, head->seen)) {
    strcpy(head->status, "KK\0");
  } else
    return;
  if (head->next == NULL) {
    return;
  }
  check_all_seen_by_all(subscribers, head->next);
}









// semaphores
int initsem(key_t semkey, int size) {
  int status = 0, semid;
  union semun { /* this has to be declared */
    int val;
    struct semid_ds *stat;
    ushort *array;
  } ctl_arg;

  if ((semid = semget(semkey, 1, IPC_CREAT|IPC_EXCL|0666)) > 0) {
    ctl_arg.val = size; /* semctl should be called with */
    status = semctl(semid, 0, SETVAL, ctl_arg);
  }

  if (semid < 0 || status < 0) {
    return (-1);
  } else
    return (semid);
}

void semclean(key_t semkey){
  int semid;
  union semun {
    int val;
    struct semid_ds *stat;
  } ctl_arg;

  if ((semid = semget(semkey, 1, IPC_CREAT|0666)) > 0) {
    ctl_arg.val = 1;
    semctl(semid, 0, SETVAL, ctl_arg);
  }

  semctl(semid, 0, IPC_RMID); // CLEANS existing semaphore 
}

int Pwait(int semid) {

  struct sembuf p_buf;

  p_buf.sem_num = 0;
  p_buf.sem_op = -1;
  p_buf.sem_flg = 0;

  if (semop(semid, &p_buf, 1) == -1) {
    perror("Pwait(semid) failed");
    return (-1);
  } else {
    return (0);
  }
}

int Vpost(int semid) {

  struct sembuf v_buf;

  v_buf.sem_num = 0;
  v_buf.sem_op = 1;
  v_buf.sem_flg = 0;

  if (semop(semid, &v_buf, 1) == -1) {
    perror("Vpost(semid) failed");
    return (-1);
  } else {
    return (0);
  }
}



// SAVE and LOAD
void write_arr(int *arr, FILE * file) {
  int i;
  for (i = 0; i < MAX_SUBSCRIBERS && arr[i] != -1; i++) {
    fprintf (file, "%d ", arr[i]);
  }
  fprintf (file, "\n");
}

void write_comments(comment *head, FILE * file){
  replace_char(head->status, '\n', '\0');
  replace_char(head->author, '\n', '\0');
  //replace_char(head->comm, '\n', '\0');
  fprintf (file, "%d\n%d\n%s\n%s\n%s",head->id,head->in_reply_to,head->status,head->author,head->comm);
  fprintf (file, "Replies: ");
  write_arr(head->replies,file);
  fprintf (file, "Seen: ");
  write_arr(head->seen,file);
  
  if (head->next == NULL) {
    printf("-");
    return;
  }
  write_comments(head->next, file);
}

void write_links(linkt *head, FILE * file) {
  fprintf (file, "%d\n%d\n%d\n\n",head->id,head->topic_id,head->thread_id);
  if (head->next == NULL) {
    printf("-");
    return;
  }
  write_links(head->next, file);
}

void write_topics(topic *head, FILE * file) {
  fprintf (file, "%d\n%s\n%s%s",head->id,head->author,head->title,head->content);
  fprintf (file, "Subscribers: ");
  write_arr(head->subscribers,file);
  fprintf (file, "Viewers: ");
  write_arr(head->viewers,file);
  
  // saving comments
  char name[32];
  sprintf(name, "saved_dumps/comments_dump%d", head->id);

  FILE * fcm= fopen(name, "wb");
  if (fcm != NULL) {
    fprintf (fcm, "%d\n",head->id);
    write_comments(head->commentshead, fcm);  //skip admin
    fclose(fcm);
  }
  //saving links
  char lname[32];
  sprintf(lname, "saved_dumps/links_dump%d", head->id);
  FILE * fln= fopen(lname, "wb");
  if (fln != NULL) {
    fprintf (fln, "%d\n\n",head->id);
    write_links(head->linkshead, fln);
    fclose(fln);
  }

  if (head->next == NULL) {
    printf("-");
    return;
  }
  
  write_topics(head->next, file);
}

void write_users(user *head, FILE * file) {
  //fwrite (head, sizeof(user), 1, file); 
  fprintf (file, "%d\n%s\n%s\n\n",head->id,head->username,head->password);
  if (head->next == NULL) {
    printf("-");
    return;
  }
  write_users(head->next, file);
}

void save_wb(whiteboard* w){

  FILE * fus= fopen("saved_dumps/users_dump", "wb");
  if (fus != NULL) {
    write_users(w->usershead, fus);  //skip admin
    fclose(fus);
  }

  FILE * fto= fopen("saved_dumps/topics_dump", "wb");
  if (fto != NULL) {
    write_topics(w->topicshead, fto);  //skip admin
    fclose(fto);
  }
}



int* read_arr(FILE* file, int* arr){
  char line[1024];
  memset(line, 0, 1024); // FLUSH
  memset(arr, -1, MAX_SUBSCRIBERS * sizeof(int)); // FLUSH
  fgets(line, sizeof(line), file);
  line[strlen(line)-1]='\0';
  int i; 
  for (i = 0; line[i] != '\0'; i++) { 
     if (line[i] == ' '){
        arr=add_to_arr(arr,get_digit(line, i+1),MAX_SUBSCRIBERS);
    } 
  } 
  return arr;

}

void read_comments(comment *head, FILE * file){
  int id, irt;
  char status[4], author[32], content[1024];
  while(!feof(file)){
    memset(status, 0, 4); // FLUSH
    memset(author, 0, 32); // FLUSH
    memset(content, 0, 1024); // FLUSH

    fscanf (file, "%d\n",&id);
    fscanf (file, "%d\n",&irt);
    fgets(status, sizeof(status), file);
    fgets(author, sizeof(author), file);
    fgets(content, sizeof(content), file);
    replace_char(status, '\n', '\0');
    replace_char(author, '\n', '\0');
    time_t date;  //TODO? naah
    time(&date);
    comment* c = new_comment(id, author, date, content, irt);
    strcpy(c->status,status);
    read_arr(file,c->replies);
    read_arr(file,c->seen);
    if(find_comment(head, c->id)==NULL) append_comment(head,c);
    
  }
}
void read_links(linkt *head, FILE * file){
  int id, top, thr;
  while(!feof(file)){
    fscanf(file, "%d\n%d\n%d\n\n",&id,&top,&thr);
    linkt* l=new_link(id,top,thr);
    append_link(head,l);
  }
}


void read_topics(topic *head, FILE * file){
  int id;

  char author[32], title[256], content[1024];
  
  while(!feof(file)){
    memset(author, 0, 32); // FLUSH
    memset(title, 0, 256); // FLUSH
    memset(content, 0, 1024); // FLUSH

    fscanf (file, "%d\n",&id);
    fgets(author, sizeof(author), file);
    if(!strcmp(author,"\n")) fgets(author, sizeof(author), file);
    fgets(title, sizeof(title), file);
    if(!strcmp(title,"\n")) fgets(title, sizeof(title), file);
    fgets(content, sizeof(content), file);
    if(!strcmp(content,"\n")) fgets(content, sizeof(content), file);
    topic* t = new_topic(id, author, title, content);
    read_arr(file,t->subscribers);
    read_arr(file,t->viewers);
    if(find_topic(head, t->id)==NULL) append_topic(head,t);
  }
  FILE* fcm;
  char filename[32];
  int topic_id;
  id=0;
  sprintf(filename, "saved_dumps/comments_dump0");
  for(id=0;id<MAX_TOPICS-1;){
    if(fcm = fopen(filename, "r")){
      fscanf (fcm, "%d\n",&topic_id);
      if(topic_id!=id) 
        printf("Beware! Topic %d is changing id into %d.", id, topic_id);    // DEBUG: we hope that this will never happen
      read_comments(find_topic(head,id)->commentshead,fcm);
      fclose(fcm);
      
    }
    id++;
    sprintf(filename, "saved_dumps/comments_dump%d",id);
  }

  FILE* fln;
  id=0;
  sprintf(filename, "saved_dumps/links_dump0");
  for(id=0;id<MAX_TOPICS-1;){
    if(fln = fopen(filename, "r")){
      fscanf (fln, "%d\n",&topic_id);
      if(topic_id!=id) 
        printf("Beware! Topic %d is changing id into %d.", id, topic_id);    // DEBUG: we hope that this will never happen
      read_links(find_topic(head,id)->linkshead,fln);
      fclose(fln);
      
    }
    id++;
    sprintf(filename, "saved_dumps/links_dump%d",id);
  }
}

void load_wb(whiteboard* w){
  FILE * fto= fopen("saved_dumps/topics_dump", "r");
  if (fto != NULL) {
    read_topics(w->topicshead, fto);  //skip admin
    
    fclose(fto);
  }
}






// encryption/decryption
void encrypt(char* filename){
  char command[128];
  sprintf(command,"gpg --yes --batch --passphrase=%s -c %s >/dev/null 2>/dev/null",SUPERSECRET_KEY, filename);
  system(command);
}
void decrypt(char* filename){   // filename should end with .gpg
  char command[128];
  sprintf(command,"gpg --yes --batch --passphrase=%s  %s >/dev/null 2>/dev/null",SUPERSECRET_KEY, filename);
  system(command);
}

void encryptall(char* folder){
  DIR *d;
  struct dirent *dir;
  char path[512];
  d = opendir(folder);
  if (d){
    while ((dir = readdir(d)) != NULL){
      memset(path, 0, 512);
      sprintf(path,"%s/%s", folder, dir->d_name);
      if(strstr(path, "/.")==NULL){

        encrypt(path);
      }
    }
    closedir(d);
 }
}

void decryptall(char* folder){
  DIR *d;
  struct dirent *dir;
  char path[512];
  d = opendir(folder);
  if (d){
    while ((dir = readdir(d)) != NULL){
      memset(path, 0, 512);
      sprintf(path,"%s/%s", folder, dir->d_name);
      if(strstr(path, "/.")==NULL){
        decrypt(path);
      }
    }
    closedir(d);
 }
}


void rmenc(char* folder){
  DIR *d;
  struct dirent *dir;
  char path[512];
  d = opendir(folder);
  if (d){
    while ((dir = readdir(d)) != NULL){
      memset(path, 0, 512);
      sprintf(path,"%s/%s", folder, dir->d_name);
      if(strstr(path, "/.")==NULL){
        if(strstr(path, ".gpg") != NULL) {
          remove(path);
        }
      }
    }
    closedir(d);
 }
}

void rmdec(char* folder){
  DIR *d;
  struct dirent *dir;
  char path[512];
  d = opendir(folder);
  if (d){
    while ((dir = readdir(d)) != NULL){
      memset(path, 0, 512);
      sprintf(path,"%s/%s", folder, dir->d_name);
      if(strstr(path, "/.")==NULL){
        if(strstr(path, ".gpg") == NULL) {
          remove(path);
        }
      }
    }
    closedir(d);
 }
}





