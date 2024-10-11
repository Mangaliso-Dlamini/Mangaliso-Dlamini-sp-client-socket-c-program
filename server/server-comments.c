// Cwk2: server.c - multi-threaded server using readn() and writen()

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include "rdwrn.h"

#include <time.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>

// thread function
void *client_handler(void *);

//Global variables
pthread_t threads[10];
int client_count = 0;
int server_socket;
struct timeval start_time; 

//function prototypes
void send_name_id(int);
void send_rand_arr(int);
void send_sys_info(int);
void send_file_list(int);

//SIGINT function(ctrl+c)
void sigint_handler(int signo){
	printf("\nReceived SIGINT. Exiting gracefully...\n");
	
	//closing server sockect
	close(server_socket);
	
	//handle connected clients	
	int i;
	for (i = 0; i < client_count; i++) {
        	pthread_join(threads[i], NULL);
    	}

	//get time at the end of the session
	struct timeval end_time;
	gettimeofday(&end_time, NULL);

	//calculating time elapsed and converting time microseconds
    	long long elapsed_time = (end_time.tv_sec * 1000000 + end_time.tv_usec) -
                             (start_time.tv_sec * 1000000 + start_time.tv_usec);
    	printf("Total execution time: %lld microseconds\n", elapsed_time);

    	exit(EXIT_SUCCESS);

}
//send file contents function
void send_file(int socket)
{
	//receiving filename length via socket
    	size_t filename_len;
    	readn(socket, (unsigned char *)&filename_len, sizeof(size_t));
    
    	if (filename_len == 0) {
        	fprintf(stderr, "Error: Received an empty filename.\n");
        	return;
    	}

	//allocating file name length dynamically
    	char *filename = malloc(filename_len);
    	if (filename == NULL) {
        	perror("Error allocating memory for filename");
        	return;
    	}
	//receiving filename via socket
    	readn(socket, (unsigned char *)filename, filename_len);

	//constructing file path
    	char filepath[1024];
    	snprintf(filepath, sizeof(filepath), "upload/%s", filename);

	//opening filename for reading
    	FILE *file = fopen(filepath, "rb");
    	free(filename); // Free dynamically allocated memory
    	if (file == NULL) {
		perror("Error opening file");
		size_t n = 0;
		writen(socket, (unsigned char *)&n, sizeof(size_t));
		return;
    	}
	
	//getting file size
    	fseek(file, 0, SEEK_END);
    	size_t filesize = ftell(file);
    	fseek(file, 0, SEEK_SET);

	//sending file size to client
    	writen(socket, (unsigned char *)&filesize, sizeof(size_t));

	//reading file content
    	size_t remaining = filesize;
    	while (remaining > 0) {
        	char buffer[1024];
        	size_t readsize = fread(buffer, 1, sizeof(buffer), file);
        	if (readsize <= 0) {
            		perror("Error reading file");
            		break;
        	}
		//sending file content
        	writen(socket, (unsigned char *)buffer, readsize);
        	remaining -= readsize;
    	}
	//close file
    	fclose(file);
}

// you shouldn't need to change main() in the server except the port number
int main(void)
{
     //signal handler
     signal(SIGINT, sigint_handler);

    int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t socksize = sizeof(struct sockaddr_in);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(50031);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
	perror("Failed to listen");
	exit(EXIT_FAILURE);
    }
    // end socket setup
    gettimeofday(&start_time, NULL);


    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    while (1) {
	printf("Waiting for a client to connect...\n");
	connfd =
	    accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	printf("Connection accepted...\n");
	printf("Connected to %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	pthread_t sniffer_thread;
        // third parameter is a pointer to the thread function, fourth is its actual parameter
	if (pthread_create
	    (&sniffer_thread, NULL, client_handler,
	     (void *) &connfd) < 0) {
	    perror("could not create thread");
	    exit(EXIT_FAILURE);
	}
	//Now join the thread , so that we dont terminate before the thread
	//pthread_join( sniffer_thread , NULL);
	printf("Handler assigned\n");
    }

    // never reached...
    // ** should include a signal handler to clean up
    exit(EXIT_SUCCESS);
} // end main()

