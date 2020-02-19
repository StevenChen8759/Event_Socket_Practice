//Code Reference: https://blog.csdn.net/a2796749/article/details/79428755

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <event.h>
#include <errno.h>
//#include <event_struct.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
 
#define PORT        502		//Define your own port number
#define BACKLOG     5		//Size of unconnected queue
#define MEM_SIZE    1024        //Receive data buffer size
 
struct event_base* base;	//global event base	        

struct sockEvent{		//Declare self-defined sockEvent structure

    struct event* readEvent;
    struct event* writeEvent;
    struct event* closeEvent;
    char* buffer;
    struct list* sockset;
};

struct list{			//Self-defined link-list structure

    int sockfd;			//Socket file descriptor
    struct sockEvent *se;	//Socket event pointer
    struct list* next;  	//Next node
};	
 
//As the connection closed, delete from base and free it
void releaseSockEvent(struct sockEvent* ev){ 

    printf("Connection closed, callback:releaseSockEvent\n");

    event_del(ev->readEvent);
    event_del(ev->closeEvent);
    free(ev->readEvent);
    free(ev->writeEvent);
    free(ev->closeEvent);
    free(ev->buffer);
    free(ev);
}

//Close the socket event
void closeConnEvent(int sock, short event, void* arg){ 

    printf("Callback:closeConnEvent...\n");

}

//Keyboard Interrupt Handler
void keyIntHandler(int signo, short events, void* arg){

    struct list *Sockset = (struct list*) arg, *ptr, *temp; //socket file descriptor set
    struct sockEvent *septr;

    printf("\n");
    printf("Receive signal %d\n", signo);
    
//------------------Socket List Freeup------------------
    //close all connection
    ptr = Sockset->next;
    while(ptr != NULL){
	temp = ptr;
        close(ptr->sockfd);
	printf("Close sockfd:%d", ptr->sockfd);
	printf("\n");
        
        septr = ptr->se;
	event_del(septr->readEvent);
	event_del(septr->closeEvent);
	free(septr->readEvent);
	free(septr->writeEvent);
	free(septr->closeEvent);
	free(septr->buffer);
	free(septr);
        printf("Delete the socket evnet...");
	printf("\n");

	ptr = ptr->next;
        free(temp);
    }

    close(Sockset->sockfd);
    printf("Close main socket:%d", Sockset->sockfd);
    printf("\n");

    free(Sockset);

//-------------------Event Base Break-------------------
    event_base_loopbreak(base);  //Exit loop immediately

}

//Socket response callback function
void handleWrite(int sock, short event, void* arg) {

    char* buffer = (char*)arg;
    send(sock, buffer, strlen(buffer), 0);
 
    printf("Response:%s\n", buffer);

    free(buffer);	//clean the buffer after writing
}

