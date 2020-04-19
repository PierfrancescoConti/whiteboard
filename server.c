#include "global.h"
#include "whiteboard.h"

///////////////POSSIBILI ALTRI INCLUDE///////////////
int get_digit(char* buf, int i){    // i is the num's starting index
  char d='a';
  d=buf[i];
  if(!isdigit(d)) return -1;
  int number=0;
  while(d!='\0'){
    d=buf[i];
    // printf("d: %c\n", d);
    if(!isdigit(d)) break;
    
    int digit = d - '0';
    number = number*10 + digit;
    i++;
  }
  return number;
}


void app_loop(int shmidwb, int socket_desc, char* current_user){
  int ret, recv_bytes;
  int current_tp_id=-1;
  char* buf=(char*)malloc(1024*sizeof(char));
  size_t buf_len = 1024;
  char* resp=(char*)malloc(32768*sizeof(char));
  size_t resp_len = 32768;


  while(1){

    // printf("... entering app_loop\n");   //DEBUG
    memset(buf, 0, buf_len);          // FLUSH
    memset(resp, 0, resp_len);          // FLUSH

    while((recv(socket_desc, buf, buf_len, 0)) < 0 ) {
      if(errno == EINTR) continue;
      ERROR_HELPER(-1, "Cannot read from socket");
    }

    ////////// choose topic //////////
    if (strncmp(buf, "topic ",6) == 0) {
      char d='a';
      int number=get_digit(buf, 6);
      if(number==-1){
        strcpy(resp, "\033[41;1m   Invalid topic                                                                                        \033[0m\0");
      }
      else{
        // printf("num: %d\n", number);
        whiteboard* w = (whiteboard*) shmat(shmidwb, NULL, 0);
        w->topicshead = (topic*) shmat(w->shmidto, NULL, 0);
        w->usershead = (user*) shmat(w->shmidus, NULL, 0);

        topic* t=get_topic(w,number);
        

        if(t==NULL) strcpy(resp, "\033[41;1m   Invalid topic                                                                                        \033[0m\0");
        else{
          t->commentshead = (comment*) shmat(t->shmidcm, NULL, 0);
          int cuid=get_user_by_usname(w,current_user)->id;
          current_tp_id=t->id;    // important!
          if(int_in_arr(t->subscribers,cuid)) {
            strcpy(resp, tp_to_string(t,1));
            add_viewer(t,cuid);
            add_all_seen(t->commentshead,cuid);
            check_all_seen_by_all(t->subscribers, t->commentshead);

            //print_arr(t->viewers);  //DEBUG
          }
          else strcpy(resp, tp_to_string(t,0));
          strcat(resp, "\0");
          shmdt(t->commentshead); 
        }
      
        
        shmdt(w->usershead);
        shmdt(w->topicshead);
        shmdt(w);
      }
      printf("current_tp_id: %d\n",current_tp_id);    //DEBUG
      

    } 
    ////////// list topics //////////
    else if (strncmp(buf, "list topics",11) == 0) {
      whiteboard* w = (whiteboard*) shmat(shmidwb, NULL, 0);
      w->topicshead = (topic*) shmat(w->shmidto, NULL, 0);
      
      strcpy(resp, wb_to_string(w));
      strcat(resp, "\0");

      //printf("... printing wb\n");   //DEBUG
      //print_wb(w);   //DEBUG
      
      shmdt(w->topicshead);
      shmdt(w);
      // printf("%s\n", resp);   //DEBUG
    } 
    ////////// get comment //////////
    else if (strncmp(buf, "get ",4) == 0) {
      // do something
    } 
    ////////// status comment //////////
    else if (strncmp(buf, "status ",7) == 0) {
      // do something
    } 
    ////////// reply to comment //////////
    else if (strncmp(buf, "reply ",6) == 0) {
      if(current_tp_id!=-1){
        //printf("aaa\n");

        char d='a';
        int number=get_digit(buf, 6);
        if(number==-1){
          strcpy(resp, "\033[41;1m   Invalid comment                                                                                      \033[0m\0");
          send(socket_desc, resp,strlen(resp),0);
          recv(socket_desc, buf, buf_len, 0);
        }
        else{
          whiteboard* w = (whiteboard*) shmat(shmidwb, NULL, 0);
          w->topicshead = (topic*) shmat(w->shmidto, NULL, 0);

          topic* t=get_topic(w, current_tp_id);
          //create comment and add
          t->commentshead = (comment*) shmat(t->shmidcm, NULL, 0);
          comment* r=get_comment(t,number);    // check if number (reply_id) exists
          if(!(r==NULL)){
            char* content=(char*)malloc(1024*sizeof(char));

            send(socket_desc, "content\0",9,0);

            recv(socket_desc, content, 1024, 0);



            time_t date;
            time(&date);


            comment* last=get_last_comment(t);    // check if number (reply_id) exists
            comment* c=new_comment(last->id+1, current_user, date, content, number);
            push_comment(t,c);
            add_reply(r,c->id);
            memset(t->viewers, -1, MAX_REPLIES*sizeof(int));    //clear viewers
            print_arr(t->viewers);  //DEBUG


            //printf("RID: %d\n", c->in_reply_to);

            shmdt(t->commentshead);
            shmdt(w->topicshead);
            shmdt(w);


            free(content);

            strcpy(resp, "\033[42;1m   Comment added successfully!                                                                          \033[0m\0");
          }
          else{
            strcpy(resp, "\033[41;1m   Invalid comment                                                                                      \033[0m\0");
            send(socket_desc, resp,strlen(resp),0);
            recv(socket_desc, buf, buf_len, 0);
          }



          
        }
      }
      else{
        strcpy(resp, "\033[41;1m   At first you have to choose a topic.      (usage: topic [topic#])                                    \033[0m\0");
        
        send(socket_desc, resp,strlen(resp),0);
        recv(socket_desc, buf, buf_len, 0);
        
      }
    }
    ////////// create topic //////////
    else if (strncmp(buf, "create topic",12) == 0) {
      char* title=(char*)malloc(256*sizeof(char));
      char* content=(char*)malloc(1024*sizeof(char));

      send(socket_desc, "title\0",7,0);

      recv(socket_desc, title, 256, 0);

      send(socket_desc, "content\0",9,0);

      recv(socket_desc, content, 1024, 0);

      whiteboard* w = (whiteboard*) shmat(shmidwb, NULL, 0);
      w->topicshead = (topic*) shmat(w->shmidto, NULL, 0);
      w->usershead = (user*) shmat(w->shmidus, NULL, 0);

      time_t date;
      time(&date);
      topic* last=get_last_topic(w);
      topic* t=new_topic(last->id+1, current_user, title, content, date);

      int cuid= get_user_by_usname(w, current_user)->id;

      add_subscriber(t,cuid);
      add_subscription_entry(w,cuid,t->id);
      add_viewer(t,cuid);


      add_topic(w, t);

      shmdt(w->usershead);
      shmdt(w->topicshead);
      shmdt(w);


      free(content);
      free(title);

      strcpy(resp, "\033[42;1m   Topic created successfully!                                                                          \033[0m");
      strcat(resp, "\0");

    } 
    ////////// add comment //////////
    else if (strncmp(buf, "add comment",11) == 0) {
      if(current_tp_id!=-1){
        //printf("aaa\n");
        char* content=(char*)malloc(1024*sizeof(char));

        send(socket_desc, "content\0",9,0);

        recv(socket_desc, content, 1024, 0);

        whiteboard* w = (whiteboard*) shmat(shmidwb, NULL, 0);
        w->topicshead = (topic*) shmat(w->shmidto, NULL, 0);

        time_t date;
        time(&date);

        topic* t=get_topic(w, current_tp_id);
        //create comment and add
        t->commentshead = (comment*) shmat(t->shmidcm, NULL, 0);

        comment* last=get_last_comment(t);
        comment* c=new_comment(last->id+1, current_user, date, content,-1);
        push_comment(t,c);
        memset(t->viewers, -1, MAX_REPLIES*sizeof(int));    //clear viewers
        print_arr(t->viewers);  //DEBUG

        shmdt(t->commentshead);
        shmdt(w->topicshead);
        shmdt(w);


        free(content);

        strcpy(resp, "\033[42;1m   Comment added successfully!                                                                          \033[0m\0");
      }
      else{
        strcpy(resp, "\033[41;1m   At first you have to choose a topic.      (usage: topic [topic#])                                    \033[0m\0");
        
        send(socket_desc, resp,strlen(resp),0);
        recv(socket_desc, buf, buf_len, 0);
        
      }
      
    } 
    ////////// subscribe to topic //////////
    else if (strncmp(buf, "subscribe",9) == 0) {
      if(current_tp_id!=-1){
        whiteboard* w = (whiteboard*) shmat(shmidwb, NULL, 0);
        w->topicshead = (topic*) shmat(w->shmidto, NULL, 0);
        w->usershead = (user*) shmat(w->shmidus, NULL, 0);

        user* u=get_user_by_usname(w, current_user);
        //if(u==NULL) printf("NULL\n");     //DEBUG
        //printf("us: %s\n",u->username);     //DEBUG
        topic* t=get_topic(w, current_tp_id);
        //printf("us: %s  -  tp:%d\n",u->username,t->id);     //DEBUG
        //CHECK if already subscribed
        add_subscriber(t,u->id);
        add_subscription_entry(w,u->id,t->id);
        //printf("Subscribers:\n");     //DEBUG
        //printf("%d-%d\n",t->subscribers[0],t->subscribers[1]);     //DEBUG

        shmdt(w->usershead);
        shmdt(w->topicshead);
        shmdt(w);
        strcpy(resp, "\033[42;1m   Subscribed.                                                                                          \033[0m\0");
      }
      else strcpy(resp, "\033[41;1m   At first you have to choose a topic.      (usage: topic [topic#])                                    \033[0m\0");
    } 

    ////////// delete topic //////////
    else if (strncmp(buf, "delete topic ",13) == 0) {
      int number=get_digit(buf, 13);
      //printf("%d\n",number);    //DEBUG
      if(number==-1) strcpy(resp, "I don't understand which topic I should delete.\0");
      else{
        whiteboard* w = (whiteboard*) shmat(shmidwb, NULL, 0);
        w->topicshead = (topic*) shmat(w->shmidto, NULL, 0);

        topic* t= get_topic(w,number);
        if(!(t==NULL)){
          
          char cu[32];                //DEBUG       FAI ATTENZIONE AL '\n' NELL'USNAME
          strcpy(cu, current_user);   //DEBUG       quindi forse devi togliere tutto
          cu[strlen(cu)-1]='\0';      //DEBUG       anche questo
          //printf("%s--%s\n",t->author,cu);    //DEBUG
          if(!strcmp(t->author,cu)){
            delete_topic(w, number);
            strcpy(resp, "\033[42;1m   Deleted successfully.                                                                                 \033[0m\0");
          }
          else strcpy(resp, "\033[41;1m   Only topic's author can delete it.                                                                                \033[0m\0");
        }
        else strcpy(resp, "\033[41;1m   This topic does not exist.                                                                              \033[0m\0");

        shmdt(w->topicshead);
        shmdt(w);
      }
      
    }
    ////////// quit //////////
    else if (strncmp(buf, "quit",4) == 0) {
      break;
    }
    ////////// help //////////
    else /* default: */{
      strcpy(resp, "help\0");
    }
    
    send(socket_desc, resp,strlen(resp),0);

  }
  //printf("free\n");     //DEBUG

  free(buf);
  free(resp);
  return;
  // send whiteboard's message
  // receive command
  // switch case
}


