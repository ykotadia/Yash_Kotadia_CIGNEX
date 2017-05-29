#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

FILE *fptr;
int myID;
int fileMemStart;

int retPID(char procName[])
{
	FILE *fp;
	int ID;
    char path[1035];

    /* Open the command for reading. */
    char cmnd[] = "../sh_script/getProcID.sh ";
    strcat(cmnd, procName);
    fp = popen(cmnd, "r"); 
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit;
    }
    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
		ID = atoi(path); 
    }
    pclose(fp);
	return ID;
}

void sendSignal(int id, int sig)
{
	char cmnd[] = "../sh_script/sendSIG.sh ";
	char pid[10];
	char signal[5];
	char space[] = " ";
	snprintf(signal, 10, "%d", sig);
	snprintf(pid, 10, "%d", id);
	strcat(cmnd, signal);
	strcat(cmnd, space);
	strcat(cmnd, pid);
	system(cmnd);
}

void writeDataMem(int num1, int num2)
{
	rewind(fptr);
	char ID[20];
	char IDStr[40] = "ID=";
	char readVal[40] = "";
	char sNum2[40];
	char sNum1[40];
	
	sprintf(ID, "%d", myID);
	sprintf(sNum2, "%d", num2);
	sprintf(sNum1, "%d", num1);
	strcat(IDStr, ID);	
	
	fseek(fptr, fileMemStart, SEEK_SET);
	fprintf(fptr,"ID=%s",ID);
	fseek(fptr, fileMemStart+50, SEEK_SET);		
	fprintf(fptr,"num2=%s     ",sNum2);
	fseek(fptr, fileMemStart+100, SEEK_SET);
	fprintf(fptr,"num1=%s     ",sNum1);
	fseek(fptr, fileMemStart+150, SEEK_SET);				
	fprintf(fptr,"END");			
}

void exit_func (int i)
{
    signal(SIGINT,exit_func);
    printf("\nI am dead!!!\n");
    sendSignal(retPID("dispatcher.out"), 3);	//Sending SIGQUIT
    exit(0);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, exit_func);	
	
	fileMemStart = atoi(argv[1]);
	
	//myID = retPID("calc.out");
	myID = getpid();
	
	fptr = fopen("../DataMemory","r+");
	if(fptr == NULL)
	{
	  printf("Error!");   
	  exit(1);             
	}
	
	int num1=0;
	int num2=0;
	while(num2<5000)
	{
		printf("\nnum1=%d num2=%d", num1, num2);
		num1++;
		writeDataMem(num1, num2);
		if(num1>100)
		{
			num1=0;
			num2++;
			writeDataMem(num1, num2);
		}
	}
	printf("END of process! num1=%d, num2=%d", num1, num2);
	//Before return 0, send a signal to dispatcher
	fclose(fptr);
	
	sleep(2);	//Executing wait//File write delay
	sendSignal(retPID("dispatcher.out"), 3);	//Sending SIGQUIT
	return 0;
}