//Socket receive callback function, sock->socket_fd, *arg->socketEvnet for a connection
void handldRead(int sock, short event, void* arg){

    struct event* writeEvent;	//Declare a new event for writing
    int size;			//Store received data size
    
    struct sockEvent* ev = (struct sockEvent*)arg;
    char *resp;

    struct list *ptr = ev->sockset, *temp;

    //Dynamic allocation for data receive bufferring
    ev->buffer = (char*)malloc(MEM_SIZE);   
    bzero(ev->buffer, MEM_SIZE);	   //Set value to zero

    //Receive Data
    size = recv(sock, ev->buffer, MEM_SIZE, 0);

    printf("receive data:%s, size:%d\n", ev->buffer, size);

    if (size == 0) //client has send FIN(Written by original code)
    {
        printf("Client connection aborted (recv() return 0)\n");
        releaseSockEvent(ev);
        
	//offset to the position should be remove 
        while (ptr->sockfd != sock){
	    temp = ptr;   
            ptr = ptr->next;
        }

        printf("find socket fd position in link list...\n");

	//free the sockfd storage space
        temp->next = ptr->next;		//change the link
	free(ptr);			//free the space
	printf("free the space where ptr point to...\n");

        close(sock);
	printf("Close the socket...\n");
        return;
    }
    else if (size == -1){
	//Receive Error Handler
	printf("Receive Error!!!\n");
        printf("socket() failed: %s\n", strerror(errno));
        
    }

    //Do the service and Edit the server response message
    if (strncmp(ev->buffer,"ClientCRTS",strlen("ClientCRTS")) == 0 && strlen(ev->buffer) == strlen("ClientCRTS")){
	printf("ServerCCTS!\n");

        //Edit the response
        resp = (char*) calloc(strlen("ServerCCTS"), sizeof(char));
        strncpy(resp, "ServerCCTS", strlen("ServerCCTS"));
    }
    else if (strncmp(ev->buffer,"uart",strlen("uart")) == 0){

	printf("Transfer message to UART...\n");

	//implement uart manipulation here
	printf("Message transferred to UART is %s\n", (ev->buffer + strlen("uart")));

	//Edit the response
	if (strlen(ev->buffer) - strlen("uart") == 0){ //Prefix only, not valid
	    
	    resp = (char*) calloc(strlen("Please input message after prefix \"uart\"..."), sizeof(char));
            strncpy(resp, "Please input message after prefix \"uart\"...", strlen("Please input message after prefix \"uart\"..."));
        }
	else {
	    resp = (char*) calloc(strlen(ev->buffer) - strlen("uart"), sizeof(char));
            strncpy(resp, (ev->buffer + strlen("uart")), (strlen(ev->buffer) - strlen("uart")));
    	}
    }
    else if (strncmp(ev->buffer,"help",strlen("help")) == 0 && strlen(ev->buffer) == strlen("help")){
    	
	printf("Send help response...\n");

	//Edit the response
	resp = (char*) calloc(strlen("Input vaild instruction to make server do something..."), sizeof(char));
        strncpy(resp, "Input vaild instruction to make server do something...", strlen("Input vaild instruction to make server do something..."));

    }
    else if (strncmp(ev->buffer,"close",strlen("close")) == 0 && strlen(ev->buffer) == strlen("close")){
    	
	printf("Close the connection by the request from user...\n");
	releaseSockEvent(ev);

        //offset to the position should be remove 
        while (ptr->sockfd != sock){
	    temp = ptr;   
            ptr = ptr->next;
        }

        printf("find socket fd position in link list...\n");

	//free the sockfd storage space
        temp->next = ptr->next;		//change the link
	free(ptr);			//free the space
	printf("free the space where ptr point to...\n");

        close(sock);
	printf("Close the socket...\n");

	return; //No response for this input
    }
    else{

	//Edit the response
	resp = (char*) calloc(strlen("Denied Request..."), sizeof(char));
        strncpy(resp, "Denied Request...", strlen("Denied Request..."));
    }

    //printf("%s\n", resp);

    //add event to base to send the received data
    event_set(ev->writeEvent, sock, EV_WRITE, handleWrite, resp); //ev->buffer for request sendin
    event_base_set(base, ev->writeEvent);
    event_add(ev->writeEvent, NULL);
}
 
