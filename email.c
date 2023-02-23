#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>


#include<stdio.h>
#include<stdlib.h>
#include<string.h>



// UTILS-macros for socket status & management.
#define ISVALIDSOCKET(s)(s >= 0)
#define GETSOCKERRNO()(errno)
#define CLOSESOCKET(s)(close(s))



// SMTP_CODES
#define SMTP_INIT_SERVER_CODE "220"
#define SMTP_SUCCESS_CODE "250"
#define SMTP_CONNECTION_CLOSE_CODE "221"



// SMTP-server configugations
#define SMTP_PORT "25"
#define MAX_USER_INPUT_SIZE 1024
#define MAX_RECV_BUFFER_SIZE 2048
#define MAX_SEND_BUFFER_SIZE 2048
#define ERR_BUFFER_SIZE 248



// custom-err-codes
#define STORAGE_INIT_ERR "10"
#define MISC_ERR "111"
#define SUCCESS_CODE  "112"
#define IP_RESOLV_ERR "113"
#define CONN_LOST_ERR "114"


// misc-macros
#define SMTP_INIT main




// err-struct
struct MSG_{
    char  MSG_CODE[5];
    char MSG[ERR_BUFFER_SIZE];
};

struct MSG_* MSG_LOG = NULL;



// smtp-config-struct
typedef struct SMTP_{
    char SMTP_SERVER[512];
    char USER_NAME[512];
    char USER_PWD[512];
    char RECV_BUFFER[MAX_RECV_BUFFER_SIZE];
    char SEND_BUFFER[MAX_SEND_BUFFER_SIZE];
    char CLIENT_INPUT[MAX_USER_INPUT_SIZE];
    char SERVER_HOST[100];
    char SERVER_SERV[100];
    int CLIENT_FD;
}SMTP_;








// utils-signaturs.
SMTP_* INIT_SMTP_CLIENT();
int CLEANUP_SMTP_CLIENT(SMTP_* SMTP_CLIENT);
void prompt_get(const char* prompt,char* storage_buffer,const int buffer_size);
void prompt_msg(const char* err_msg,const char* msg_code,const int mType);
void init_SMTP_dialog(SMTP_* SMTP_CLIENT);
void wait_on_response(char* recv_buffer,const int SOCK,const int buffer_size,const char* EXcode);
void auth_smtp(SMTP_* SMTP_CLIENT);


int SMTP_INIT(){
    MSG_LOG = (struct MSG_*)malloc(sizeof(struct MSG_));
    

    // initilizing smtp-storage-bucket.
    SMTP_ *SMTP_CLIENT = INIT_SMTP_CLIENT();
    if(SMTP_CLIENT == NULL){
        prompt_msg("Smtp Client Storage cannot be initilised",STORAGE_INIT_ERR,0);
    }


    // initilise server & wait for (ready,220).
    init_SMTP_dialog(SMTP_CLIENT);

    // Send EHLO & wait on response 
    // get user-name from user & try authenticate.
    auth_smtp(SMTP_CLIENT);



    // cleaning-up storage-bucket.
    CLEANUP_SMTP_CLIENT(SMTP_CLIENT);

}





// Storage-bucket-allocation.
SMTP_* INIT_SMTP_CLIENT(){
    // allocating client-storage-memory
    SMTP_* TempStorageBucket = NULL;
    
    // mem-allocation.
    TempStorageBucket = (SMTP_*)malloc(sizeof(SMTP_));

    // controll-flow-exit.
    if(TempStorageBucket != NULL){
        prompt_msg("Storage Bucket Initilized.",SUCCESS_CODE,1);
        return TempStorageBucket;
    }else{
        return NULL;
    }
}






// Storage-memory-cleanup
int CLEANUP_SMTP_CLIENT(SMTP_* SMTP_CLIENT){
    // Storage-bucket validation is NULL.
    if(SMTP_CLIENT == NULL){
        prompt_msg("While Cleaning up client Storage bucket.",MISC_ERR,0);
        return -1;
    }else{
        prompt_msg("Storage Bucket clean-up Success.",SUCCESS_CODE,1);
        free(SMTP_CLIENT);
        return 0;
    }
}










void prompt_msg(const char* msg,const char* msg_code,const int mType){

    // buffer-validation
    if(MSG_LOG == NULL){
        fprintf(stderr,"ERR : ERR_LOG is null\n");
        exit(1);
    }

    // buffer-cleanup
    memset(MSG_LOG->MSG,0,ERR_BUFFER_SIZE);

    // setting-code-msg.
    strcpy(MSG_LOG->MSG_CODE,msg_code);
    strcpy(MSG_LOG->MSG,msg);

    
    switch(mType){
        case 0:
            fprintf(stderr,"ERR : %s : %s\n",MSG_LOG->MSG,MSG_LOG->MSG_CODE);
            break;
        case 1:
            fprintf(stderr,"INFO : %s : %s\n",MSG_LOG->MSG,MSG_LOG->MSG_CODE);
            break;
        
        default:
            fprintf(stderr,"MSG : %s : %s\n",MSG_LOG->MSG,MSG_LOG->MSG_CODE);
    }
    
        
    
}




