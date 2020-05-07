#include "whiteboard.h"

int get_digit(char *buf, int i) { // i is the num's starting index
  char d = 'a';
  d = buf[i];
  if (!isdigit(d))
    return -1;
  int number = 0;
  while (d != '\0') {
    d = buf[i];
    // printf("d: %c\n", d);  // DEBUG
    if (!isdigit(d))
      break;

    int digit = d - '0';
    number = number * 10 + digit;
    i++;
  }
  return number;
}

void notify(int shmidwb, int socket_desc, char *current_user, int mutex) {
  int ret, recv_bytes;
  char *buf = (char *)malloc(16 * sizeof(char));
  MALLOC_ERROR_HELPER(buf, "Malloc Error.");
  size_t buf_len = 16;
  char *resp = (char *)malloc(1024 * sizeof(char));
  MALLOC_ERROR_HELPER(resp, "Malloc Error.");
  size_t resp_len = 1024;
  recv(socket_desc, buf, buf_len, 0);
  int *to_notify = (int *)malloc(sizeof(int) * MAX_TOPICS);
  MALLOC_ERROR_HELPER(to_notify, "Malloc Error.");

  memset(to_notify, -1, MAX_TOPICS * sizeof(int));

  ////////////////////////////////////////////////////////////////
  //////////////////////////// notify ////////////////////////////
  if (strncmp(buf, "notify", 6) == 0) {
    ret = Pwait(mutex);
    ERROR_HELPER(ret, "Pwait Error");

    whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
    w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
    w->usershead = (user *)shmat(w->shmidus, NULL, 0);

    int cuid = get_user_by_usname(w, current_user)->id;

    int *listid =
        get_list_from_pool(w, cuid); // topics subscribed from current_user
    int j, i = 0;
    for (j = 0; j < MAX_TOPICS && listid[j] != -1; j++) {
      topic *t = get_topic(w, listid[j]);
      if (!int_in_arr(t->viewers, cuid)) {
        to_notify[i] = listid[j];
        printf("%d, ", to_notify[i]);
        i++;
      }
    }
    if (to_notify[0] != -1) {
      strcpy(resp, "\033[103;1m\033[30;1m   Some of the topics you follow are "
                   "updated! These are:");
      int len = strlen(resp);
      for (j = 0; j < MAX_TOPICS && to_notify[j] != -1; j++) {
        len += sprintf(resp + len, " %d,", to_notify[j]);
      }
      resp[strlen(resp) - 1] = ' ';
      strcat(resp, "        \033[0m\n\n\0");
    } else
      strcpy(resp,
             "\033[42;1m   No topics are updated!                              "
             "                                                 \033[0m\n\n\0");
    free(to_notify);
    free(buf);
    shmdt(w->usershead);
    shmdt(w->topicshead);
    shmdt(w);
    ret = Vpost(mutex);
    ERROR_HELPER(ret, "Vpost Error");
  }
  // printf("SENDING: %s\n",resp);   //DEBUG

  send(socket_desc, resp, strlen(resp), 0);
  free(resp);
}

