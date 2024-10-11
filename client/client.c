#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "rdwrn.h"

#include <sys/utsname.h>
#include <fcntl.h>

void display_menu(){
	printf("1. Get and display name and student ID\n");
    	printf("2. Get and display random numbers\n");
    	printf("3. Get and display server uname information\n");
    	printf("4. Get and display file and directory names in the 'upload' directory\n");
    	printf("5. Copy file from 'upload' directory\n");
	printf("6. Exit\n");
}

void get_name_id(int socket)
{
    char to_recv[50];
    size_t k;

    readn(socket, (unsigned char *) &k, sizeof(size_t));	
    readn(socket, (unsigned char *) to_recv, k);

    printf("Name and ID with IP prefix: \n%s\n", to_recv);
    printf("Received: %zu bytes\n\n", k);
} 

void get_rand_arr(int socket){
	int arr[5];

	size_t k;

    	readn(socket, (unsigned char *) &k, sizeof(size_t));	
    	readn(socket, (unsigned char *) arr, k);
    	
	int i;
	int sum = 0;
	printf("Element of array: [ ");
	for(i=0; i < 5; i++){
		printf("%d ", arr[i]);
		sum += arr[i];
	}
	printf("]\n");
	printf("Sum of array elements: %d\n\n", sum);

}

void get_sys_info(int socket){
	struct utsname systemInfo;	
	
	size_t payload_length;
	
	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));	   
    	readn(socket, (unsigned char *) &systemInfo, payload_length);

	printf("\nOperating System: %s\n", systemInfo.sysname);
	printf("Release: %s\n", systemInfo.release);
	printf("Version: %s\n\n", systemInfo.version);
}

void get_file_list(int socket){
	int result;
	readn(socket, &result, sizeof(result));

	if(!result){
		printf("Directory does not exist.\n\n");
		return;
	}

	char file_list[4096];
    	size_t k;

	readn(socket, (unsigned char *) &k, sizeof(size_t));	
	readn(socket, (unsigned char *) file_list, k);

	printf("File in upload directory: %s\n", file_list);
	printf("Received: %zu bytes\n\n", k);
	
}

void get_file(int sockfd)
{

    	printf("\nEnter the filename to copy: ");
    	char filename[256];
    	if (fgets(filename, sizeof(filename), stdin) == NULL) {
        	perror("Error reading filename");
        	return;
    	}

    
    	size_t filename_len = strlen(filename);
    	if (filename_len > 0 && filename[filename_len - 1] == '\n') {
        	filename[--filename_len] = '\0';
    	}

    	writen(sockfd, (unsigned char *) &filename_len, sizeof(size_t));
    	writen(sockfd, (unsigned char *) filename, filename_len);

    	size_t filesize;
    	readn(sockfd, (unsigned char *) &filesize, sizeof(size_t));

    	if (filesize == 0) {
        	printf("File does not exist on the server.\n\n");
        	return;
    	}

    	FILE *file = fopen(filename, "wb");
    	if (file == NULL) {
        	perror("Error creating file");
        	return;
    	}

    	size_t remaining = filesize;
    	while (remaining > 0) {
		char buffer[1024];
		size_t to_read = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;

		ssize_t bytes_read = read(sockfd, buffer, to_read);
		if (bytes_read <= 0) {
            		perror("Error receiving file");
            		fclose(file);
            		return;
        	}	

		size_t bytes_written = fwrite(buffer, 1, bytes_read, file);
		if (bytes_written != bytes_read) {
            		perror("Error writing to file");
            		fclose(file);
            		return;
        	}

        	remaining -= bytes_read;
    	}

    	fclose(file);
    	printf("File '%s' copied successfully.\n\n", filename);
}


int main(void)
{
    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("Error - could not create socket");
	exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons(50031);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
	perror("Error - connect failed");
	exit(1);
    } else
       printf("Connected to server...\n");

	do{
		display_menu();
        	int choice = 0;
		char input[100];
        	printf("Enter your choice: ");
        	if (fgets(input, sizeof(input), stdin) == NULL) {
            		fprintf(stderr, "Error reading input\n");
            		return 1;
        	}

		 size_t input_len = strlen(input);
    		if (input_len > 0 && input[input_len - 1] == '\n') {
        		input[--input_len] = '\0';
    		}

		if (sscanf(input, "%d", &choice) == 1) {
       		} else {
           		printf("Invalid input. Please enter a valid integer.\n");
        	}
			

		writen(sockfd, &choice, sizeof(choice));
 		
		switch(choice){
			case 1:
				get_name_id(sockfd);
				break;
			case 2:
				get_rand_arr(sockfd);
				break;
			case 3: 
				get_sys_info(sockfd);
				break;
			case 4:
				get_file_list(sockfd);
				break;
			case 5:
				get_file(sockfd);
				break;
			case 6:
				printf("Exiting\n");
				break;
			default: 
				printf("Invalid input! Select 1 to 6\n\n");
				break;


		}
		
		if(choice == 6){break;}

	
	}while (6);

        close(sockfd);

    exit(EXIT_SUCCESS);
}