//Accept new connection callback
void handleAccept(int sock, short event, void* arg){

    struct sockaddr_in cli_addr;  //For new connection info storage
    int newfd;			  //New connection file descriptor
    socklen_t sinSize;		  //For connection info size
    
    struct sockEvent* ev;         //new socket event pointer for dynamically

    struct list *Sockset = (struct list*) arg, *ptr;
    
    //New event for handing new connection
    ev = (struct sockEvent*) malloc(sizeof(struct sockEvent));	  //SockEvent dynamic allocation
    ev->readEvent = (struct event*)malloc(sizeof(struct event));  //Dynamic allocation for read
    ev->writeEvent = (struct event*)malloc(sizeof(struct event)); //Dynamic allocation for write
    ev->closeEvent = (struct event*)malloc(sizeof(struct event)); //Dynamic allocation for close
    sinSize = sizeof(struct sockaddr_in);			  //get and store size of connection info
    
    printf("Accept a new connection...\n");

    //Socket accept and set to non-blocking
    newfd = accept(sock, (struct sockaddr*)&cli_addr, &sinSize);
    fcntl(newfd, F_SETFL, O_NONBLOCK);

    //Add new file descriptor into the tail link list
    ptr = Sockset;
    while (ptr->next != NULL)  ptr = ptr->next; 	//move to the tail

    ptr->next = (struct list*) malloc(sizeof(struct list)); //Memory allocation
    ptr = ptr->next; 					//move to new allocated space
    bzero(ptr, sizeof(struct list));
    ptr->sockfd = newfd;				//Store the file descriptor value
    ptr->se = ev;					//Point to correspond socket event
    
    ev->sockset = Sockset;				

    //set the new coming connection event - readevent
    event_set(ev->readEvent, newfd, EV_READ|EV_PERSIST, handldRead, ev);
    event_base_set(base, ev->readEvent);	//bind to global event base
    event_add(ev->readEvent, NULL);		//Add to readEvent
    printf("readEvent is added...\n");

    //set the new connection closed event - close event
    event_set(ev->closeEvent, newfd, EV_FINALIZE, closeConnEvent, ev);
    event_base_set(base, ev->closeEvent);	//bind to global event base
    event_add(ev->closeEvent, NULL);		//Add to closeEvent
    printf("closeEvent is added...\n");

    printf("End of accepting a new connection...\n");
    //close(Sockset->sockfd);

}
 
int main(int argc, char* argv[])
{

    int i;
    const char **methods = event_get_supported_methods();
    printf("Starting Libevent %s.  Available methods are:\n",
            event_get_version());
    for (i=0 ; methods[i] != NULL ; ++i) {
        printf("    %s\n", methods[i]);
    }

 
//-----------------------------Socket configuration-----------------------------
    struct sockaddr_in serverAddr;	//Declare server
    int sock;				//Socket file descriptor
    struct list *Sockset;	//Socket fd link list storage

    sock = socket(AF_INET, SOCK_STREAM, 0);	//Open the socket

    int on = 1;					//Set reusable with 4 Byte parameter
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

    //memset(&serverAddr, 0, sizeof(serverAddr));  //Set all content to zero
    bzero(&serverAddr, sizeof(serverAddr));	   //This method is also available
    
    //Set binding address, port and its type
    serverAddr.sin_family = AF_INET;		   //IPv4	
    serverAddr.sin_port = htons(PORT);		   //defined upward
    serverAddr.sin_addr.s_addr = INADDR_ANY;	   //0.0.0.0

    //Bind the socket and listen
    bind(sock, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr));
    if(!listen(sock, BACKLOG)) printf("Echo server is listening now, port = %d\n", PORT);
    else {
    	printf("Echo server listening error...\n");
    }

    Sockset = (struct list*) malloc(sizeof(struct list));
    bzero(Sockset, sizeof(struct list));
    Sockset->sockfd = sock;

 
//---------------------------Config event-----------------------------
    struct event listenEvent, closeEvent;	//event declaration
    base = event_base_new();			//Create new EventBase

    i = event_base_get_features(base);

    printf("Event Base feature BitMask: 0x%X\n", i);

    //conbine listenEvent and it's callback function
    event_set(&listenEvent, sock, EV_READ|EV_PERSIST, handleAccept, Sockset);
    event_base_set(base, &listenEvent);	//bind to global event base
    event_add(&listenEvent, NULL);	//add event
    printf("Add evsocket...\n");

    //set the new connection closed event - close event
    /*event_set(&closeEvent, sock, EV_FINALIZE, closeConnEvent, NULL);
    event_base_set(base, &closeEvent);	//bind to global event base
    event_add(&closeEvent, NULL);		//Add to closeEvent
    printf("evsock closeEvent is added...\n");*/

//--------------------KeyboardInterrupt signal config--------------------
    struct event *kbint;	//KeyboardIntterrupt Event
    int signo = 2;		//2 stand for SIGINT

    //KeyboardInterrupt
    kbint = evsignal_new(base, signo, keyIntHandler, Sockset);
    evsignal_add(kbint, NULL);
    printf("Add evsignal...\n");
  
    printf("Now dispatch the base...\n");
    event_base_dispatch(base);		//start base
   
    printf("End of the program...\n");

    return 0;
}