void app_loop(int shmidwb, int socket_desc, char *current_user, int mutex) {
  notify(shmidwb, socket_desc, current_user, mutex);
  printf("%s\n", current_user); // DEBUG
  int ret, recv_bytes;
  int current_tp_id = -1;
  char *buf = (char *)malloc(1024 * sizeof(char));
  MALLOC_ERROR_HELPER(buf, "Malloc Error.");

  size_t buf_len = 1024;
  char *resp = (char *)malloc(32768 * sizeof(char));
  MALLOC_ERROR_HELPER(resp, "Malloc Error.");
  size_t resp_len = 32768;

  while (1) {
    memset(buf, 0, buf_len);   // FLUSH
    memset(resp, 0, resp_len); // FLUSH

    while ((recv(socket_desc, buf, buf_len, 0)) < 0) {
      if (errno == EINTR)
        continue;
      ERROR_HELPER(-1, "Cannot read from socket");
    }
    //////////////////////////////////////////////////////////////////////
    //////////////////////////// choose topic ////////////////////////////
    if (strncmp(buf, "topic ", 6) == 0) {
      int number = get_digit(buf, 6);
      if (number == -1) {
        strcpy(resp,
               "\033[41;1m   Invalid topic                                     "
               "                                                   \033[0m\0");
      } else {
        // printf("num: %d\n", number);
        ret = Pwait(mutex);
        ERROR_HELPER(ret, "Pwait Error");
        whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
        w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
        w->usershead = (user *)shmat(w->shmidus, NULL, 0);

        topic *t = get_topic(w, number);

        if (t == NULL)
          strcpy(
              resp,
              "\033[41;1m   Invalid topic                                      "
              "                                                  \033[0m\0");
        else {
          t->commentshead = (comment *)shmat(t->shmidcm, NULL, 0);
          int cuid = get_user_by_usname(w, current_user)->id;
          current_tp_id = t->id; // important!
          if (int_in_arr(t->subscribers, cuid)) {
            add_viewer(t, cuid);
            add_all_seen(t->commentshead, cuid);
            check_all_seen_by_all(t->subscribers, t->commentshead);
            strcpy(resp, tp_to_string(t, 1));
          } else
            strcpy(resp, tp_to_string(t, 0));
          strcat(resp, "\0");
          shmdt(t->commentshead);
        }
        shmdt(w->usershead);
        shmdt(w->topicshead);
        shmdt(w);
        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      }
      printf("current_tp_id: %d\n", current_tp_id); // DEBUG
    }
    /////////////////////////////////////////////////////////////////////
    //////////////////////////// list topics ////////////////////////////
    else if (strncmp(buf, "list topics", 11) == 0) {
      ret = Pwait(mutex);
      ERROR_HELPER(ret, "Pwait Error");
      whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
      w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);

      strcpy(resp, wb_to_string(w));
      strcat(resp, "\0");

      shmdt(w->topicshead);
      shmdt(w);
      ret = Vpost(mutex);
      ERROR_HELPER(ret, "Vpost Error");
      // printf("%s\n", resp);   //DEBUG
    }
    ////////////////////////////////////////////////////////////////////////
    //////////////////////////// status comment ////////////////////////////
    else if (strncmp(buf, "status ", 7) == 0) {
      int number = get_digit(buf, 7);
      if (number == -1) {
        strcpy(resp,
               "\033[41;1m   Invalid comment                                   "
               "                                                   \033[0m\0");
      } else {
        ret = Pwait(mutex);
        ERROR_HELPER(ret, "Pwait Error");
        whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
        w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
        w->usershead = (user *)shmat(w->shmidus, NULL, 0);

        if (current_tp_id == -1) {
          strcpy(
              resp,
              "\033[41;1m   At first you have to choose a topic.      (usage: "
              "topic [topic#])                                    \033[0m\0");
        } else {
          topic *t = get_topic(w, current_tp_id);
          int cuid = get_user_by_usname(w, current_user)->id;
          if (!int_in_arr(t->subscribers, cuid)) {
            strcpy(resp, "\033[41;1m   You should subscribe first.      "
                         "(usage: subscribe)                                   "
                         "               \033[0m\0");
          } else {
            t->commentshead = (comment *)shmat(t->shmidcm, NULL, 0);
            comment *c = get_comment(t, number);
            if (c == NULL) {
              strcpy(resp, "\033[41;1m   Invalid comment                       "
                           "                                                   "
                           "            \033[0m\0");
            } else {
              if (strcmp(c->status, "KK\0"))
                strcpy(resp, "Published and seen by all subscribers.\0");
              else if (strcmp(c->status, "K\0"))
                strcpy(resp, "Published, but not seen by all subscribers.\0");
            }
            shmdt(t->commentshead);
          }
        }
        shmdt(w->usershead);
        shmdt(w->topicshead);
        shmdt(w);
        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      }
    }
    /////////////////////////////////////////////////////////////////////////////////
    //////////////////////////// link [topic#] [thread#] ////////////////////////////
    else if (strncmp(buf, "link ", 5) == 0) {
      if (current_tp_id == -1) {
        strcpy(resp,
               "\033[41;1m   At first you have to choose a topic.      (usage: "
               "topic [topic#])                                    \033[0m\0");
      } else {
        int nth, nfrom = get_digit(buf, 5);
        printf("From Topic#: %d\n", nfrom); // DEBUG
        if (nfrom > 9) {
          if (nfrom > 99)
            nth = get_digit(buf, 9);
          else
            nth = get_digit(buf, 8);
        } else {
          nth = get_digit(buf, 7);
          printf("Thread#: %d\n", nth); // DEBUG
        }
        printf("%d", nth);
        if (nfrom == -1 || nth == -1) {
          strcpy(
              resp,
              "\033[41;1m   Invalid arguments                                  "
              "                                                  \033[0m\0");
        } else {
          ret = Pwait(mutex);
          ERROR_HELPER(ret, "Pwait Error");
          whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
          w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
          w->usershead = (user *)shmat(w->shmidus, NULL, 0);

          topic *to = get_topic(w, current_tp_id);
          topic *from = get_topic(w, nfrom);
          if (from == NULL) {
            strcpy(resp, "\033[41;1m   Invalid topic                           "
                         "                                                     "
                         "        \033[0m\0");
          } else {

            from->commentshead = (comment *)shmat(from->shmidcm, NULL, 0);
            comment *th = get_comment(from, nth);
            if (th->in_reply_to != -1) {
              strcpy(resp, "\033[41;1m   Invalid thread: this is a comment     "
                           "                                                   "
                           "            \033[0m\0");
              shmdt(from->commentshead);
            } else {
              if (th == NULL) {
                shmdt(from->commentshead);
                strcpy(resp, "\033[41;1m   Invalid thread                      "
                             "                                                 "
                             "              \033[0m\0");
              } else {
                shmdt(from->commentshead);
                to->commentshead = (comment *)shmat(to->shmidcm, NULL, 0);
                int cuid = get_user_by_usname(w, current_user)->id;
                if (!int_in_arr(to->subscribers, cuid)) {
                  strcpy(resp, "\033[41;1m   You should subscribe to topic "
                               "first.      (usage: subscribe)                 "
                               "                       \033[0m\0");
                } else {
                  char *content = (char *)malloc(1024 * sizeof(char));
                  MALLOC_ERROR_HELPER(content, "Malloc Error.");

                  sprintf(content,
                          "\033[33;1mLINK TO THREAD %d FROM TOPIC %d.\033[0m",
                          nth, nfrom);
                  strcat(content, "\0");
                  comment *last = get_last_comment(to);
                  comment *c =
                      new_comment(last->id + 1, current_user, 0, content, -1);
                  memset(to->viewers, -1,
                         MAX_REPLIES * sizeof(int)); // clear viewers
                  linkt *l = new_link(last->id + 1, nfrom, nth);

                  add_link(to, l);
                  push_comment(to, c);

                  add_viewer(to, cuid);
                  add_all_seen(to->commentshead, cuid);
                  check_all_seen_by_all(to->subscribers, to->commentshead);
                  free(content);
                  strcpy(resp, "\033[42;1m   Link added successfully!          "
                               "                                               "
                               "                    \033[0m\0");
                }
              }
            }
            shmdt(to->commentshead);
          }
          shmdt(w->usershead);
          shmdt(w->topicshead);
          shmdt(w);
          ret = Vpost(mutex);
          ERROR_HELPER(ret, "Vpost Error");
        }
      }
    }
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////// print link [link#] ////////////////////////////
    else if (strncmp(buf, "print link ", 11) == 0) {
      if (current_tp_id == -1) {
        strcpy(resp,
               "\033[41;1m   At first you have to choose a topic.      (usage: "
               "topic [topic#])                                    \033[0m\0");
      } else {
        int nl = get_digit(buf, 11);
        if (nl == -1) {
          strcpy(
              resp,
              "\033[41;1m   Invalid link                                       "
              "                                                  \033[0m\0");
        } else {
          int buf_len = 32768;
          ret = Pwait(mutex);
          ERROR_HELPER(ret, "Pwait Error");
          whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
          w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
          w->usershead = (user *)shmat(w->shmidus, NULL, 0);

          char buf[buf_len];

          strcpy(resp, ln_to_string(w, get_topic(w, current_tp_id), nl, buf));
          strcat(resp, "\0");

          memset(buf, 0, buf_len); // FLUSH

          shmdt(w->usershead);
          shmdt(w->topicshead);
          shmdt(w);
          ret = Vpost(mutex);
          ERROR_HELPER(ret, "Vpost Error");
        }
      }
    }
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////// reply to comment ////////////////////////////
    else if (strncmp(buf, "reply ", 6) == 0) {
      if (current_tp_id != -1) {

        char d = 'a';
        int number = get_digit(buf, 6);
        if (number == -1) {
          strcpy(
              resp,
              "\033[41;1m   Invalid comment                                    "
              "                                                  \033[0m\0");
          send(socket_desc, resp, strlen(resp), 0);
          recv(socket_desc, buf, buf_len, 0);
        } else {
          ret = Pwait(mutex);
          ERROR_HELPER(ret, "Pwait Error");
          whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
          w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
          w->usershead = (user *)shmat(w->shmidus, NULL, 0);

          topic *t = get_topic(w, current_tp_id);
          int cuid = get_user_by_usname(w, current_user)->id;

          if (!int_in_arr(t->subscribers, cuid)) {
            send(socket_desc, "subfirst\0", 10, 0);
            recv(socket_desc, resp, 1024, 0);
            strcpy(resp, "\033[41;1m   You should subscribe first.      "
                         "(usage: subscribe)                                   "
                         "               \033[0m\0");
          } else {
            // create comment and add
            t->commentshead = (comment *)shmat(t->shmidcm, NULL, 0);
            comment *r =
                get_comment(t, number); // check if number (reply_id) exists
            if (!(r == NULL)) {
              // release semaphore while waiting for user input
              ret = Vpost(mutex);
              ERROR_HELPER(ret, "Vpost Error");
              char *content = (char *)malloc(1024 * sizeof(char));
              MALLOC_ERROR_HELPER(content, "Malloc Error.");

              send(socket_desc, "content\0", 9, 0);

              recv(socket_desc, content, 1024, 0);
              ret = Pwait(mutex);
              ERROR_HELPER(ret, "Pwait Error");

              time_t date;
              time(&date);

              comment *last =
                  get_last_comment(t); // check if number (reply_id) exists
              comment *c = new_comment(last->id + 1, current_user, date,
                                       content, number);
              push_comment(t, c);
              add_reply(r, c->id);
              memset(t->viewers, -1, MAX_REPLIES * sizeof(int)); // clear
                                                                 // viewers
              print_arr(t->viewers); // DEBUG

              free(content);

              strcpy(resp, "\033[42;1m   Comment added successfully!           "
                           "                                                   "
                           "            \033[0m\0");
            } else {
              strcpy(resp, "\033[41;1m   Invalid comment                       "
                           "                                                   "
                           "            \033[0m\0");
              send(socket_desc, resp, strlen(resp), 0);
              recv(socket_desc, buf, buf_len, 0);
            }
            shmdt(t->commentshead);
          }
          shmdt(w->usershead);
          shmdt(w->topicshead);
          shmdt(w);
          ret = Vpost(mutex);
          ERROR_HELPER(ret, "Vpost Error");
        }
      } else {
        strcpy(resp,
               "\033[41;1m   At first you have to choose a topic.      (usage: "
               "topic [topic#])                                    \033[0m\0");
        send(socket_desc, resp, strlen(resp), 0);
        recv(socket_desc, buf, buf_len, 0);
      }
    }
    //////////////////////////////////////////////////////////////////////
    //////////////////////////// create topic ////////////////////////////
    else if (strncmp(buf, "create topic", 12) == 0) {
      char *title = (char *)malloc(256 * sizeof(char));
      MALLOC_ERROR_HELPER(title, "Malloc Error.");
      char *content = (char *)malloc(1024 * sizeof(char));
      MALLOC_ERROR_HELPER(content, "Malloc Error.");

      send(socket_desc, "title\0", 7, 0);

      recv(socket_desc, title, 256, 0);

      send(socket_desc, "content\0", 9, 0);

      recv(socket_desc, content, 1024, 0);

      ret = Pwait(mutex);
      ERROR_HELPER(ret, "Pwait Error");
      whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
      w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
      w->usershead = (user *)shmat(w->shmidus, NULL, 0);

      time_t date;
      time(&date);
      topic *last = get_last_topic(w);
      topic *t = new_topic(last->id + 1, current_user, title, content, date);

      int cuid = get_user_by_usname(w, current_user)->id;

      add_subscriber(t, cuid);
      add_subscription_entry(w, cuid, t->id);
      add_viewer(t, cuid);
      t->commentshead = (comment *)shmat(t->shmidcm, NULL, 0);
      add_all_seen(t->commentshead, cuid);
      check_all_seen_by_all(t->subscribers, t->commentshead);
      shmdt(t->commentshead);

      add_topic(w, t);

      shmdt(w->usershead);
      shmdt(w->topicshead);
      shmdt(w);
      ret = Vpost(mutex);
      ERROR_HELPER(ret, "Vpost Error");

      free(content);
      free(title);

      strcpy(resp,
             "\033[42;1m   Topic created successfully!                         "
             "                                                 \033[0m");
      strcat(resp, "\0");
    }
    ////////////////////////////////////////////////////////////////////
    //////////////////////////// add thread ////////////////////////////
    else if (strncmp(buf, "add thread", 10) == 0) {
      if (current_tp_id != -1) {

        whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
        w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
        w->usershead = (user *)shmat(w->shmidus, NULL, 0);

        topic *t = get_topic(w, current_tp_id);
        // create comment and add

        int cuid = get_user_by_usname(w, current_user)->id;

        if (!int_in_arr(t->subscribers, cuid)) {
          send(socket_desc, "subfirst\0", 10, 0);
          recv(socket_desc, resp, 1024, 0);
          strcpy(resp, "\033[41;1m   You should subscribe first.      (usage: "
                       "subscribe)                                             "
                       "     \033[0m\0");
        } else {
          // release semaphore while waiting for user input
          ret = Vpost(mutex);
          ERROR_HELPER(ret, "Vpost Error");

          char *content = (char *)malloc(1024 * sizeof(char));
          MALLOC_ERROR_HELPER(content, "Malloc Error.");

          send(socket_desc, "content\0", 9, 0);

          recv(socket_desc, content, 1024, 0);
          ret = Pwait(mutex);
          ERROR_HELPER(ret, "Pwait Error");

          time_t date;
          time(&date);

          t->commentshead = (comment *)shmat(t->shmidcm, NULL, 0);

          comment *last = get_last_comment(t);
          comment *c =
              new_comment(last->id + 1, current_user, date, content, -1);
          push_comment(t, c);
          memset(t->viewers, -1, MAX_REPLIES * sizeof(int)); // clear viewers
          print_arr(t->viewers);                             // DEBUG
          strcpy(
              resp,
              "\033[42;1m   Comment added successfully!                        "
              "                                                  \033[0m\0");

          free(content);
          shmdt(t->commentshead);
        }
        shmdt(w->usershead);
        shmdt(w->topicshead);
        shmdt(w);
        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      } else {
        strcpy(resp,
               "\033[41;1m   At first you have to choose a topic.      (usage: "
               "topic [topic#])                                    \033[0m\0");

        send(socket_desc, resp, strlen(resp), 0);
        recv(socket_desc, buf, buf_len, 0);
      }
    }
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////// subscribe to topic ////////////////////////////
    else if (strncmp(buf, "subscribe", 9) == 0) {
      if (current_tp_id != -1) {
        ret = Pwait(mutex);
        ERROR_HELPER(ret, "Pwait Error");
        whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
        w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
        w->usershead = (user *)shmat(w->shmidus, NULL, 0);

        user *u = get_user_by_usname(w, current_user);
        // if(u==NULL) printf("NULL\n");     //DEBUG
        // printf("us: %s\n",u->username);     //DEBUG
        topic *t = get_topic(w, current_tp_id);
        t->commentshead = (comment *)shmat(t->shmidcm, NULL, 0);
        int cuid = get_user_by_usname(w, current_user)->id;

        // printf("us: %s  -  tp:%d\n",u->username,t->id);     //DEBUG
        // CHECK if already subscribed
        if (int_in_arr(t->subscribers, cuid)) {
          strcpy(
              resp,
              "\033[43;1m   Already subscribed.                                "
              "                                                  \033[0m\0");
        } else {
          add_subscriber(t, u->id);
          add_subscription_entry(w, u->id, t->id);
          add_viewer(t, u->id);
          add_all_seen(t->commentshead, u->id);
          check_all_seen_by_all(t->subscribers, t->commentshead);
          // printf("Subscribers:\n");     //DEBUG
          // printf("%d-%d\n",t->subscribers[0],t->subscribers[1]);     //DEBUG
          strcpy(
              resp,
              "\033[42;1m   Subscribed.                                        "
              "                                                  \033[0m\n\n");
          strcat(resp, tp_to_string(t, 1));
          strcat(resp, "\0");
        }

        shmdt(t->commentshead);
        shmdt(w->usershead);
        shmdt(w->topicshead);
        shmdt(w);
        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      } else
        strcpy(resp,
               "\033[41;1m   At first you have to choose a topic.      (usage: "
               "topic [topic#])                                    \033[0m\0");
    }
    //////////////////////////////////////////////////////////////////////
    //////////////////////////// delete topic ////////////////////////////
    else if (strncmp(buf, "delete topic ", 13) == 0) {
      int number = get_digit(buf, 13);
      // printf("%d\n",number);    //DEBUG
      if (number == -1)
        strcpy(resp, "\033[41;1m   I don't understand which topic I should delete."
                         "                                                     "
                         " \033[0m\0");
      else {
        ret = Pwait(mutex);
        ERROR_HELPER(ret, "Pwait Error");
        whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
        w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);

        topic *t = get_topic(w, number);
        if (!(t == NULL)) {
          if (!strcmp(t->author, current_user)) {
            delete_topic(w, number);
            strcpy(resp, "\033[42;1m   Deleted successfully.                   "
                         "                                                     "
                         "        \033[0m\0");
          } else
            strcpy(resp, "\033[41;1m   Only topic's author can delete it.      "
                         "                                                     "
                         "        \033[0m\0");
        } else
          strcpy(
              resp,
              "\033[41;1m   This topic does not exist.                         "
              "                                                  \033[0m\0");

        shmdt(w->topicshead);
        shmdt(w);
        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      }

    }
    ///////////////////////////////////////////////////////////////////////////
    //////////////////////////// ADMIN: list users ////////////////////////////
    else if (!strcmp(buf, "list users")) {
      if (strcmp(current_user, "admin")) {
        strcpy(resp,
               "\033[41;1m   You are not allowed to perform this action.       "
               "                                                   \033[0m\0");
      } else {
        ret = Pwait(mutex);
        ERROR_HELPER(ret, "Pwait Error");
        whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
        w->usershead = (user *)shmat(w->shmidus, NULL, 0);

        strcpy(resp, us_to_string(w));
        strcat(resp, "\0");

        shmdt(w->usershead);
        shmdt(w);
        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      }
    }
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////// ADMIN: delete user ////////////////////////////
    else if (!strncmp(buf, "delete user ", 12)) {
      int userid = get_digit(buf, 12);
      // printf("%d\n",number);    //DEBUG
      if (userid == -1)
        strcpy(resp, "I don't understand which user I should delete.\0");
      else {
        if (strcmp(current_user, "admin")) {
          strcpy(
              resp,
              "\033[41;1m   You are not allowed to perform this action.        "
              "                                                  \033[0m\0");
        } else if (userid == 0)
          strcpy(
              resp,
              "\033[41;1m   This user cannot be deleted.                       "
              "                                                  \033[0m\0");
        else {
          ret = Pwait(mutex);
          ERROR_HELPER(ret, "Pwait Error");
          whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);
          w->usershead = (user *)shmat(w->shmidus, NULL, 0);

          delete_user(w, userid);
          strcpy(resp, us_to_string(w));
          strcat(resp, "\0");

          shmdt(w->usershead);
          shmdt(w);
          ret = Vpost(mutex);
          ERROR_HELPER(ret, "Vpost Error");
        }
      }
    }
    //////////////////////////////////////////////////////////////
    //////////////////////////// quit ////////////////////////////
    else if (strncmp(buf, "quit", 4) == 0) {
      break;
    }
    //////////////////////////////////////////////////////////////
    //////////////////////////// help ////////////////////////////
    else /* default: */ {
      strcpy(resp, "help\0");
    }

    send(socket_desc, resp, strlen(resp), 0);
  }
  // printf("free\n");     //DEBUG

  free(buf);
  free(resp);
  return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// PROCESS ROUTINE
