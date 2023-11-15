#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>
#include<time.h>
#include<signal.h>

//Considering two type of child A and B. Child A generates message at random interval of 0,1,or 2 second
//where as Child B prompts on stdout and take inputs from stdin

#define child_count 5
#define child_duration 30
#define READ_FD 0
#define WRITE_FD 1

//Global Start Time
struct timeval start;

double startTime;

void forkChild(int childNo, int pipe);

int main(int argc, char **argv) {

	gettimeofday(&start,NULL);
	startTime = (double)start.tv_sec + (double)start.tv_usec/1000000;

	int childNo = 1;
	int pipefd[child_count][2];	//Pipe has 2 file descripter, 0 - read & 1 - write
	
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
	
	// Parent process creates 5 child
	while(childNo <= child_count) {

		//Forking child
		int pid = fork();
		if(pid == 0) {
			isChild = 1;
			break;
		}
		if(childNo == child_count) {
			pid_5 = pid;
		}

		childNo++;
	}
	if(isChild) {
		//Closed unused pipe FileDescriptor
		for(int i=0;i<child_count;i++) {
			if(i != childNo - 1) {
				close(pipefd[i][WRITE_FD]);
			}
			close(pipefd[i][READ_FD]);
		}
		forkChild(childNo,pipefd[childNo-1][WRITE_FD]);	//Passing write fd of pipe
		return 0;
	} else {
		//Close unused pipe write fds
		for(int i=0;i<child_count;i++) {
			close(pipefd[i][WRITE_FD]);
		}

		// initialize tv, to keep track of time, also for select
		struct timeval tv;
		int isPipeActive[child_count];
		for(int i=0;i<child_count;i++) {
			isPipeActive[i] = 1;
		}

		// writing to file
		struct timeval cur;
		FILE *fp = fopen("output.txt","w");
		while(1) {
			
			gettimeofday(&cur,NULL);
			double curTime = (double)cur.tv_sec + (double)cur.tv_usec/1000000;
			
			tv.tv_sec = 0;
			tv.tv_usec = 1; //Wait for 1000 millisecond

			//Reinitializing read file descriptor set
			FD_ZERO(&rdfs);
			int totalPipeActive = 0;
			
			for(int i=0;i<child_count;i++) {
				if(isPipeActive[i]){
					FD_SET(pipefd[i][READ_FD],&rdfs);
					totalPipeActive++;
				}
			}

			if(totalPipeActive == 0) {
				break;
			}
			int status = select(maxFD, &rdfs, NULL, NULL, &tv);
			if(status == -1) {
				printf("Select Failure\n");
				break;
			} else if(status) {
				char msg[255];
				
				for(int i = 0;i < child_count;i++) {
					if(FD_ISSET(pipefd[i][READ_FD],&rdfs)) {
						int c = read(pipefd[i][READ_FD], msg, sizeof(char)*255);
						if(c) {
							fprintf(fp,"%s", msg);
							fflush(fp);
						} else{
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

	// Lifetime of each child 30 seconds
	// Each 4 childs sleep at random time of 0,1 or 2 seconds
	// Time stamped message to nearest 1000th of second gettimeofday(&tv, NULL), sends it to the parent process
	return 0;
}

void forkChild(int child_no, int p) {
	//printf("Child %d :: Forked Child\n",child_no);
	int random_wait_time = 0;	//0,1, or 2 sec
	int n;
	struct timeval cur;
	gettimeofday(&cur,NULL);

	// Intializes random number generator
	time_t t;
    srand((unsigned) time(&t));
	
	double curTime = (double)cur.tv_sec + (double)cur.tv_usec/1000000;
	int msgNo = 1;
	if(child_no == child_count) {
		// for child number 5, do input output

		fd_set rdfs;
		FD_ZERO(&rdfs);
		struct timeval tv;
		printf("Input to file :  ");
		while((curTime - startTime) < child_duration) {
			char msg[255];
			
			fflush(stdout);
			FD_ZERO(&rdfs);
			FD_SET(0,&rdfs);
			tv.tv_sec = 0;
			tv.tv_usec = 1; // Set microseconds to 1 (very short timeout)
			// select is used to check if data can be read in stdin
			if(select(1,&rdfs,NULL,NULL,&tv)>0) {
				n = read(0,msg,255);
				msg[n - 1] = '\0';	//remove the newline and set the ending character
				char bmsg[255];
				sprintf(bmsg,"0:%.3f: Child %d %s\n",(curTime-startTime),child_no,msg);
				write(p,bmsg,sizeof(char)*strlen(bmsg)+1);
				printf("Input to file : ");
			}
			// update time 
			gettimeofday(&cur,NULL);
			curTime = (double)cur.tv_sec + (double)cur.tv_usec/1000000;
		}
		printf("\n");
	} else {
		while((curTime - startTime) < child_duration) {
			
			// init char array to hold message
			char msg[255];
			// time stamp, child num, message number local to child proc
			// format and send to pipe
			sprintf(msg,"0:%.3f: Child %d message %d\n",(curTime-startTime),child_no,msgNo);
			write(p,msg,sizeof(char)*strlen(msg)+1);
			msgNo++;
			
			// after writing to pipe, wait for a random time to prevent collision
			random_wait_time = (rand()%4);
			sleep(random_wait_time);

			// update time
			gettimeofday(&cur,NULL);
			curTime = (double)cur.tv_sec + (double)cur.tv_usec/1000000;
		}
	}
	close(p);
}