////////////////////////////////////////// PROCESS ROUTINE //////////////////////////////////////////

void* connection_handler(int shmidwb, int socket_desc, struct sockaddr_in* client_addr) {

    int ret, recv_bytes;

    char* current_user=NULL;

    char* buf=(char*)malloc(1024*sizeof(char));
    size_t buf_len = 1024;

    // parse client IP address and port
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
/*
    // read message from client
    while((recv_bytes = recv(socket_desc, buf, buf_len, 0)) < 0 ) {
        if(errno == EINTR) continue;
        ERROR_HELPER(-1, "Cannot read from socket");
    }

    printf("%s\n", buf);


	///////////////////////////////REGISTER///////////////////////////////
    if (buf[0]=='R' || buf[0]=='r'){
      char* resp=NULL;
      while(1){
        resp=Register(shmidwb, socket_desc);

        if(resp!=NULL) break;
        // printf("Username already taken.\n");     //DEBUG
        send(socket_desc, "Username already taken.\0",32, 0);
      }
      //printf("out\n");     //DEBUG

  		send(socket_desc, resp,11, 0);
    }

	///////////////////////////////AUTH///////////////////////////////
    if (buf[0]=='A' || buf[0]=='a'){

      current_user=Auth(shmidwb, socket_desc);
      if(current_user==NULL){
        // printf("Invalid credentials.\n");     //DEBUG
        send(socket_desc, "Invalid credentials.\0",32, 0);
        free(buf);
        free(client_addr); // do not forget to free this buffer!
        ret = close(socket_desc);
      	ERROR_HELPER(ret, "Cannot close socket for incoming connection");
        //detach from shared memory
        //shmdt(w);
        printf("all closed\n");     //DEBUG
        connection_handler(shmidwb, socket_desc, client_addr);
        exit(1);
      }
      printf("Current_User: %s\n", current_user);     //DEBUG


      char* resp = current_user;
      replace_char(resp, '\n', '\0');
      char end='\0';    //????
      strncat(resp, &end, 1);
  		send(socket_desc, resp, 32, 0);

      app_loop(shmidwb, socket_desc, current_user);

    }
    */app_loop(shmidwb, socket_desc, "admin\n");   // DEBUG: auth bypass (cancellare questa riga e decommentare)
	// close socket
  //printf("closing....\n");     // DEBUG
	ret = close(socket_desc);
	ERROR_HELPER(ret, "Cannot close socket for incoming connection");
	free(buf);
  free(client_addr); // do not forget to free this buffer!
  //detach from shared memory
  //shmdt(w);
  printf("all closed\n");     // DEBUG
  exit(1);

}


