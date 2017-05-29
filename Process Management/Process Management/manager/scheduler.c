#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

char procList[] = "";


void findProc()
{
	memset(&procList[0], 0, sizeof(procList));
	FILE *fp;
    char path[1035];

    /* Open the command for reading. */
    char cmnd[] = "../sh_script/findProc.sh";
    fp = popen(cmnd, "r"); 
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit;
    }
    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
    	strcat(procList, path);
    }
    pclose(fp);
}

void moveProc()
{
	char cmnd[] = "../sh_script/mvProc.sh";
    system(cmnd);
}

void exit_func (int i)
{
    signal(SIGINT,exit_func);
    printf("\nYou cannot kill me!!!\n");
}

int main()
{
	int fd;
	char * myfifo = "/tmp/myfifo";
	
	signal(SIGINT, exit_func);
	
	while(1)
	{
		findProc();		//Check if any new process is submitted
		moveProc();		//Move all the processes to exec directory
		if(procList!=NULL)
			printf("Sending %s", procList);
		else
			printf("Sending NULL");
		/* create the FIFO (named pipe) */
		mkfifo(myfifo, 0666);

		/* write string to the FIFO */
		fd = open(myfifo, O_WRONLY);
		char sendInfo[1024];
		strcpy(sendInfo, procList);
		write(fd, sendInfo, sizeof(sendInfo));
		close(fd);

		/* remove the FIFO */
		unlink(myfifo);
	}

    return 0;
}
