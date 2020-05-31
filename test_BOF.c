#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
    //printf("\e[1;1H\e[2J");     //clean
    printf("\n\n\033[1;31m");
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
    printf("-> \033[1;34madd thread             \033[0m|-  You can simply add a comment to a topic.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mreply [comment#]       \033[0m|-  You can write a comment as response to another comment.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mstatus [comment#]      \033[0m|-  It shows the status of the comment you decide inside the current topic.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mlink [topic#] [topic#] \033[0m|-  You can refer to a thread contained in another topic.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mprint link [link#]     \033[0m|-  It loads the thread and its messages to which the link refers.\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("-> \033[1;34mquit                   \033[0m|-  Well.... you quit.\n");
    printf("\n\033[1;44m                                                                                                        \033[0m\n\n");

}







void client_loop(int socket_desc){

    char* buf=(char*)malloc(32768*sizeof(char));
    MALLOC_ERROR_HELPER(buf, "Malloc Error.");
    size_t buf_len = 32768;

    char choice[32];



    strcpy(choice,"notify\0");
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);
    print_logo();
    printf("%s\n", buf);

    memset(buf, 0, buf_len);          // FLUSH
    memset(choice, 0, 32);          // FLUSH

    int i;
    //Fill and exceed topics' memory creating topics//
    for(i=0;i<128;i++){
        strcpy(choice,"create topic\0");
        send(socket_desc, choice,strlen(choice),0);
        recv(socket_desc, buf, buf_len, 0);


        send(socket_desc, "Titolo Topic Test\n",20,0);
        recv(socket_desc, buf, buf_len, 0);

        send(socket_desc, "Contenuto Topic Test\n",22,0);
        recv(socket_desc, buf, buf_len, 0);


    }

    printf("%s\n", buf);

    memset(buf, 0, buf_len);          // FLUSH
    memset(choice, 0, 32);          // FLUSH

        printf("\n\n\n");

    //choose topic 1//
    printf("\n> \033[34;1mtopic 1\033[0m\n");
    strcpy(choice,"topic 1\0");
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);

    printf("%s\n", buf);

    memset(buf, 0, buf_len);          // FLUSH
    memset(choice, 0, 32);          // FLUSH

    printf("\n\n\n");

    //Fill and exceed a topic's memory creating comments//
    for(i=0;i<128;i++){
        strcpy(choice,"add thread\0");
        send(socket_desc, choice,strlen(choice),0);
        recv(socket_desc, buf, buf_len, 0);
        if(strncmp(buf, "content",7)){
            send(socket_desc, "NO\0",3,0);
        }
        else{

            send(socket_desc, "Comment Test\n",22,0);
        }
        recv(socket_desc, buf, buf_len, 0);


    }
    printf("%s\n", buf);

    memset(buf, 0, buf_len);          // FLUSH
    memset(choice, 0, 32);          // FLUSH

    printf("\n\n\n");



    //quit//
    printf("\n> \033[34;1mquit\033[0m\n");
    strcpy(choice,"quit\0");
    send(socket_desc, choice,strlen(choice),0);
    //recv(socket_desc, buf, buf_len, 0);

    //memset(buf, 0, buf_len);          // FLUSH
    //memset(choice, 0, 32);          // FLUSH


    free(buf);
    exit(EXIT_SUCCESS);
}



















int main(int argc, char* argv[]) {
    char choice[32];
    int ret=0;


    char buf[1024];
    size_t buf_len = sizeof(buf);
    // variables for handling a socket
    int socket_desc;
    struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

    int i;
    //Fill and exceed users' memory creating users//
    for(i=0;i<128;i++){
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
    

        // loop del messaggio (inserisci i comandi corretti)
        //REGISTER//
        strcpy(choice,"R\0");
        send(socket_desc, choice,strlen(choice),0);
        recv(socket_desc, buf, buf_len, 0);
        //Username
        strcpy(choice,"user_test");
        int len=strlen(choice);
        if(i!=0){
            sprintf(choice+len,"%d",i);
        }
        
        strcat(choice,"\0");
        send(socket_desc, choice,strlen(choice),0);
        recv(socket_desc, buf, buf_len, 0);
        //Password
        strcpy(choice,"test\0");
        send(socket_desc, choice,strlen(choice),0);
        recv(socket_desc, buf, buf_len, 0);
    }
    printf("\n\n\033[41;1m%s                                                                                 \033[0m\n", buf);

    memset(buf, 0, buf_len);          // FLUSH
    memset(choice, 0, 32);          // FLUSH

    printf("\n\n\n");

    sleep(1);
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

    printf("\n\033[42;1m   Connection established!                                                                              \033[0m\n\n");

    //AUTHENTICATE//
    printf("\n\033[44;1m   AUTHENTICATION   \033[0m\n");
    strcpy(choice,"A\0");
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);
    printf("%s", buf);
    
    //Username
    srand(time(NULL));
    strcpy(choice,"user_test");
    int len=strlen(choice);
    sprintf(choice+len,"%d",rand()%((120+1)-2) + 2);
    strcat(choice,"\0");

    printf("%s\n", choice);
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);
    printf("%s", buf);
    //Password
    strcpy(choice,"test\0");
    printf("%s\n", choice);
    send(socket_desc, choice,strlen(choice),0);
    recv(socket_desc, buf, buf_len, 0);
    sleep(1);
    
    client_loop(socket_desc);
}
