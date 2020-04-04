#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>

#include "global.h"
#include <ctype.h>



// string management
char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos){
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}



// TODO: rimpiazzare tutte le funzioni ad alto livello che non le vogliamo -> tipo fgets
int main(int argc, char* argv[]) {
    while(1){
     char init[16];
     int ret,recv_bytes,log=0;


    char buf[1024];
    size_t buf_len = sizeof(buf);
     // variables for handling a socket
     int socket_desc;
     struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

     // create a socket
     socket_desc = socket(AF_INET, SOCK_STREAM, 0);
     ERROR_HELPER(socket_desc, "Could not create socket");

     // set up parameters for the connection
     server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
     server_addr.sin_family      = AF_INET;
     server_addr.sin_port        = htons(SERVER_PORT); // don't forget about network byte order!

     // initiate a connection on the socket
     ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
     ERROR_HELPER(ret, "Could not create connection");

     if (DEBUG) fprintf(stderr, "Connection established!\n");

     // loop del messaggio (inserisci i comandi corretti)
     do{
     printf("per il login scrivi authenticate o A\nper registrarti scrivi register o R\n");
      if(fgets(init, sizeof(init),stdin) != (char*)init){
          fprintf(stderr,"Error while reading from stdin, exiting...\n");
          exit(EXIT_FAILURE);
      }
     init[strlen(init)-1]='\0';
   }while(!(strcmp(init,"authenticate")==0 || strcmp(init,"register")==0 || strcmp(init,"R")==0 || strcmp(init,"A")==0));


   // manda il messaggio A o R
    while((ret = send(socket_desc, init,strlen(init), 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(-1, "Cannot write to socket");
    }


            // riceve la risposta del messaggio A o R
            recv_bytes = recv(socket_desc, buf, buf_len, 0);
            // stampa la risposta del messaggio A o R ("Please insert credentials\nUsername:")
            printf("%s", buf);

            char username[32];
            fgets(username, sizeof(username),stdin);
            replace_char(username, '\n', '\0');
            char end='\0';    //????
            strncat(username, &end, 1);
            if(!strcmp(username, "")){
              printf("You didn't insert anything.\n");
              break;
            }

            // sending username
            send(socket_desc, username,strlen(username),0);

            // receiving password request ("Password: ")
            recv_bytes = recv(socket_desc, buf, buf_len, 0);

            printf("%s", buf);

            char password[32];
            fgets(password, sizeof(password),stdin);
            replace_char(password, '\n', '\0');
            strncat(password, &end, 1);
            printf("lens: %d - %d\n", strlen(username) ,strlen(password));

            // sending password
            send(socket_desc, password,strlen(password), 0);

            // Authenticated? Registrated?
            while ( (recv_bytes = recv(socket_desc, buf, buf_len, 0)) < 0 ) {
                    if (errno == EINTR) continue;
                    ERROR_HELPER(-1, "Cannot read from socket");
            }
            printf("%s\n", buf);

            if(!(strcmp(buf,"Invalid credentials.\0")==0 || strcmp(buf,"Username already taken.\0")==0)) break;
    }
}
