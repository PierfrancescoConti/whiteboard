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




void print_logo(){
    printf("\e[1;1H\e[2J");     //clean
    printf("\033[1;31m");
    //printf("\033[1;50m");     //Background
    //printf("\n\toooooo   oooooo     oooo  .o8                                          .o8  \n\t\033[1;34m `888.    `888.     .8'  \"888                                         \"888  \n\t\033[1;35m  `888.   .8888.   .8'    888oooo.   .ooooo.   .oooo.   oooo d8b  .oooo888  \n\t\033[1;36m   `888  .8'`888. .8'     d88' `88b d88' `88b `P  )88b  `888\"\"8P d88' `888  \n\t\033[1;37m    `888.8'  `888.8'      888   888 888   888  .oP\"888   888     888   888  \n\t\033[1;31m     `888'    `888'       888   888 888   888 d8(  888   888     888   888  \n\t\033[1;32m      `8'      `8'        `Y8bod8P' `Y8bod8P' `Y888\"\"8o d888b    `Y8bod88P\" \n\t                                                                            \n");
    printf("\n\toooooo   oooooo     oooo  .o8                                          .o8  \n\t `888.    `888.     .8'  \"888                                         \"888  \n\t\033[1;97m  `888.   .8888.   .8'    888oooo.   .ooooo.   .oooo.   oooo d8b  .oooo888  \n\t   `888  .8'`888. .8'     d88' `88b d88' `88b `P  )88b  `888\"\"8P d88' `888  \n\t    `888.8'  `888.8'      888   888 888   888  .oP\"888   888     888   888  \n\t\033[1;32m     `888'    `888'       888   888 888   888 d8(  888   888     888   888  \n\t      `8'      `8'        `Y8bod8P' `Y8bod8P' `Y888\"\"8o d888b    `Y8bod88P\" \n\t                                                                            \n");
    //printf("\n\toooooo   oooooo     oooo  .o8                                          .o8  \n\t\033[1;33m `888.    `888.     .8'  \"888                                         \"888  \n\t\033[1;33m  `888.   .8888.   .8'    888oooo.   .ooooo.   .oooo.   oooo d8b  .oooo888  \n\t\033[1;32m   `888  .8'`888. .8'     d88' `88b d88' `88b `P  )88b  `888\"\"8P d88' `888  \n\t\033[1;32m    `888.8'  `888.8'      888   888 888   888  .oP\"888   888     888   888  \n\t\033[1;36m     `888'    `888'       888   888 888   888 d8(  888   888     888   888  \n\t\033[1;36m      `8'      `8'        `Y8bod8P' `Y8bod8P' `Y888\"\"8o d888b    `Y8bod88P\" \n\t                                                                            \n");
    printf("\033[0m\n");
}


void print_menu(){      //help
    printf("\033[1;44m\033[1;97m   MENU:                                                                                                \033[0m\n\n");
    printf("-> \033[1;34mhelp                   \033[0m|-  It prints a help menu.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mlist topics            \033[0m|-  It shows all existing topics.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mcreate topic           \033[0m|-  You can create a new topic.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mtopic [topic#]         \033[0m|-  It shows the topic you decide that will become the current topic.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mdelete topic [topic#]  \033[0m|-  You can delete a topic if you are the owner/author.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34msubscribe              \033[0m|-  You subscribe to the current topic and will receive a notification\n                          |   about changes.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34madd comment            \033[0m|-  You can simply add a comment to a topic.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mreply [comment#]       \033[0m|-  You can write a comment as response to another comment.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mstatus [comment#]      \033[0m|-  It shows the status of the comment you decide inside the current topic.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mquit                   \033[0m|-  Well.... you quit.\n");
    printf("\n\033[1;44m                                                                                                        \033[0m\n\n");

}