/////////////////////////////////////////////

void *connection_handler(int shmidwb, int socket_desc,
                         struct sockaddr_in *client_addr, int mutex) {

  int ret, recv_bytes;

  char *current_user = NULL;

  char *buf = (char *)malloc(1024 * sizeof(char));
  MALLOC_ERROR_HELPER(buf, "Malloc Error.");
  size_t buf_len = 1024;

  // parse client IP address and port
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
  // read message from client
  while ((recv_bytes = recv(socket_desc, buf, buf_len, 0)) < 0) {
    if (errno == EINTR)
      continue;
    ERROR_HELPER(-1, "Cannot read from socket");
  }

  printf("Command: %s\n", buf); // DEBUG

  //////////////////////////////////////////////////////////////////////
  ///////////////////////////////REGISTER///////////////////////////////
  if (buf[0] == 'R' || buf[0] == 'r') {
    char *resp = NULL;
    while (1) {
      resp = Register(shmidwb, socket_desc, mutex);

      if (resp != NULL)
        break;
      // printf("Username already taken.\n");     //DEBUG
      send(socket_desc, "Username already taken.\0", 32, 0);
      // close socket
      ret = close(socket_desc);
      ERROR_HELPER(ret, "Cannot close socket for incoming connection");
      free(buf);
      free(client_addr);      // do not forget to free this buffer!
      printf("all closed\n"); // DEBUG
      exit(1);
    }
    // printf("out\n");     //DEBUG
    strcpy(resp, "Registration Done.\0");
    send(socket_desc, resp, 32, 0);
  }
  //////////////////////////////////////////////////////////////////
  ///////////////////////////////AUTH///////////////////////////////
  if (buf[0] == 'A' || buf[0] == 'a') {

    current_user = Auth(shmidwb, socket_desc, mutex);
    if (current_user == NULL) {
      // printf("Invalid credentials.\n");     //DEBUG
      send(socket_desc, "Invalid credentials.\0", 32, 0);
      free(buf);
      free(client_addr);      // do not forget to free this buffer!
      printf("all closed\n"); // DEBUG
      exit(1);
    }
    printf("Current_User: %s\n", current_user); // DEBUG

    char *resp = current_user;
    replace_char(resp, '\n', '\0');
    send(socket_desc, resp, 32, 0);

    app_loop(shmidwb, socket_desc, current_user, mutex);
  }
  // close socket
  ret = close(socket_desc);
  ERROR_HELPER(ret, "Cannot close socket for incoming connection");
  free(buf);
  free(client_addr);      // do not forget to free this buffer!
  printf("all closed\n"); // DEBUG
  exit(1);
}

