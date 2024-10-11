// Cwk2: client.c - message length headers with variable sized payloads
//  also use of readn() and writen() implemented in separate code module

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

//function to show menu
void display_menu(){
	printf("1. Get and display name and student ID\n");
    	printf("2. Get and display random numbers\n");
    	printf("3. Get and display server uname information\n");
    	printf("4. Get and display file and directory names in the 'upload' directory\n");
    	printf("5. Copy file from 'upload' directory\n");
	printf("6. Exit\n");
}

//function to receive and output name id with IP address prefix
void get_name_id(int socket)
{
    char to_recv[50];
    size_t k;

    readn(socket, (unsigned char *) &k, sizeof(size_t));	
    readn(socket, (unsigned char *) to_recv, k);

    printf("Name and ID with IP prefix: \n%s\n", to_recv);
    printf("Received: %zu bytes\n\n", k);
} 

//function to receive output random mumbers array and calculate sum
void get_rand_arr(int socket){
	int arr[5];

	size_t k;
	
	//recieve array via socket
    	readn(socket, (unsigned char *) &k, sizeof(size_t));	
    	readn(socket, (unsigned char *) arr, k);
    	
	//print and add array elements
	int i;
	int sum = 0;
	printf("Element of array: [ ");
	for(i=0; i < 5; i++){
		printf("%d ", arr[i]);
		sum += arr[i];
	}
	printf("]\n");

	//printing sum
	printf("Sum of array elements: %d\n\n", sum);

}
//function to get uname/server system information
void get_sys_info(int socket){
	//declaring struct to store system information
	struct utsname systemInfo;	
	
	//receiving uname via socket
	size_t payload_length;
	readn(socket, (unsigned char *) &payload_length, sizeof(size_t));	   
    	readn(socket, (unsigned char *) &systemInfo, payload_length);

	//printing relevent fields of the uname struct
	printf("\nOperating System: %s\n", systemInfo.sysname);
	printf("Release: %s\n", systemInfo.release);
	printf("Version: %s\n\n", systemInfo.version);
}

//function to receive the list of file in server's upload directory
void get_file_list(int socket){
	int result;
	readn(socket, &result, sizeof(result));//receiving value to indicate if directory exist

	//graceful error handling if directory does not exist
	if(!result){
		printf("Directory does not exist.\n\n");
		return;
	}

	//receiving list of files via socket
	char file_list[4096];
    	size_t k;

	readn(socket, (unsigned char *) &k, sizeof(size_t));	
	readn(socket, (unsigned char *) file_list, k);

	//print list of files
	printf("File in upload directory: %s\n", file_list);
	printf("Received: %zu bytes\n\n", k);
	
}

void get_file(int sockfd)
{
    //getting user input
    	printf("\nEnter the filename to copy: "); //prompt 
    	char filename[256];
    	if (fgets(filename, sizeof(filename), stdin) == NULL) {
        	perror("Error reading filename");
        	return;
    	}

    	//removing new line character from input
    	size_t filename_len = strlen(filename);
    	if (filename_len > 0 && filename[filename_len - 1] == '\n') {
        	filename[--filename_len] = '\0';
    	}

	//sending file name via socket
     	writen(sockfd, (unsigned char *) &filename_len, sizeof(size_t));
   	writen(sockfd, (unsigned char *) filename, filename_len);
	
	//receiving file size
    	size_t filesize;
    	readn(sockfd, (unsigned char *) &filesize, sizeof(size_t));

	//gracefully error handling if file sze == 0, i.e, file does not exist 
    	if (filesize == 0) {
        	printf("File does not exist on the server.\n\n");
        	return;
    	}
	
	//open file for writing
    	FILE *file = fopen(filename, "wb");
    	if (file == NULL) {
        	perror("Error creating file");
        	return;
    	}

	//receiving file content via socket and writing on file
    	size_t remaining = filesize;
    	while (remaining > 0) {
        	char buffer[1024];
        	size_t to_read = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
		
		//receiving file content
        	ssize_t bytes_read = read(sockfd, buffer, to_read);
        	if (bytes_read <= 0) {
            		perror("Error receiving file");//Error handling for receiving errors
            		fclose(file);
            		return;
        	}

		//writing  file content to file
        	size_t bytes_written = fwrite(buffer, 1, bytes_read, file);
        	if (bytes_written != bytes_read) {
            		perror("Error writing to file"); //Error handing for writing to file errors
            		fclose(file);
            		return;
        	}

        	remaining -= bytes_read; //decrementing remaining 
    	}
	//closing the file
    	fclose(file);
    	printf("File '%s' copied successfully.\n\n", filename);
}


int main(void)
{
    // *** this code down to the next "// ***" does not need to be changed except the port number
    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("Error - could not create socket");
	exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;

    // IP address and port of server we want to connect to
    serv_addr.sin_port = htons(50031);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // try to connect...
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
	perror("Error - connect failed");
	exit(1);
    } else
       printf("Connected to server...\n");

    // ***
    // your own application code will go here and replace what is below... 
    // i.e. your menu etc.

	do{
		//show menu
		display_menu();
		
		//get user input for menu option choosen
        	int choice = 0;
		char input[100];
        	printf("Enter your choice: ");
        	if (fgets(input, sizeof(input), stdin) == NULL) {
            		fprintf(stderr, "Error reading input\n");
            		return 1;
        	}

		//remove newline character
		 size_t input_len = strlen(input);
    		if (input_len > 0 && input[input_len - 1] == '\n') {
        		input[--input_len] = '\0';
    		}
		
		//attempt to convert string user input to integer
		if (sscanf(input, "%d", &choice) == 1) {
       		} else {
           		printf("Invalid input. Please enter a valid integer.\n");//Error handling if input was not a integer
        	}
		
		//send choice to server via socket
		writen(sockfd, &choice, sizeof(choice));
 		
		//switch statement to call the function that corresponds to the menu option entered by user
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
			default: //error handling for integer outside the 1 to 6 range
				printf("Invalid input! Select 1 to 6\n\n");
				break;


		}
		//exit loop when option 6 selected
		if(choice == 6){break;}

	
	}while (6);
	//closing socket
        close(sockfd);

    	exit(EXIT_SUCCESS);
} // end main()