/////////////////////////////////// MAIN ///////////////////////////////////
int main(int argc, char* argv[]) {
    int ret;
    int processID;
    int socket_desc, client_desc;


    // shared memory
    int shmidwb;

    // get shared memory
    // whiteboard
    if ((shmidwb = shmget(10000, sizeof(whiteboard), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    // attach to shared memory
    whiteboard* w = (whiteboard*) shmat(shmidwb, NULL, 0);

    w=create_wb(w);

    shmdt(w);

    // some fields are required to be filled with 0
    struct sockaddr_in server_addr = {0};

    int sockaddr_len = sizeof(struct sockaddr_in); // I will reuse it for accept()

    // initialize socket for listening
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    ERROR_HELPER(socket_desc, "Could not create socket");

    server_addr.sin_addr.s_addr = INADDR_ANY; // I want to accept connections from any interface
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

    /* I enable SO_REUSEADDR to quickly restart our server after a crash:
     * for more details, read about the TIME_WAIT state in the TCP protocol */
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "Cannot set SO_REUSEADDR option");

    // bind address to socket
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sockaddr_len);
    ERROR_HELPER(ret, "Cannot bind address to socket");

    // start listening
    ret = listen(socket_desc, MAX_CONN_QUEUE);
    ERROR_HELPER(ret, "Cannot listen on socket");

    // I allocate client_addr dynamically and initialize it to zero
    struct sockaddr_in* client_addr = calloc(1, sizeof(struct sockaddr_in));
    if (DEBUG) fprintf(stderr, "Server in listening mode...\n");


    while (1) {
        // accept incoming connection
        client_desc = accept(socket_desc, (struct sockaddr*) client_addr, (socklen_t*) &sockaddr_len);
        if (client_desc == -1 && errno == EINTR) continue; // check for interruption by signals
        ERROR_HELPER(client_desc, "Cannot open socket for incoming connection");

        if (DEBUG) fprintf(stderr, "Incoming connection accepted...\n");

        if ((processID = fork()) < 0) {
            perror("fork() failed");
            exit(1);
        }
        else if (processID == 0){  /* If this is the child process */

            connection_handler(shmidwb, client_desc, client_addr);

        }
        else client_addr = calloc(1, sizeof(struct sockaddr_in));
    }
    exit(EXIT_SUCCESS); // this will never be executed
}
