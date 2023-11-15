#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>
#include<time.h>
#include<signal.h>

#define child_count 5
#define child_duration 30
#define READ_FD 0
#define WRITE_FD 1

// Global Start Time
struct timeval start;

double startTime;

// Function to fork child processes
void forkChild(int childNo, int pipe);

int main(int argc, char **argv) {
	// Initialize global start time
	gettimeofday(&start,NULL);
	startTime = (double)start.tv_sec + (double)start.tv_usec/1000000;

	int childNo = 1;
	// Array to store file descriptors for pipes
	int pipefd[child_count][2];	
	
	fd_set rdfs;
	FD_ZERO(&rdfs);
	int maxFD = 0;
	int isChild = 0;
	int pid_5 = -1;

	//Create pipes
	for(int i=0;i < child_count;i++) {
		pipe(pipefd[i]);
	}
	maxFD = pipefd[child_count-1][READ_FD] + 1;
	
	// Parent process creates 5 child processes
	while(childNo <= child_count) {
		
		int pid = fork();
		if(pid == 0) {
			// Child process
			isChild = 1;
			break;
		}
		if(childNo == child_count) {
			// Record the process ID of the fifth child
			pid_5 = pid;
		}

		childNo++;
	}
	if(isChild) {
		// Code for child processes

        // Close unused pipe file descriptors		
		for(int i=0;i<child_count;i++) {
			if(i != childNo - 1) {
				close(pipefd[i][WRITE_FD]);
			}
			close(pipefd[i][READ_FD]);
		}

		// Call the forkChild function to handle specific behavior for each child
		forkChild(childNo,pipefd[childNo-1][WRITE_FD]);	
		return 0;
	} else {		
		// Code for the parent process

        // Close unused pipe write file descriptors		
		for(int i=0;i<child_count;i++) {
			close(pipefd[i][WRITE_FD]);
		}
		
		struct timeval tv;
		int isPipeActive[child_count];
		for(int i=0;i<child_count;i++) {
			// Initialize array to track active pipes
			isPipeActive[i] = 1;
		}
		struct timeval cur;

		// Declaring Ouput file
		FILE *fp = fopen("output.txt","w");
		
		// Main loop to read from pipes and write to the output file
		while(1) {
			
			gettimeofday(&cur,NULL);
			double curTime = (double)cur.tv_sec + (double)cur.tv_usec/1000000;
			
			tv.tv_sec = 0;
			tv.tv_usec = 1; // Wait for 1000 milliseconds

			// Reinitialize read file descriptor set
			FD_ZERO(&rdfs);
			int totalPipeActive = 0;
			for(int i=0;i<child_count;i++) {
				if(isPipeActive[i]){
					FD_SET(pipefd[i][READ_FD],&rdfs);
					totalPipeActive++;
				}
			}

			if(totalPipeActive == 0) {
				// If no active pipes, exit the loop
				break;
			}
			int status = select(maxFD, &rdfs, NULL, NULL, &tv);
			if(status == -1) {
				// Error handling for select failure
				printf("Select Failure\n");
				break;
			} else if(status) {
				char msg[255];
				
				// Iterate through pipes to check for available data
				for(int i = 0;i < child_count;i++) {
					if(FD_ISSET(pipefd[i][READ_FD],&rdfs)) {
						int c = read(pipefd[i][READ_FD], msg, sizeof(char)*255);
						if(c) {
							// Write messages to the output file
							fprintf(fp,"%s", msg);
							fflush(fp);
						} else{
							// If child has closed the pipe, mark it as inactive
							printf("Child %d has closed the pipe\n",i+1);
							close(pipefd[i][READ_FD]);
							isPipeActive[i] = 0;
						}
					}
				}
			}
		}
		fclose(fp);
		printf("Terminating main process\n");
	}

	
	return 0;
}


// Function to handle behavior of child processes
void forkChild(int child_no, int p) {
	
	int random_wait_time = 0;	//0,1, or 2 sec
	int n;
	struct timeval cur;
	gettimeofday(&cur,NULL);

	
	time_t t;
    srand((unsigned) time(&t));
	
	double curTime = (double)cur.tv_sec + (double)cur.tv_usec/1000000;
	int msgNo = 1;

	// Behavior for the fifth child
	if(child_no == child_count) {
		fd_set rdfs;
		FD_ZERO(&rdfs);
		struct timeval tv;
		printf("Child 5 >> ");
		while((curTime - startTime) < child_duration) {
			char msg[255];
			fflush(stdout);
			FD_ZERO(&rdfs);
			FD_SET(0,&rdfs);
			tv.tv_sec = 0;
			tv.tv_usec = 1;
			if(select(1,&rdfs,NULL,NULL,&tv)>0) {
				n = read(0,msg,255);
				msg[n - 1] = '\0';	
				char bmsg[255];
				sprintf(bmsg,"0:%.3f: Child %d %s\n",(curTime-startTime),child_no,msg);
				write(p,bmsg,sizeof(char)*strlen(bmsg)+1);
				printf("Child 5 >> ");
			}
			gettimeofday(&cur,NULL);
			curTime = (double)cur.tv_sec + (double)cur.tv_usec/1000000;
		}
		printf("\n");
	} else {

		// Behavior for the first four children
		while((curTime - startTime) < child_duration) {
			// Generate random wait time
			random_wait_time = (rand()%4);

			char msg[255];
			sprintf(msg,"0:%.3f: Child %d message %d\n",(curTime-startTime),child_no,msgNo);
			write(p,msg,sizeof(char)*strlen(msg)+1);
			msgNo++;
			// Sleep for random wait time
			sleep(random_wait_time);

			// Update current time
			gettimeofday(&cur,NULL);
			curTime = (double)cur.tv_sec + (double)cur.tv_usec/1000000;
		}
	}

	// Close the write end of the pipe
	close(p);
}