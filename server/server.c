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
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>

void *client_handler(void *);

pthread_t threads[10];
int client_count = 0;
int server_socket;
struct timeval start_time; 

void send_name_id(int);
void send_rand_arr(int);
void send_sys_info(int);
void send_file_list(int);

void sigint_handler(int signo){
	printf("\nReceived SIGINT. Exiting gracefully...\n");

	close(server_socket);
		
	int i;
	for (i = 0; i < client_count; i++) {
        	pthread_join(threads[i], NULL);
    	}

	struct timeval end_time;
	gettimeofday(&end_time, NULL);

    	long long elapsed_time = (end_time.tv_sec * 1000000 + end_time.tv_usec) -
                             (start_time.tv_sec * 1000000 + start_time.tv_usec);
    	printf("Total execution time: %lld microseconds\n", elapsed_time);

    	exit(EXIT_SUCCESS);

}

void send_file(int socket)
{
    	size_t filename_len;
    	readn(socket, (unsigned char *)&filename_len, sizeof(size_t));
    
    	if (filename_len == 0) {
		fprintf(stderr, "Error: Received an empty filename.\n");
		return;
    	}

    	char *filename = malloc(filename_len);
    	if (filename == NULL) {
        	perror("Error allocating memory for filename");
        	return;
    	}
    	readn(socket, (unsigned char *)filename, filename_len);

    	char filepath[1024];
    	snprintf(filepath, sizeof(filepath), "upload/%s", filename);

    	FILE *file = fopen(filepath, "rb");
    	free(filename); 
    	if (file == NULL) {
		perror("Error opening file");
		size_t n = 0;
		writen(socket, (unsigned char *)&n, sizeof(size_t));
		return;
    	}

    	fseek(file, 0, SEEK_END);
    	size_t filesize = ftell(file);
    	fseek(file, 0, SEEK_SET);

    	writen(socket, (unsigned char *)&filesize, sizeof(size_t));

    	size_t remaining = filesize;
   	while (remaining > 0) {
		char buffer[1024];
		size_t readsize = fread(buffer, 1, sizeof(buffer), file);
		if (readsize <= 0) {
            		perror("Error reading file");
            		break;
        	}
        	writen(socket, (unsigned char *)buffer, readsize);
        	remaining -= readsize;
    	}

    	fclose(file);
}

int main(void)
{
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
    
    gettimeofday(&start_time, NULL);

    puts("Waiting for incoming connections...");
    while (1) {
	printf("Waiting for a client to connect...\n");
	connfd =
	    accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	printf("Connection accepted...\n");
	printf("Connected to %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	pthread_t sniffer_thread;
	if (pthread_create
	    (&sniffer_thread, NULL, client_handler,
	     (void *) &connfd) < 0) {
	    perror("could not create thread");
	    exit(EXIT_FAILURE);
	}

	printf("Handler assigned\n");
    }

    exit(EXIT_SUCCESS);
} 

void *client_handler(void *socket_desc)
{
    
    int connfd = *(int *) socket_desc;

    
	do{
		int choice;
		readn(connfd, &choice, sizeof(choice));
	 	printf("Choice: %d\n", choice);
		
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
				printf("Invalid choice\n");
				break;


		}
		if(choice == 6){
			break;

		}


	}while(1);

	shutdown(connfd, SHUT_RDWR); 
	close(connfd); 
	printf("Thread %lu exiting\n", (unsigned long) pthread_self());		

    shutdown(connfd, SHUT_RDWR);
    close(connfd);

    return 0;
}  

void send_rand_arr(int socket){
	srand((unsigned)time(NULL));

	int arr[5];

	int i; 
	for(i=0; i < 5; i++){
		arr[i] = rand() % 401 +100;
	}

	size_t n = sizeof(arr);
	writen(socket, (unsigned char *) &n, sizeof(size_t));	
    	writen(socket, (unsigned char *) arr, n);	

}


void send_name_id( int socket){

	char name[] = "Mangaliso Linda Dlamini";
	char sid[] = "S2110978";
	char to_send[50];

	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	getpeername(socket, (struct sockaddr *)&addr, &addr_size);
	char *server_ip = inet_ntoa(addr.sin_addr);

 
   	sprintf(to_send, "%s %s %s", server_ip, name, sid);

	size_t n = strlen(to_send) + 1;
	writen(socket, (unsigned char *) &n, sizeof(size_t));	
    	writen(socket, (unsigned char *) to_send, n);	  
}

void send_sys_info(int socket){
	struct utsname systemInfo;
	
	if(uname(&systemInfo) == -1){
		perror("uname");
		return;
	}

	size_t payload_length = sizeof(struct utsname);
	writen(socket, (unsigned char *) &payload_length, sizeof(size_t));
   	writen(socket, (unsigned char *) &systemInfo, payload_length);
	
}

void send_file_list(int socket){
	DIR *dir = opendir("upload");

	struct stat info;
        int exists = stat("upload", &info) == 0 && S_ISDIR(info.st_mode);

	writen(socket, &exists, sizeof(exists));

	if(!exists){
		printf("Directory does not exist.\n");
		return;
	}	

	struct dirent *entry;
	char file_list[4096] = "";

	while((entry = readdir(dir)) != NULL){
		if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
			strcat(file_list, entry->d_name);
			strcat(file_list, "||");
		}
	}
	closedir(dir);

	size_t payload_length = strlen(file_list);
	writen(socket, (unsigned char *)&payload_length, sizeof(size_t));
    	writen(socket, (unsigned char *)file_list, payload_length);
}