////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MAIN ///////////////////////////////////
int main(int argc, char *argv[]) {
  int ret;
  int processID;
  int socket_desc, client_desc;

  // semaphore
  key_t semkey = 0x200;
  int mutex;

  if ((mutex = initsem(semkey)) < 0) {
    perror("initsem");
    exit(1);
  }

  // shared memory
  int shmidwb;
  // get shared memory - whiteboard
  if ((shmidwb = shmget(10000, sizeof(whiteboard), IPC_CREAT | 0666)) < 0) {
    perror("shmget");
    exit(1);
  }

  // attach to shared memory  // no need to protect with semaphores
  whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);

  w = create_wb(w);

  shmdt(w);

  // some fields are required to be filled with 0
  struct sockaddr_in server_addr = {0};

  int sockaddr_len = sizeof(struct sockaddr_in); // I will reuse it for accept()

  // initialize socket for listening
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  ERROR_HELPER(socket_desc, "Could not create socket");

  server_addr.sin_addr.s_addr =
      INADDR_ANY; // I want to accept connections from any interface
  server_addr.sin_family = AF_INET;
  server_addr.sin_port =
      htons(SERVER_PORT); // don't forget about network byte order!

  int reuseaddr_opt = 1;
  ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt,
                   sizeof(reuseaddr_opt));
  ERROR_HELPER(ret, "Cannot set SO_REUSEADDR option");

  // bind address to socket
  ret = bind(socket_desc, (struct sockaddr *)&server_addr, sockaddr_len);
  ERROR_HELPER(ret, "Cannot bind address to socket");

  // start listening
  ret = listen(socket_desc, MAX_CONN_QUEUE);
  ERROR_HELPER(ret, "Cannot listen on socket");

  // I allocate client_addr dynamically and initialize it to zero
  struct sockaddr_in *client_addr = calloc(1, sizeof(struct sockaddr_in));
  MALLOC_ERROR_HELPER(client_addr, "Calloc Error.");
  if (DEBUG)
    fprintf(stderr, "Server in listening mode...\n");

  while (1) {
    // accept incoming connection
    client_desc = accept(socket_desc, (struct sockaddr *)client_addr,
                         (socklen_t *)&sockaddr_len);
    if (client_desc == -1 && errno == EINTR)
      continue; // check for interruption by signals
    ERROR_HELPER(client_desc, "Cannot open socket for incoming connection");

    printf("Incoming connection accepted...\n");

    if ((processID = fork()) < 0) {
      ERROR_HELPER(processID, "Fork error.");
    } else if (processID == 0) { /* If this is the child process */
      connection_handler(shmidwb, client_desc, client_addr, mutex);
    } else {
      client_addr = calloc(1, sizeof(struct sockaddr_in));
      MALLOC_ERROR_HELPER(client_addr, "Calloc Error.");
    }
  }
  exit(EXIT_SUCCESS); // this will never be executed
}
