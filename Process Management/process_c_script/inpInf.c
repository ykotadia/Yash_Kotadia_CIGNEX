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

void writeDataMem(int num1, int num2, int sum, int diff, int product, int div)
{
	rewind(fptr);
	char ID[20];
	char IDStr[40] = "ID=";
	char readVal[40] = "";
	char sNum2[40];
	char sNum1[40];
	char sSum[40];
	char sDiff[40];
	char sProduct[40];
	char sDiv[40];
	
	
	sprintf(ID, "%d", myID);
	sprintf(sNum2, "%d", num2);
	sprintf(sNum1, "%d", num1);
	sprintf(sSum, "%d", sum);
	sprintf(sDiff, "%d", diff);
	sprintf(sProduct, "%d", product);
	sprintf(sDiv, "%d", div);
	
	strcat(IDStr, ID);	
	
	fseek(fptr, fileMemStart, SEEK_SET);
	fprintf(fptr,"ID=%s",ID);
	fseek(fptr, fileMemStart+50, SEEK_SET);		
	fprintf(fptr,"num2=%s     ",sNum2);
	fseek(fptr, fileMemStart+100, SEEK_SET);
	fprintf(fptr,"num1=%s     ",sNum1);
	fseek(fptr, fileMemStart+150, SEEK_SET);
	fprintf(fptr,"sum=%s     ",sSum);
	fseek(fptr, fileMemStart+200, SEEK_SET);
	fprintf(fptr,"diff=%s     ",sDiff);
	fseek(fptr, fileMemStart+250, SEEK_SET);
	fprintf(fptr,"product=%s     ",sProduct);
	fseek(fptr, fileMemStart+300, SEEK_SET);
	fprintf(fptr,"div=%s     ",sDiv);
	fseek(fptr, fileMemStart+350, SEEK_SET);					
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
	
	myID = retPID("inpInf.out");
	
	fptr = fopen("../DataMemory","r+");
	if(fptr == NULL)
	{
	  printf("Error!");   
	  exit(1);             
	}
	
	int num1=0;
	int num2=0;
	int sum=0;
	int diff=0;
	int product=0;
	int div=0;
	writeDataMem(num1, num2, sum, diff, product, div);
	
	printf("Enter num1:");
	scanf("%d", &num1);
	writeDataMem(num1, num2, sum, diff, product, div);
	printf("Enter num2:");
	scanf("%d", &num2);
	writeDataMem(num1, num2, sum, diff, product, div);
	
	sum = num1 + num2;
	writeDataMem(num1, num2, sum, diff, product, div);
	diff = num1 - num2;
	writeDataMem(num1, num2, sum, diff, product, div);
	product = num1*num2;
	writeDataMem(num1, num2, sum, diff, product, div);
	div = num1/num2;	
	writeDataMem(num1, num2, sum, diff, product, div);
	
	//Before return 0, send a signal to dispatcher
	fclose(fptr);
	int sight=1;
	while(sight==1)
	{}
	
	sleep(2);	//Executing wait//File write delay
	sendSignal(retPID("dispatcher.out"), 3);	//Sending SIGQUIT
	return 0;
}
