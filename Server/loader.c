#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
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
        printf("%s\n",path);

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



int main(int argc, char* argv[]) {
    sleep(1);

    char choice[32];
    int ret=0, id;
  
    size_t b_len = 32;

    char username[32], password[32];


    char buf[1024];
    size_t buf_len = sizeof(buf);
    // variables for handling a socket
    int socket_desc;
    struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0

    decrypt("saved_dumps/users_dump.gpg");

    FILE * fus= fopen("saved_dumps/users_dump", "r");

    
    //Fill and exceed users' memory creating users//
    while(!feof(fus)){
        
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
    

        memset(username, 0, b_len); // FLUSH

        memset(password, 0, b_len); // FLUSH

        fscanf(fus,"%d\n%s\n%s\n\n",&id,username,password);

        strcat(username, "\0");
        strcat(password, "\0");
        // loop del messaggio (inserisci i comandi corretti)
        //REGISTER//

        strcpy(choice,"R\0");
        send(socket_desc, choice,strlen(choice),0);
        recv(socket_desc, buf, buf_len, 0);
        //Username
                
        send(socket_desc, username,strlen(username),0);
        recv(socket_desc, buf, buf_len, 0);
        //Password
        send(socket_desc, password,strlen(password),0);
        recv(socket_desc, buf, buf_len, 0);

    }
    

    fclose(fus);
    rmdec("saved_dumps");
    exit(EXIT_SUCCESS);
    
}