void client_loop(int socket_desc){

    char* buf=(char*)malloc(32768*sizeof(char));
    size_t buf_len = 32768;

    char choice[32];
    strcpy(choice,"notify\0");
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);
    print_logo();
    printf("%s\n", buf);
 
    while(1){
        do{
            
            printf("\nNeed help? Write \033[34;1mhelp\033[0m.\n> ");
            printf("\033[34;1m");

            if(fgets(choice, sizeof(choice),stdin) != (char*)choice){
                fprintf(stderr,"Error while reading from stdin, exiting...\n");
                exit(EXIT_FAILURE);
            }
            choice[strlen(choice)-1]='\0';
            printf("\033[0m");

        }
        while(!(strcmp(choice,"help")==0 || strncmp(choice,"topic ",6)==0 || strcmp(choice,"list topics")==0 || strncmp(choice, "status ",7)==0 || strcmp(choice, "create topic") == 0 || strncmp(choice, "reply ",6) == 0 || strncmp(choice, "delete topic ",13) == 0 || strncmp(choice, "delete comment ",15) == 0 || strcmp(choice, "subscribe") == 0 || strcmp(choice, "add comment") == 0 || strcmp(choice, "quit") == 0));
        send(socket_desc, choice,strlen(choice),0);
        if (!strcmp(choice, "quit")) break;
        else if(!strcmp(choice, "create topic")){
            recv(socket_desc, buf, buf_len, 0);
            if(strncmp(buf, "title",5)){
                printf("errore in create topic (title)");
            }
            printf("Insert here Topic's Title and Content (Press Enter to send)\nTitle> ");
            if(fgets(buf, 256,stdin) != (char*)buf){
                fprintf(stderr,"Error while reading from stdin, exiting...\n");
                exit(EXIT_FAILURE);
            }
            buf[255]='\0';
            send(socket_desc, buf,256,0);

            recv(socket_desc, buf, buf_len, 0);
            if(strncmp(buf, "content",7)){
                printf("errore in create topic (content)");
            }

            printf("Content> ");
            if(fgets(buf, 1024,stdin) != (char*)buf){
                fprintf(stderr,"Error while reading from stdin, exiting...\n");
                exit(EXIT_FAILURE);
            }
            buf[1023]='\0';
            send(socket_desc, buf,1024,0);

        }
        else if(!strcmp(choice, "add comment")){
            recv(socket_desc, buf, buf_len, 0);
            if(strncmp(buf, "content",7)){
                send(socket_desc, "NO\0",3,0);
                printf("errore in add comment (content)");
            }
            else{
                printf("Insert here the Comment to the current Topic. (Press Enter to send)\nComment> ");
                if(fgets(buf, 1024,stdin) != (char*)buf){
                    fprintf(stderr,"Error while reading from stdin, exiting...\n");
                    exit(EXIT_FAILURE);
                }
                buf[1023]='\0';
                send(socket_desc, buf,1024,0);
            }

        }
        else if(!strncmp(choice, "reply ",6)){
            recv(socket_desc, buf, buf_len, 0);
            if(strncmp(buf, "content",7)){
                send(socket_desc, "NO\0",3,0);
                printf("errore in add comment (content)");
            }
            else{
                printf("Insert here the Comment to the current Topic. (Press Enter to send)\nComment> ");
                if(fgets(buf, 1024,stdin) != (char*)buf){
                    fprintf(stderr,"Error while reading from stdin, exiting...\n");
                    exit(EXIT_FAILURE);
                }
                buf[1023]='\0';
                send(socket_desc, buf,1024,0);
            }

        }
        recv(socket_desc, buf, buf_len, 0);
        if (!strncmp(buf, "help\0",4)){
            print_logo();
            print_menu();
        }
        // TODO: check other responses
        else{
            print_logo();
            printf("%s\n", buf);
        }

        
        memset(buf, 0, buf_len);          // FLUSH
        memset(choice, 0, 32);          // FLUSH

  
    } 

    free(buf);
    exit(EXIT_SUCCESS);
}



// TODO: rimpiazzare tutte le funzioni ad alto livello che non le vogliamo -> tipo fgets
int main(int argc, char* argv[]) {
    char init[16];
    int ret,recv_bytes=0;


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
        printf("\e[1;1H\e[2J");     //clean

        printf("\033[42;1m   Connection established!                                                                              \033[0m\n\n");
        // loop del messaggio (inserisci i comandi corretti)
        do{
        printf("to login write authenticate or A\nto register write register o R\n> ");
        if(fgets(init, sizeof(init),stdin) != (char*)init){
            fprintf(stderr,"Error while reading from stdin, exiting...\n");
            exit(EXIT_FAILURE);
        }
        init[strlen(init)-1]='\0';
        }
        while(!(strcmp(init,"authenticate")==0 || strcmp(init,"register")==0 || strcmp(init,"R")==0 || strcmp(init,"A")==0));
        // manda il messaggio A o R
        while((ret = send(socket_desc, init,strlen(init), 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(-1, "Cannot write to socket");
        }
        // riceve la risposta del messaggio A o R
        recv_bytes = recv(socket_desc, buf, buf_len, 0);
        // stampa la risposta del messaggio A o R ("\nPlease insert credentials\nUsername:")
        printf("%s", buf);
        char username[32];
        fgets(username, sizeof(username),stdin);
        replace_char(username, '\n', '\0');
        char end='\0';    //????
        strncat(username, &end, 1);
        if(!strcmp(username, "")){
          printf("You didn't insert anything.\n");
          exit(1);
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
        // printf("lens: %d - %d\n", strlen(username) ,strlen(password));   // DEBUG
        // sending password
        send(socket_desc, password,strlen(password), 0);
        // Authenticated? Registrated?
        while ( (recv_bytes = recv(socket_desc, buf, buf_len, 0)) < 0 ) {
            if (errno == EINTR) continue;
            ERROR_HELPER(-1, "Cannot read from socket");
        }
        
        if(!strcmp(buf,"Registration Done.\0")){
            printf("\n\033[42;1m   %s                                                                                 \033[0m\n\n", buf);
            exit(EXIT_SUCCESS);
        }
        printf("\n\033[41;1m   %s                                                                              \033[0m\n\n", buf);
        if((strcmp(buf,"Invalid credentials.\0")==0 || strcmp(buf,"Username already taken.\0")==0)) exit(1);;
    // printf("Sending Ready\n");       //DEBUG
    
    
    client_loop(socket_desc);
}