// setting up server & waiting on initial response (ready,220).
void init_SMTP_dialog(SMTP_* SMTP_CLIENT){
    // getting-smtp-server-input
    prompt_get("SMTP Server",SMTP_CLIENT->SMTP_SERVER,MAX_USER_INPUT_SIZE);
    printf("%s\n",SMTP_CLIENT->SMTP_SERVER);


    // settin-up server 
    struct addrinfo hints;
    struct addrinfo *SMTP_PEER;

    memset(&hints,0,sizeof(hints));

  
    hints.ai_socktype = SOCK_STREAM;

    

    // resolving-server-ip
    prompt_msg("Resolving Server IP...",SUCCESS_CODE,1);
    
    sleep(2);
    if(getaddrinfo(SMTP_CLIENT->SMTP_SERVER,SMTP_PORT,&hints,&SMTP_PEER)){
        prompt_msg("Server cannot be resolved",IP_RESOLV_ERR,0);
        exit(1);
    }

    
    prompt_msg("IP Resolved",SUCCESS_CODE,1);
    sleep(2);

    getnameinfo(SMTP_PEER->ai_addr,SMTP_PEER->ai_addrlen,
                SMTP_CLIENT->SERVER_HOST,sizeof(SMTP_CLIENT->SERVER_HOST),
                SMTP_CLIENT->SERVER_SERV,sizeof(SMTP_CLIENT->SERVER_SERV),
                0);


    
    printf("Connecting... %s@%s\n",SMTP_CLIENT->SERVER_HOST,SMTP_CLIENT->SERVER_SERV);
    sleep(2);

    SMTP_CLIENT->CLIENT_FD = socket(SMTP_PEER->ai_family,SMTP_PEER->ai_socktype,SMTP_PEER->ai_protocol);

    if(!ISVALIDSOCKET(SMTP_CLIENT->CLIENT_FD)){
        prompt_msg("Client Socket Failed",MISC_ERR,0);
        exit(1);
    }


    if(!ISVALIDSOCKET(connect(SMTP_CLIENT->CLIENT_FD,SMTP_PEER->ai_addr,SMTP_PEER->ai_addrlen))){
        prompt_msg("Cannot be connected to SMTP server",CONN_LOST_ERR,0);
        exit(1);
    }


    



    // waiting for client Response.
    printf("\n");
    wait_on_response(SMTP_CLIENT->RECV_BUFFER,SMTP_CLIENT->CLIENT_FD,MAX_RECV_BUFFER_SIZE,SMTP_INIT_SERVER_CODE);







}



void prompt_get(const char* prompt,char* storage_buffer,const int buffer_size){
    memset(storage_buffer,0,buffer_size);
    printf("%s : ",prompt);
    fgets(storage_buffer,buffer_size,stdin);

    storage_buffer[strlen(storage_buffer) - 1] = 0;
}








void wait_on_response(char* recv_buffer,const int SOCK,const int buffer_size,const char* EXcode){
    memset(recv_buffer,0,buffer_size);
    recv(SOCK,recv_buffer,buffer_size,0);

    char errcode_response[4] = {0};

    for(int i=0; i<3; i++){
        errcode_response[i] = recv_buffer[i];
    }

    if(strncmp(errcode_response,EXcode,3) != 0){
        prompt_msg(recv_buffer,errcode_response,0);
        exit(1);
    }

    printf("S : %s\n",recv_buffer);
    return;
}







void auth_smtp(SMTP_* SMTP_CLIENT){
    // send EHLO & wait on response.
    memset(SMTP_CLIENT->SEND_BUFFER,0,MAX_SEND_BUFFER_SIZE);
    strcpy(SMTP_CLIENT->SEND_BUFFER,"EHLO SJPC\r\n");
    send(SMTP_CLIENT->CLIENT_FD,SMTP_CLIENT->SEND_BUFFER,strlen(SMTP_CLIENT->SEND_BUFFER),0);
    wait_on_response(SMTP_CLIENT->RECV_BUFFER,SMTP_CLIENT->CLIENT_FD,MAX_RECV_BUFFER_SIZE,SMTP_SUCCESS_CODE);

    strcpy(SMTP_CLIENT->SEND_BUFFER,"STARTTLS\r\n");
    send(SMTP_CLIENT->CLIENT_FD,SMTP_CLIENT->SEND_BUFFER,strlen(SMTP_CLIENT->SEND_BUFFER),0);
    wait_on_response(SMTP_CLIENT->RECV_BUFFER,SMTP_CLIENT->CLIENT_FD,MAX_RECV_BUFFER_SIZE,"220");


    strcpy(SMTP_CLIENT->SEND_BUFFER,"AUTH LOGIN\r\n");
    send(SMTP_CLIENT->CLIENT_FD,SMTP_CLIENT->SEND_BUFFER,strlen(SMTP_CLIENT->SEND_BUFFER),0);
    wait_on_response(SMTP_CLIENT->RECV_BUFFER,SMTP_CLIENT->CLIENT_FD,MAX_RECV_BUFFER_SIZE,"334");

}





// gmail-smtp-in.l.google.com



        // send initial diloage mgs.
        // get user & pwd from user & authenticate.
        // get from & to email from user. & send.
        // get subject. & send.
        // get message till "\n" send.
        // Quit connection & cleanup memory leaks.
