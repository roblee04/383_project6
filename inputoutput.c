#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<unistd.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>
#include<time.h>
#include<signal.h>

void child_logic(int num, int pipe);

int main() {
    int children = 5;
    int child_num = 0;
    int isChild = 0;

    // set the file descriptor for select() sys call
    fd_set readfds;
    FD_ZERO(&readfds);

    // create pipes fd for children, 0 read, 1 write
    int c_fd[children][2]

    // make pipes
    for(int i = 0; i < children; i ++) {
        pipe(c_fd[i])
    }

    //create 5 children
    while(child_num < children) {
        int pid = fork();
    
        if(pid == 0) {
            child_num ++;
            isChild = 1;
            printf("I am child %d \n", child_num);
            break;
        } else {
            printf("my pid is %d \n", pid);
        }
        child_num += 1;
    }


    // logic for children
    if (isChild) {
        // make pipe to write
        int pipe = c_fd[child_num - 1][1];
        // do write logic
        child_logic(child_num, pipe);
        return;
    }
    else {
        // use select() system call to congregate pipes

        // wait for children to finish

        return;
    }

}

// num = child #, p = parent or not
void child_logic(int num, int pipe) {

    if(num == 5) {
        printf("I am child 5\n");
        // scan input and write to pipe
    }

    else {
        printf("I am child %d\n", num);
        // write to pipe

    }
}
