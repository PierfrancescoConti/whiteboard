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
    printf("-> \033[1;34mget [comment#]         \033[0m|-  It shows all the info about the comment you decide inside\n                          |   the current topic.\n");
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
//TODO: non funge
    strcpy(choice,"notify\0");
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);
    print_logo();
    printf("%s\n", buf);

    memset(buf, 0, buf_len);          // FLUSH
    memset(choice, 0, 32);          // FLUSH
    
    printf("\n> help\n");
    strcpy(choice,"help\0");
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);

    print_menu();


    //create topic//
    printf("\n> create topic\n");
    strcpy(choice,"create topic\0");
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);

    printf("Insert here Topic's Title and Content (Press Enter to send)\nTitle> Titolo Topic Test\n");

    send(socket_desc, "Titolo Topic Test\n",20,0);
    recv(socket_desc, buf, buf_len, 0);

    printf("Content> Contenuto Topic Test\n\n");
    send(socket_desc, "Contenuto Topic Test\n",22,0);
    recv(socket_desc, buf, buf_len, 0);

    printf("%s\n", buf);





    //list topics//
    printf("\n> list topics\n");
    strcpy(choice,"list topics\0");
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);

    printf("%s\n", buf);


    printf("\n> quit\n");
    strcpy(choice,"quit\0");
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);

    //memset(buf, 0, buf_len);          // FLUSH
    //memset(choice, 0, 32);          // FLUSH



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

     while(1){
     
        // initiate a connection on the socket
        ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
        ERROR_HELPER(ret, "Could not create connection");
        if (DEBUG) fprintf(stderr, "Connection established!\n");
        // loop del messaggio (inserisci i comandi corretti)
        /*
        
        printf("to login write authenticate or A\nto register write register o R\n> ");
        strcpy(init,"R\0");
        
        
        while((ret = send(socket_desc, init,strlen(init), 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(-1, "Cannot write to socket");
        }
        // riceve la risposta del messaggio A o R
        recv_bytes = recv(socket_desc, buf, buf_len, 0);
        // stampa la risposta del messaggio A o R ("Please insert credentials\nUsername:")
        printf("%s", buf);
        char username[32];
        strcpy(username, "user_test\0");
        
        // sending username
        send(socket_desc, username,strlen(username),0);
        // receiving password request ("Password: ")
        recv_bytes = recv(socket_desc, buf, buf_len, 0);
        printf("%s", buf);
        char password[32];
        strcpy(password, "test\0");
        // sending password
        send(socket_desc, password,strlen(password), 0);
        // Authenticated? Registrated?
        while ( (recv_bytes = recv(socket_desc, buf, buf_len, 0)) < 0 ) {
            if (errno == EINTR) continue;
            ERROR_HELPER(-1, "Cannot read from socket");
        }
        printf("%s\n", buf);
        if(!(strcmp(buf,"Invalid credentials.\0")==0 || strcmp(buf,"Username already taken.\0")==0)) break;
        
        
        printf("to login write authenticate or A\nto register write register o R\n> ");
        strcpy(init,"A\0");
        
        
        while((ret = send(socket_desc, init,strlen(init), 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(-1, "Cannot write to socket");
        }
        // riceve la risposta del messaggio A o R
        recv_bytes = recv(socket_desc, buf, buf_len, 0);
        // stampa la risposta del messaggio A o R ("Please insert credentials\nUsername:")
        printf("%s", buf);
        char username[32];
        strcpy(username, "user_test\0");
        
        // sending username
        send(socket_desc, username,strlen(username),0);
        // receiving password request ("Password: ")
        recv_bytes = recv(socket_desc, buf, buf_len, 0);
        printf("%s", buf);
        char password[32];
        strcpy(password, "test\0");
        // sending password
        send(socket_desc, password,strlen(password), 0);
        // Authenticated? Registrated?
        while ( (recv_bytes = recv(socket_desc, buf, buf_len, 0)) < 0 ) {
            if (errno == EINTR) continue;
            ERROR_HELPER(-1, "Cannot read from socket");
        }
        printf("%s\n", buf);
        if(!(strcmp(buf,"Invalid credentials.\0")==0 || strcmp(buf,"Username already taken.\0")==0)) break;
        */break;    //DEBUG: auth bypass (cancellare questa riga e decommentare)
    }
    // printf("Sending Ready\n");       //DEBUG
    
    
    client_loop(socket_desc);
}
