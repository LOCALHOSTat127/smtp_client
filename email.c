#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define MAXINPUT 512
#define MAXRESPONSE 1024
#define PORT "25"
#define INIT_RES_CODE "220"
#define SUCCESS_RES_CODE "250"
#define SMTP_BY_CODE "221"




void get_input(const char* Prompt_msg,char *Buffer_ptr,const int buffer_size){
    printf("%s : ",Prompt_msg);
    memset(Buffer_ptr,0,buffer_size);
    fgets(Buffer_ptr,(buffer_size-1),stdin);
    Buffer_ptr[strlen(Buffer_ptr) - 1] = 0;
}





int connect_SMTP(const char *SMTP_SERVER, const char *SMTP_PORT){
    struct addrinfo hints;
    struct addrinfo *SMTP_PEER;
    memset(&hints,0,sizeof(hints));
  

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;


    if(getaddrinfo(SMTP_SERVER,SMTP_PORT,&hints,&SMTP_PEER)){
        fprintf(stderr,"ERR : SMTP server cannot be resolved\n");
        exit(1);
    }


    char SERVER_IP[100];
    char SERVER_SERV[100];
    getnameinfo(SMTP_PEER->ai_addr,SMTP_PEER->ai_addrlen,
                SERVER_IP,sizeof(SERVER_IP),
                SERVER_SERV,sizeof(SERVER_SERV),
                NI_NUMERICHOST);
    

    printf("Trying to connect %s@%s...\n",SERVER_IP,SERVER_SERV);
    sleep(2);
    int  clientSocketFD = socket(SMTP_PEER->ai_family,SMTP_PEER->ai_socktype,SMTP_PEER->ai_protocol);
    int clientSocketTemp = connect(clientSocketFD,SMTP_PEER->ai_addr,SMTP_PEER->ai_addrlen);

    if(!ISVALIDSOCKET(clientSocketTemp)){
        fprintf(stderr,"ERR : Cannot connect to SMTP server\n");
        exit(0);
    }

    printf("INFO : Connection Successfull\n\n");
    freeaddrinfo(SMTP_PEER);
    return clientSocketFD;
}
    



int wait_on_response(const int clientSocket,const char* RES_CODE){
    char res_buffer[MAXINPUT];
    // memset(res_buffer,0,MAXINPUT);

    recv(clientSocket,res_buffer,MAXINPUT,0);

    // if(strlen(res_buffer) <= 0){
    //     fprintf(stderr,"ERR : Connection Closed by SMTP server.\n");
    //     return -1;    
    // }


    // if(strncmp(res_buffer,RES_CODE,3) != 0){
    //     fprintf(stderr,"ERR : Connection Closed by SMTP server.\n");
    //     fprintf(stderr,"S : %s\n",res_buffer);
    //     return -1;
    // }

    printf("S : %s",res_buffer);

    return 0;
}


int send_msg(const int SOCK,const char* MSG,...){
    printf("C : %s",MSG);
    if(send(SOCK,MSG,strlen(MSG),0)){
        return 0;
    }    
}



void open_mail_box(const int SOCK){
    char client_input[MAXINPUT];
    char SEND_MSG[MAXINPUT];
    memset(client_input,0,MAXINPUT);
    memset(SEND_MSG,0,MAXINPUT);

    // get_input("\n> MAIL FROM",client_input,MAXINPUT);
    // strcpy(SEND_MSG,"MAIL FROM:");
    // strcat(SEND_MSG,"<");
    // strcat(SEND_MSG,client_input);
    // strcat(SEND_MSG,">");    
    // send_msg(SOCK,"MAIL FROM:<sahiljoshi6378@gmail.com>");
    wait_on_response(SOCK,SUCCESS_RES_CODE);

}




int main(){
    char SMTP_SERVER[MAXINPUT];
    get_input("SMTP-server",SMTP_SERVER ,MAXINPUT);
    printf("Resolving SMTP Server %s@%s...\n",SMTP_SERVER,PORT);

    SOCKET clientSocket = connect_SMTP(SMTP_SERVER,PORT);
    
   
    wait_on_response(clientSocket,INIT_RES_CODE);
    send_msg(clientSocket,"HELO SJ-PC\r\n");
    wait_on_response(clientSocket,SUCCESS_RES_CODE);
    open_mail_box(clientSocket);


}