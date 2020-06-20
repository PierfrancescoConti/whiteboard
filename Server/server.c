#include "whiteboard.h"


void notify(whiteboard* w, int socket_desc, char *current_user, int mutex) {
  int ret;
  char *buf = (char *)malloc(16 * sizeof(char));
  MALLOC_ERROR_HELPER(buf, "Malloc Error.");
  size_t buf_len = 16;
  char *resp = (char *)malloc(1024 * sizeof(char));
  MALLOC_ERROR_HELPER(resp, "Malloc Error.");
  ret=recv(socket_desc, buf, buf_len, 0);
  ERROR_HELPER(ret, "Recv Error");
  int *to_notify = (int *)malloc(sizeof(int) * MAX_TOPICS);   // array to fill with TOPIC_IDs to notify to the current user
  MALLOC_ERROR_HELPER(to_notify, "Malloc Error.");
  memset(to_notify, -1, MAX_TOPICS * sizeof(int));

  ////////////////////////////////////////////////////////////////
  //////////////////////////// notify ////////////////////////////
  if (strncmp(buf, "notify", 6) == 0) {
    ret = Pwait(mutex);
    ERROR_HELPER(ret, "Pwait Error");

    int cuid = get_user_by_usname(w, current_user)->id;

    int *listid = get_list_from_pool(w, cuid); // topics subscribed from current_user
    int j, i = 0;
    for (j = 0; j < MAX_TOPICS && listid[j] != -1; j++) {
      topic *t = get_topic(w, listid[j]);
      if (!int_in_arr(t->viewers, cuid)) {    // is this topic changed? -> notify!
        to_notify[i] = listid[j];
        i++;
      }
    }
    // Notifications
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

    ret = Vpost(mutex);
    ERROR_HELPER(ret, "Vpost Error");
  }

  ret=send(socket_desc, resp, strlen(resp), 0);
  ERROR_HELPER(ret, "Send Error");
  free(resp);
}