// thread function - one instance of each for each connected client
// this is where the do-while loop will go
void *client_handler(void *socket_desc)
{
    //Get the socket descriptor
    int connfd = *(int *) socket_desc;

    
	do{
		//receive selected menu option
		int choice=0;
		readn(connfd, &choice, sizeof(choice));
	 	printf("Choice: %d\n", choice);
		
		//switch received option and call the corresponding functiom
		switch(choice){
			case 1:
				send_name_id(connfd);
				break;
			case 2:
				send_rand_arr(connfd);
				break;
			case 3: 
				send_sys_info(connfd);
				break;
			case 4:
				send_file_list(connfd);
				break;
			case 5:
				send_file(connfd);
				break;
			case 6:
				printf("Client Exiting\n");
				break;
			default: 
				printf("Invalid choice");//invalid selection error handling
				break;


		}
		//exit client if option 6 is selected
		if(choice == 6){
			break;

		}


	}while(1);

	shutdown(connfd, SHUT_RDWR); 
	close(connfd); 
	printf("Thread %lu exiting\n", (unsigned long) pthread_self());		

    // always clean up sockets gracefully
    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
}  // end client_handler()


//generate and send random array
void send_rand_arr(int socket){
	srand((unsigned)time(NULL)); //time used to generate random numbers

	int arr[5];

	//generate 5 random number and store in array (100-500)
	int i; 
	for(i=0; i < 5; i++){
		arr[i] = rand() % 401 +100;
	}
	
	//send array to client via socket
	size_t n = sizeof(arr);
	writen(socket, (unsigned char *) &n, sizeof(size_t));	
    	writen(socket, (unsigned char *) arr, n);	

}

//send name id with ip address prefix
void send_name_id( int socket){
	//hard code author name and student ID
	char name[] = "Mangaliso Linda Dlamini";
	char sid[] = "S2110978";
	char to_send[50];
	
	//getting server IP address
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	getpeername(socket, (struct sockaddr *)&addr, &addr_size);
	char *server_ip = inet_ntoa(addr.sin_addr);

 	//combining name, ID, IP adress to one string
   	sprintf(to_send, "%s %s %s", server_ip, name, sid);

	//sending combined string to client	
	size_t n = strlen(to_send) + 1;
	writen(socket, (unsigned char *) &n, sizeof(size_t));	
    	writen(socket, (unsigned char *) to_send, n);	  
}

//send uname function
void send_sys_info(int socket){
	//struct with system information/uname
	struct utsname systemInfo; 
	
	if(uname(&systemInfo) == -1){
		perror("uname");
		return;
	}

	//send uname to client via socket
	size_t payload_length = sizeof(struct utsname);
	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
   	writen(socket, (unsigned char *) &systemInfo, payload_length);
	
}

//send file list function
void send_file_list(int socket){
	//initialising the upload directory	
	DIR *dir = opendir("upload");

	//checking if directory exists
	struct stat info;
        int exists = stat("upload", &info) == 0 && S_ISDIR(info.st_mode);

	//send exist or not exist value to client via socket
	writen(socket, &exists, sizeof(exists));

	//Error handling if directory does not exist
	if(!exists){
		printf("Directory does not exist.\n");
		return;
	}	

	//read entries directory
	struct dirent *entry;
	char file_list[4096] = "";

	//combing directory entries into a string with "||" seperator
	while((entry = readdir(dir)) != NULL){
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
			strcat(file_list, entry->d_name);
			strcat(file_list, "||");
		}
	}
	//closing directory
	closedir(dir);

	//send combined string to client via socket
	size_t payload_length = strlen(file_list);
	writen(socket, (unsigned char *)&payload_length, sizeof(size_t));
    	writen(socket, (unsigned char *)file_list, payload_length);
}