// After authentication -> start main Application Loop
void app_loop(whiteboard* w, int socket_desc, char *current_user, int mutex/*, int tpempty, int tpfull*/) {
  notify(w, socket_desc, current_user, mutex);    // notify me fi any topic is changed
  int ret;
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

    ret=recv(socket_desc, buf, buf_len, 0);
    ERROR_HELPER(ret, "Recv Error");

    //////////////////////////////////////////////////////////////////////
    //////////////////////////// choose topic ////////////////////////////
    if (strncmp(buf, "topic ", 6) == 0) {
      int number = get_digit(buf, 6);   // get number of topic to visualize
      if (number == -1) {
        strcpy(resp,
               "\033[41;1m   Invalid topic                                     "
               "                                                   \033[0m\0");
      } else {
        ret = Pwait(mutex);
        ERROR_HELPER(ret, "Pwait Error");        

        topic *t = get_topic(w, number);    // get topic

        if (t == NULL)
          strcpy(
              resp,
              "\033[41;1m   Invalid topic                                      "
              "                                                  \033[0m\0");
        else {
          int cuid = get_user_by_usname(w, current_user)->id;
          current_tp_id = t->id; // important!
          if (int_in_arr(t->subscribers, cuid)) {
            add_viewer(t, cuid);
            add_all_seen(t->commentshead, cuid);
            check_all_seen_by_all(t->subscribers, t->commentshead);
            strcpy(resp, tp_to_string(t, 1));   // send topic to client
          } else
            strcpy(resp, tp_to_string(t, 0));
          strcat(resp, "\0");
        }
        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      }
    }
    /////////////////////////////////////////////////////////////////////
    //////////////////////////// list topics ////////////////////////////
    else if (strncmp(buf, "list topics", 11) == 0) {
      ret = Pwait(mutex);
      ERROR_HELPER(ret, "Pwait Error");

      strcpy(resp, wb_to_string(w));  // list all topics
      strcat(resp, "\0");

      ret = Vpost(mutex);
      ERROR_HELPER(ret, "Vpost Error");
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
            comment *c = get_comment(t, number);
            if (c == NULL) {
              strcpy(resp, "\033[41;1m   Invalid comment                       "
                           "                                                   "
                           "            \033[0m\0");
            } else {
              // print the message with further info (author, status, timestamp)
              if (!strcmp(c->status, "KK")){
                int len = sprintf(resp, "\n\n\t\033[33;1m%d\033[0m. ", c->id);
                len += sprintf(resp + len, "\033[1m%s\033[0m\n", c->comm);
                len += sprintf(resp + len, "\t\t    by %s\n", c->author);
                len += sprintf(resp + len, "\n\n   \033[34;1mKK\033[0m: \033[1mPublished and seen by all subscribers. \033[0m\n\n   Date: \033[33;1m%s\033[0m\n\n", asctime(localtime(&(c->timestamp))));
              }
              else if (!strcmp(c->status, "K")){
                int len = sprintf(resp, "\n\n\t\033[33;1m%d\033[0m. ", c->id);
                len += sprintf(resp + len, "\033[1m%s\033[0m\n", c->comm);
                len += sprintf(resp + len, "\t\t    by %s\n", c->author);
                len += sprintf(resp + len, "\n\n   \033[34;1mK\033[0m: \033[1mPublished, but not seen by all subscribers. \033[0m\n\n   Date: \033[33;1m%s\033[0m\n\n", asctime(localtime(&(c->timestamp))));
              }
            }
          }
        }
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
        // get numbers of topic and thread
        int nth, nfrom = get_digit(buf, 5);
        if (nfrom > 9) {
          if (nfrom > 99)
            nth = get_digit(buf, 9);
          else
            nth = get_digit(buf, 8);
        } else {
          nth = get_digit(buf, 7);
        }
        if (nfrom == -1 || nth == -1) {
          strcpy(
              resp,
              "\033[41;1m   Invalid arguments                                  "
              "                                                  \033[0m\0");
        } else {
          ret = Pwait(mutex);
          ERROR_HELPER(ret, "Pwait Error");

          topic *to = get_topic(w, current_tp_id);  // get current topic
          topic *from = get_topic(w, nfrom);        // get topic of the thread
          
          if (from == NULL) {
            strcpy(resp, "\033[41;1m   Invalid topic                           "
                         "                                                     "
                         "        \033[0m\0");
          } else {
            comment *th = get_comment(from, nth);   // get the thread
            if (th == NULL) {
              strcpy(resp, "\033[41;1m   Invalid thread                      "
                             "                                                 "
                             "              \033[0m\0");
            } else {
              if (th->in_reply_to != -1) {
                strcpy(resp, "\033[41;1m   Invalid thread: this is a comment     "
                           "                                                   "
                           "            \033[0m\0");
                
              } else {
                int cuid = get_user_by_usname(w, current_user)->id;
                if (!int_in_arr(to->subscribers, cuid)) {
                  strcpy(resp, "\033[41;1m   You should subscribe to topic "
                               "first.      (usage: subscribe)                 "
                               "                       \033[0m\0");
                } else {
                  char *content = (char *)malloc(1024 * sizeof(char));
                  MALLOC_ERROR_HELPER(content, "Malloc Error.");

                  sprintf(content,
                          "\033[33;1mLINK TO THREAD %d FROM TOPIC %d.\033[0m\n",
                          nth, nfrom);
                  strcat(content, "\0");
                  comment *last = get_last_comment(to);
                  if(last->id>=MAX_COMMENTS-1){
                    free(content);
                    strcpy(resp, "\033[41;1m   Current Topic's memory is full!   "
                                 "                                               "
                                 "                    \033[0m\0");

                  } else {
                    comment *c = new_comment(last->id + 1, current_user, 0, content, -1);      //creating the comment
                    memset(to->viewers, -1,
                           MAX_REPLIES * sizeof(int)); // clear viewers
                    linkt *l = new_link(last->id + 1, nfrom, nth);    // creating the relative link
                    
                    add_link(to, l);      // adding link to list
                    push_comment(to, c);  // adding link to topic's threads

                    // I've seen the comment and have to check its status
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
            }
          }
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

          char buf[buf_len];
          memset(buf, 0, buf_len); // FLUSH

          // shows to client the thread and its messages (relative to the link)
          strcpy(resp, ln_to_string(w, get_topic(w, current_tp_id), nl, buf));
          strcat(resp, "\0");

          memset(buf, 0, buf_len); // FLUSH

          ret = Vpost(mutex);
          ERROR_HELPER(ret, "Vpost Error");
        }
      }
    }
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////// reply to comment ////////////////////////////
    else if (strncmp(buf, "reply ", 6) == 0) {
      if (current_tp_id != -1) {

        int number = get_digit(buf, 6);   // get the number of the comment to respond

        if (number == -1 || number == 0) {
          strcpy(
              resp,
              "\033[41;1m   Invalid comment                                    "
              "                                                  \033[0m\0");
          ret=send(socket_desc, resp, strlen(resp), 0);
          ERROR_HELPER(ret, "Send Error");
          ret=recv(socket_desc, buf, buf_len, 0);
          ERROR_HELPER(ret, "Recv Error");
        } else {
          ret = Pwait(mutex);
          ERROR_HELPER(ret, "Pwait Error");

          topic *t = get_topic(w, current_tp_id);
          int cuid = get_user_by_usname(w, current_user)->id;

          if (!int_in_arr(t->subscribers, cuid)) {
            ret=send(socket_desc, "subfirst\0", 10, 0);
            ERROR_HELPER(ret, "Send Error");
            ret=recv(socket_desc, resp, 1024, 0);
            ERROR_HELPER(ret, "Recv Error");
            strcpy(resp, "\033[41;1m   You should subscribe first.      "
                         "(usage: subscribe)                                   "
                         "               \033[0m\0");
          } else {
            // create comment and add
            comment *r = get_comment(t, number); // check if number (reply_id) exists
            if (!(r == NULL)) {
              // release semaphore while waiting for user input
              ret = Vpost(mutex);
              ERROR_HELPER(ret, "Vpost Error");
              char *content = (char *)malloc(1024 * sizeof(char));
              MALLOC_ERROR_HELPER(content, "Malloc Error.");

              ret=send(socket_desc, "content\0", 9, 0);
              ERROR_HELPER(ret, "Send Error");

              ret=recv(socket_desc, content, 1024, 0);
              ERROR_HELPER(ret, "Recv Error");
              ret = Pwait(mutex);
              ERROR_HELPER(ret, "Pwait Error");

              time_t date;
              time(&date);

              comment *last = get_last_comment(t);
              if(last->id>=MAX_COMMENTS-1){
                    free(content);
                    strcpy(resp, "\033[41;1m   Current Topic's memory is full!   "
                                 "                                               "
                                 "                    \033[0m\0");

              } else {

                comment *c = new_comment(last->id + 1, current_user, date,
                                         content, number);
                push_comment(t, c);     // add the message to topic's messages

                add_reply(r, c->id);    // add id of the reply to comment's replies

                memset(t->viewers, -1, MAX_SUBSCRIBERS * sizeof(int)); // clear viewers

                free(content);

                strcpy(resp, "\033[42;1m   Comment added successfully!           "
                             "                                                   "
                             "            \033[0m\0");
              }
            } else {
              strcpy(resp, "\033[41;1m   Invalid comment                       "
                           "                                                   "
                           "            \033[0m\0");
              ret=send(socket_desc, resp, strlen(resp), 0);
              ERROR_HELPER(ret, "Send Error");
              ret=recv(socket_desc, buf, buf_len, 0);
              ERROR_HELPER(ret, "Recv Error");
            }
          }
          ret = Vpost(mutex);
          ERROR_HELPER(ret, "Vpost Error");
        }
      } else {
        strcpy(resp,
               "\033[41;1m   At first you have to choose a topic.      (usage: "
               "topic [topic#])                                    \033[0m\0");
        ret=send(socket_desc, resp, strlen(resp), 0);
        ERROR_HELPER(ret, "Send Error");
        ret=recv(socket_desc, buf, buf_len, 0);
        ERROR_HELPER(ret, "Recv Error");
      }
    }
    //////////////////////////////////////////////////////////////////////
    //////////////////////////// create topic ////////////////////////////
    else if (strncmp(buf, "create topic", 12) == 0) {
      char *title = (char *)malloc(256 * sizeof(char));
      MALLOC_ERROR_HELPER(title, "Malloc Error.");
      char *content = (char *)malloc(1024 * sizeof(char));
      MALLOC_ERROR_HELPER(content, "Malloc Error.");

      ret=send(socket_desc, "title\0", 7, 0);
      ERROR_HELPER(ret, "Send Error");

      ret=recv(socket_desc, title, 256, 0);
      ERROR_HELPER(ret, "Recv Error");

      ret=send(socket_desc, "content\0", 9, 0);
      ERROR_HELPER(ret, "Send Error");

      ret=recv(socket_desc, content, 1024, 0);
      ERROR_HELPER(ret, "Recv Error");

      ret = Pwait(mutex);
      ERROR_HELPER(ret, "Pwait Error");

      topic *last = get_last_topic(w);
      if(last->id<MAX_TOPICS-1){

        topic *t = new_topic(last->id + 1, current_user, title, content);   // create topic

        int cuid = get_user_by_usname(w, current_user)->id;

        // Initializing the new topic
        add_subscriber(t, cuid);
        add_subscription_entry(w, cuid, t->id);
        add_viewer(t, cuid);
        add_all_seen(t->commentshead, cuid);
        check_all_seen_by_all(t->subscribers, t->commentshead);

        add_topic(w, t);    // adding topic to whiteboard

        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");

        free(content);
        free(title);

        strcpy(resp,
               "\033[42;1m   Topic created successfully!                         "
               "                                                 \033[0m");
        strcat(resp, "\0");
      }
      else{
        strcpy(resp,
               "\033[41;1m   Wait! You can't create more topics.     "
               "                                                             \033[0m");
        strcat(resp, "\0");
        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      }

      
    }
    ////////////////////////////////////////////////////////////////////
    //////////////////////////// add thread ////////////////////////////
    else if (strncmp(buf, "add thread", 10) == 0) {
      if (current_tp_id != -1) {
        ret = Pwait(mutex);
        ERROR_HELPER(ret, "Pwait Error");

        topic *t = get_topic(w, current_tp_id);   // get the current topic

        int cuid = get_user_by_usname(w, current_user)->id;

        if (!int_in_arr(t->subscribers, cuid)) {    // subscribed?
          ret=send(socket_desc, "subfirst\0", 10, 0);
          ERROR_HELPER(ret, "Send Error");
          ret=recv(socket_desc, resp, 1024, 0);
          ERROR_HELPER(ret, "Recv Error");
          strcpy(resp, "\033[41;1m   You should subscribe first.      (usage: "
                       "subscribe)                                             "
                       "     \033[0m\0");
        } else {
          // release semaphore while waiting for user input
          ret = Vpost(mutex);
          ERROR_HELPER(ret, "Vpost Error");

          char *content = (char *)malloc(1024 * sizeof(char));
          MALLOC_ERROR_HELPER(content, "Malloc Error.");

          ret=send(socket_desc, "content\0", 9, 0);
          ERROR_HELPER(ret, "Send Error");

          ret=recv(socket_desc, content, 1024, 0);
          ERROR_HELPER(ret, "Recv Error");
          ret = Pwait(mutex);
          ERROR_HELPER(ret, "Pwait Error");

          time_t date;
          time(&date);


          comment *last = get_last_comment(t);
          if(last->id>=MAX_COMMENTS-1){   // check if exceeding memory
            free(content);
            strcpy(resp, "\033[41;1m   Current Topic's memory is full!   "
                         "                                               "
                         "                    \033[0m\0");

          } else {
            // create a new thread and add it to topic
            comment *c = new_comment(last->id + 1, current_user, date, content, -1);
            push_comment(t, c);
            memset(t->viewers, -1, MAX_SUBSCRIBERS * sizeof(int)); // clear viewers
            strcpy(
                resp,
                "\033[42;1m   Comment added successfully!                        "
                "                                                  \033[0m\0");

            free(content);
          }
        }
        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      } else {
        strcpy(resp,
               "\033[41;1m   At first you have to choose a topic.      (usage: "
               "topic [topic#])                                    \033[0m\0");

        ret=send(socket_desc, resp, strlen(resp), 0);
        ERROR_HELPER(ret, "Send Error");
        ret=recv(socket_desc, buf, buf_len, 0);
        ERROR_HELPER(ret, "Recv Error");
      }
    }
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////// subscribe to topic ////////////////////////////
    else if (strncmp(buf, "subscribe", 9) == 0) {
      if (current_tp_id != -1) {
        ret = Pwait(mutex);
        ERROR_HELPER(ret, "Pwait Error");

        user *u = get_user_by_usname(w, current_user);
        topic *t = get_topic(w, current_tp_id);
        int cuid = get_user_by_usname(w, current_user)->id;

        // CHECK if already subscribed
        if (int_in_arr(t->subscribers, cuid)) {
          strcpy(
              resp,
              "\033[43;1m   Already subscribed.                                "
              "                                                  \033[0m\0");
        } else {
          //adding the user to subscribers 
          add_subscriber(t, u->id);
          add_subscription_entry(w, u->id, t->id);
          add_viewer(t, u->id);
          add_all_seen(t->commentshead, u->id);
          check_all_seen_by_all(t->subscribers, t->commentshead);
          strcpy(
              resp,
              "\033[42;1m   Subscribed.                                        "
              "                                                  \033[0m\n\n");
          strcat(resp, tp_to_string(t, 1));
          strcat(resp, "\0");
        }

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
      if (number == -1)
        strcpy(resp, "\033[41;1m   I don't understand which topic I should delete."
                         "                                                     "
                         " \033[0m\0");
      else {
        ret = Pwait(mutex);
        ERROR_HELPER(ret, "Pwait Error");

        topic *t = get_topic(w, number);    // get the topic to delete
        if (!(t == NULL)) {
          if (!strcmp(t->author, current_user)) {   // check if the user is the author
            delete_topic(w, number);    // if so, delete
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

        strcpy(resp, us_to_string(w));    // send the list of users
        strcat(resp, "\0");

        ret = Vpost(mutex);
        ERROR_HELPER(ret, "Vpost Error");
      }
    }
    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////// ADMIN: delete user ////////////////////////////
    else if (!strncmp(buf, "delete user ", 12)) {
      int userid = get_digit(buf, 12);
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

          delete_user(w, userid);     // delete a user
          strcpy(resp, us_to_string(w));    // and see again users
          strcat(resp, "\0");

          ret = Vpost(mutex);
          ERROR_HELPER(ret, "Vpost Error");
        }
      }
    }
    //////////////////////////////////////////////////////////////
    //////////////////////////// quit ////////////////////////////
    else if (strncmp(buf, "quit", 4) == 0) {
      strcpy(resp, "\n                                        \033[43;1m      Goodbye!      \033[0m\n\n");
      ret=send(socket_desc, resp, strlen(resp), 0);
      ERROR_HELPER(ret, "Send Error");
      break;      // goodbye
    }
    //////////////////////////////////////////////////////////////
    //////////////////////////// help ////////////////////////////
    else /* default: */ {
      strcpy(resp, "help\0");
    }

    ret=send(socket_desc, resp, strlen(resp), 0);   // send any response from above commands
    ERROR_HELPER(ret, "Send Error");

    // SAVE WHITEBOARD
    decryptall("saved_dumps");
    rmenc("saved_dumps");
    
    ret = Pwait(mutex);
    ERROR_HELPER(ret, "Pwait Error");

    save_wb(w);                       // saving the whiteboard

    ret = Vpost(mutex);
    ERROR_HELPER(ret, "Vpost Error");

    encryptall("saved_dumps");
    rmdec("saved_dumps");
    // Saving complete!

  }

  free(buf);
  free(resp);
  exit(EXIT_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// PROCESS ROUTINE///////////////////////////////////////////

void *connection_handler(whiteboard* w, int socket_desc,
                         struct sockaddr_in *client_addr, int mutex/*, int tpempty, int tpfull*/) {

  int ret;

  char *current_user = NULL;

  char *buf = (char *)malloc(1024 * sizeof(char));
  MALLOC_ERROR_HELPER(buf, "Malloc Error.");
  size_t buf_len = 1024;

  // parse client IP address and port
  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);

  

  // read message from client
  ret=recv(socket_desc, buf, buf_len, 0);
  ERROR_HELPER(ret, "Recv Error");
  



  //////////////////////////////////////////////////////////////////////
  ///////////////////////////////REGISTER///////////////////////////////
  if (buf[0] == 'R' || buf[0] == 'r') {
    char *resp = NULL;
    while (1) {
      resp = Register(w, socket_desc, mutex);
      
      if(!strcmp(resp,"Too many!")){    // no more space for other users
        ret=send(socket_desc, "Users memory is full.\0", 32, 0);
        ERROR_HELPER(ret, "Send Error");
        // close socket
        ret = close(socket_desc);
        ERROR_HELPER(ret, "Cannot close socket for incoming connection");
        free(buf);
        free(client_addr);      // do not forget to free this buffer!
        exit(1);
      } else if (!strcmp(resp,"NULL")){     // user already exists
        ret=send(socket_desc, "Username already taken.\0", 32, 0);
        ERROR_HELPER(ret, "Send Error");
        // close socket
        ret = close(socket_desc);
        ERROR_HELPER(ret, "Cannot close socket for incoming connection");
        free(buf);
        free(client_addr);      // do not forget to free this buffer!
        exit(1);
      }
      else
          break;
    }
    strcpy(resp, "Registration Done.\0");     // user added
    ret=send(socket_desc, resp, 32, 0);
    ERROR_HELPER(ret, "Send Error");
    
  }
  //////////////////////////////////////////////////////////////////
  ///////////////////////////////AUTH///////////////////////////////
  if (buf[0] == 'A' || buf[0] == 'a') {

    current_user = Auth(w, socket_desc, mutex);
    if (current_user == NULL) {     // not existing user or wrong password
      ret=send(socket_desc, "Invalid credentials.\0", 32, 0);
      ERROR_HELPER(ret, "Send Error");
      free(buf);
      free(client_addr);      // do not forget to free this buffer!
      exit(1);
    }
    char *resp = current_user;
    replace_char(resp, '\n', '\0');
    ret=send(socket_desc, resp, 32, 0);
    ERROR_HELPER(ret, "Send Error");
    // the user is authenticated and it can start the main Application Loop
    app_loop(w, socket_desc, current_user, mutex/*, tpempty, tpfull*/);
  }
  // close socket
  ret = close(socket_desc);
  ERROR_HELPER(ret, "Cannot close socket for incoming connection");
  free(buf);
  free(client_addr);      // do not forget to free this buffer!
  exit(1);
}

////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MAIN ///////////////////////////////////
int main(int argc, char *argv[]) {
  int ret;
  int processID;
  int socket_desc, client_desc;

  // semaphore
  key_t semkey = IPC_PRIVATE;
  //key_t tpfullsemkey = 0x200;
  //key_t tpemptysemkey = 0x300;
  int mutex/*, tpfull, tpempty*/;

  semclean(semkey);     // cleaning already existing semaphores
  //semclean(tpfullsemkey);     //NOT NECESSARY!!!
  //semclean(tpemptysemkey);

  mutex = initsem(semkey,1);    // initialization of mutex
  ERROR_HELPER(mutex,"InitSem Error");

  //tpfull = initsem(tpfullsemkey,0);
  //ERROR_HELPER(tpfull,"InitSem Error");

  //tpempty = initsem(tpemptysemkey,MAX_TOPICS);
  //ERROR_HELPER(tpempty,"InitSem Error");


  // shared memory
  int shmidwb;
  // get shared memory - whiteboard
  if ((shmidwb = shmget(10000, sizeof(whiteboard), IPC_CREAT | 0666)) < 0) {
    perror("shmget");
    exit(1);
  }

  // attach to shared memory  // no needed to protect with semaphores
  whiteboard *w = (whiteboard *)shmat(shmidwb, NULL, 0);

  w = create_wb(w);   // create and initialize the whiteboard

  // opening shared memories from father process
  w->topicshead = (topic *)shmat(w->shmidto, NULL, 0);
  w->usershead = (user *)shmat(w->shmidus, NULL, 0);



  

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

  printf("Server in listening mode...\n");

  // LOAD WHITEBOARD
  decryptall("saved_dumps");
  //system("./loader &");   // replaced from fork/exec/wait procedure
  int pid, pid2, status;
  if ((pid2 = fork()) < 0) {
    ERROR_HELPER(pid2, "Fork error.");
// PARENT CANNOT WAIT! child to wait the users' loading:
  } else if (pid2 == 0) {
    if ((pid = fork()) < 0) {
      ERROR_HELPER(pid2, "Fork error.");
    } else if (pid == 0) {     
      execv("./loader",argv);
      exit(EXIT_FAILURE); /* only if execv fails */
  
    } else {
    if ((ret = waitpid(pid,&status,0)) == -1)
      printf ("parent:error\n");
    if (WEXITSTATUS(status)) 
      printf("\033[31;1mDANGER: USERS NOT LOADED\033[0m\n"); 
    else
      printf("\033[32mUsers loaded Successfully!\033[0m\n"); 
    
  
    }

    
  }

  ret = Pwait(mutex);
  ERROR_HELPER(ret, "Pwait Error");
  load_wb(w);                                // loading
  ret = Vpost(mutex);
  ERROR_HELPER(ret, "Vpost Error");
  rmdec("saved_dumps");
  // Loading complete!
  while (1) {
    // accept incoming connection
    client_desc = accept(socket_desc, (struct sockaddr *)client_addr,
                         (socklen_t *)&sockaddr_len);
    if (client_desc == -1 && errno == EINTR)
      continue; // check for interruption by signals
    ERROR_HELPER(client_desc, "Cannot open socket for incoming connection");
    //printf("\n\033[36;4mIncoming connection accepted...\n\033[0m");
    if ((processID = fork()) < 0) {
      ERROR_HELPER(processID, "Fork error.");
    } else if (processID == 0) { /* If this is the child process */
      connection_handler(w, client_desc, client_addr, mutex/*, tpempty, tpfull*/);
    } else {
      client_addr = calloc(1, sizeof(struct sockaddr_in));
      MALLOC_ERROR_HELPER(client_addr, "Calloc Error.");
    }
  }
  // this will never be executed
  shmdt(w->usershead);
  shmdt(w->topicshead);
  shmdt(w);
  semctl(mutex, 0, IPC_RMID); 
  exit(EXIT_SUCCESS);

}
