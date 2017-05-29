#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define INTERVAL 800000

#define MAX_BUF 2000

struct timespec tstart, tend;	//Used to count time taken by process
struct timespec tstartGlobal, tendGlobal, ttemp;
double cpuTime=0;
int numOfProcDone=0;
double cpuEffPlot[10];
double cpuTh[10];
int reachVal=0;		//Determine if cpuEffPlot is filled or not//Shift logic

int maxQueueLimit = 10;	//Constant number
int activeProcesses=0;	//Number of active processes at the moment
int newProcSub=0;		//Number of New submitted processes
int queue[10];		//Actual queue that stores PID
int queueDMptr[10];		//It stores the starting address of info in DM
double queueTime[10];		//Queue for storing time taken by process
double queueTurnArTime[10];		//Queue for storing turnaround time
double queueWaitTime[10];		//Queue for storing turnaround time

int storageArr[10];

char recProc2D[100][100];	//Stores the names of processes

char buf[MAX_BUF];
char globPname[150];

void strip(char *s) {
    char *p2 = s;
    while(*s != '\0') {
        if(*s != '\t' && *s != '\n') {
            *p2++ = *s++;
        } else {
            ++s;
        }
    }
    *p2 = '\0';
}

writeCPUeff(double eff, double throughput)
{
	if(reachVal<10)
	{
		cpuEffPlot[reachVal] = eff;
		cpuTh[reachVal] = throughput;
		FILE *fpP;
		fpP = fopen("../plotEff", "a");	//append file	

		fprintf(fpP, "%d ", reachVal);
	
		fprintf(fpP, "%0.5f ", eff);
		
		fprintf(fpP, "%0.5f\n", throughput);
		fclose(fpP);
		reachVal++;
	}
	else	//Shift logic
	{
		FILE *fpP;
		fpP = fopen("../plotEff", "w");	//overwrite file
		fprintf(fpP, "Time CPUeff Throughput\n");	
		int i;
		for(i=0;i<10;i++)
		{
			if(i!=9)
			{
				cpuEffPlot[i] = cpuEffPlot[i+1];
				cpuTh[i] = cpuTh[i+1];
			}
			else
			{
				cpuEffPlot[i] = eff;
				cpuTh[i] = throughput;
			}
				
			fprintf(fpP, "%d ", i);
			fprintf(fpP, "%0.5f ", cpuEffPlot[i]);
			fprintf(fpP, "%0.5f\n", cpuTh[i]);
		}
		fclose(fpP);
	}
}

void writeHistory(int prID, double timeP)	//Writing History
{
	FILE *fpH;
	fpH = fopen("../history", "a");	//append file	
	
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	fprintf(fpH, "%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	
	char pidS[40];
	snprintf(pidS, 10, "%d", prID);
	
	retPname(prID);		//now we have globPname
	
	strip(globPname);
	
	fprintf(fpH, globPname);
	fprintf(fpH, " ");
	fprintf(fpH, pidS);
	fprintf(fpH, " ExecTime=%0.5f ", timeP);
	fprintf(fpH, " TurnArnd=%0.5f ", queueTurnArTime[0]);
	fprintf(fpH, " WaitTime=%0.5f\n", queueWaitTime[0]);
	fclose(fpH);
}

void writeLog(int prID, int state, double th)	//state 0 imples stopped->running//1 implies running->stopped
{
	FILE *fpLog;
	fpLog = fopen("../log", "a");	//append file	
	
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	fprintf(fpLog, "%d-%d-%d %d:%d:%d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	
	char act[30];
	snprintf(act, 10, "%d", activeProcesses);
	
	char pidS[40];
	snprintf(pidS, 10, "%d", prID);
	fprintf(fpLog, "ActiveProc=");	
	fprintf(fpLog, act);
	fprintf(fpLog, " ");
	retPname(prID);		//now we have globPname
	
	strip(globPname);
	
	fprintf(fpLog, globPname);
	fprintf(fpLog, " PID=");
	fprintf(fpLog, pidS);
	if(state==0)
		fprintf(fpLog, " Stopped-->Running ");
	else if(state==1)
		fprintf(fpLog, " Running-->Stopped ");
	else	//2
		fprintf(fpLog, " Running-->Exited ");
		
	clock_gettime(CLOCK_MONOTONIC, &tendGlobal);	//Stopping timer and adding timeDiff
	double totTime = ((double)tendGlobal.tv_sec + 1.0e-9*tendGlobal.tv_nsec) - ((double)tstartGlobal.tv_sec + 1.0e-9*tstartGlobal.tv_nsec);
	double cpuEff = cpuTime*100/totTime;
	//writeCPUeff(cpuEff);		//Cretes another file
	char sCPUeff[20];
	snprintf(sCPUeff, 10, "%0.5f", cpuEff);
	//fprintf(fpLog, "   CPUtime=%f ", cpuTime);
	//fprintf(fpLog, "   totTime=%f ", totTime);
	fprintf(fpLog, "   CPU_Eff=%s", sCPUeff);
	fprintf(fpLog, "   Throughput=%0.5f\n", th);
	
	fclose(fpLog);
}

void updatePCB(int ptrStart, int state)
{
	FILE *fpdm; //file ptr for DM reading
	fpdm = fopen("../DataMemory", "r");
	if(fpdm == NULL)
	{
	  printf("Error opening DM!");   
	  exit(1);             
	}
	
	FILE *fp;
	char cmndF[100] = "../proc/";
	char qID[20];
	snprintf(qID, 10, "%d", queue[0]);
	strcat(cmndF, qID);
	fp = fopen(cmndF, "w");	//file opened for writing
	
	//Data Memory is already opened
	char readVal[40]="OK";	//Dummy
	
	fseek(fpdm, ptrStart, SEEK_SET);
	int checkState=0;
	int firstPtrStart = ptrStart;
	while(readVal[0]!='E' || readVal[1]!='N' || readVal[2]!='D')
	{
		ptrStart+=50;
		if(ptrStart-firstPtrStart>500)
			break;
		fscanf(fpdm, "%s", readVal);
		fseek(fpdm, ptrStart, SEEK_SET);
		if(readVal[0]!='E' || readVal[1]!='N' || readVal[2]!='D')
		{
			fprintf(fp,readVal);			
			fprintf(fp,"\n");
			if(checkState==0)
			{
				checkState=1;
				if(state==0)
					fprintf(fp,"State=Stopped\n");			
				else
					fprintf(fp,"State=Running\n");
				fprintf(fp,"MemoryPtr=%d\n", firstPtrStart);
			}
		}
	}
	fclose(fp);
	fclose(fpdm);
}

void recProcSched()
{
	int fd;
    char * myfifo = "/tmp/myfifo";

    /* open, read, and display the message from the FIFO */
    fd = open(myfifo, O_RDONLY);
    read(fd, buf, MAX_BUF);
    printf("Received: %s\n", buf);
    
    int i, j;
    int iter=0;
    
    for(i=0;i<100;i++)
    {
    	for(j=0;j<100;j++)
    	{
    		recProc2D[i][j] = '\0';		
    	}
    }
    
    recProc2D[newProcSub][iter] = buf[i];
    for(i=0;i<strlen(buf)-1;i++)
    {
    	if(buf[i]!=' ')
    	{
    		recProc2D[newProcSub][iter] = buf[i];
    		iter++;
    	}
    	else
    	{
    		newProcSub++;
    		iter = 0;
    	}
    }    
    close(fd);
}

void retPname(int procID)
{
	FILE *fp;
	int ID;
    char path[100];

    //Open the command for reading
    char cmnd[] = "../sh_script/getPname.sh ";
    char pidStr[20] = "OK";
    snprintf(pidStr, 10, "%d", procID);
    strcat(cmnd, pidStr);
    fp = popen(cmnd, "r"); 
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit;
    }
    //Read the output a line at a time - output it
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
		strcpy(globPname, path); 
    }
    pclose(fp);
}

int retPID(char procName[])		//Given a process name, return its PID
{
	FILE *fp;
	int ID;
    char path[1035];

    //Open the command for reading
    char cmnd[] = "../sh_script/getProcID.sh ";
    strcat(cmnd, procName);
    fp = popen(cmnd, "r"); 
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit;
    }
    //Read the output a line at a time - output it
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
		ID = atoi(path); 
    }
    pclose(fp);
	return ID;
}

int sendSignal(int id, int sig)
{
	FILE *fp;
	int status=0;
    char path[1035];

    //Open the command for reading
    printf("\nID=%d, sig=%d", id, sig);
	char cmnd[] = "../sh_script/sendSIG_new.sh ";
	char pid[10];
	char signal[5];
	char space[] = " ";
	snprintf(signal, 10, "%d", sig);
	snprintf(pid, 10, "%d", id);
	strcat(cmnd, signal);
	strcat(cmnd, space);
	strcat(cmnd, pid);
	
    fp = popen(cmnd, "r"); 
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit;
    }
    //Read the output a line at a time - output it
    while (fgets(path, sizeof(path)-1, fp) != NULL) {
		status = atoi(path); 
    }
    pclose(fp);
	return status;
}

void sigRec (int i)
{
	struct itimerval tout_val;
	signal(SIGALRM,sigRec);
	
	if(i==SIGALRM)
	{
		printf("\nDispatcher got control of CPU"); 
		
		recProcSched();			//Check if new processes are submitted
		
		if(buf[0]=='N' && buf[1]=='U' && buf[2]=='L' && buf[3]=='L')
		{
			dispatcher(1);		//Normal timeSlice end logic
		}
		else
		{
			dispatcher(3);		//New process have been submitted
		}
		clock_gettime(CLOCK_MONOTONIC, &tendGlobal);	//Stopping timer and adding timeDiff
		double totTime = ((double)tendGlobal.tv_sec + 1.0e-9*tendGlobal.tv_nsec) - ((double)tstartGlobal.tv_sec + 1.0e-9*tstartGlobal.tv_nsec);
		double cpuEff = cpuTime*100/totTime;
		double th = numOfProcDone*100/totTime;
		writeCPUeff(cpuEff, th);		//Cretes another file
	}
	else if(i==SIGQUIT)
	{
		printf("\nControl 2");
		dispatcher(2);	//Current running Process exited
	}
	tout_val.it_interval.tv_sec = 0;
	tout_val.it_interval.tv_usec = 0;
	tout_val.it_value.tv_usec = INTERVAL;
	tout_val.it_value.tv_sec = 0;
	setitimer(ITIMER_REAL, &tout_val,0);
}

void freeDM()
{
	FILE *fpdm; //file ptr for DM reading
	fpdm = fopen("../DataMemory", "r+");
	if(fpdm == NULL)
	{
	  printf("Error opening DM!");   
	  exit(1);             
	}
	int ptrStart = queueDMptr[0];
	fseek(fpdm, ptrStart, SEEK_SET);
	char writeVal[60] = " ";
	char space[2] = " ";
	int checkState=0;
	int firstPtrStart = ptrStart;
	int i;
	for(i=0;i<48;i++)
		strcat(writeVal, space);
		
	for(i=0;i<10;i++)
	{
		ptrStart+=50;
		fprintf(fpdm, writeVal);
		fseek(fpdm, ptrStart, SEEK_SET);
	}
	fclose(fpdm);
	storageArr[queueDMptr[0]/500] = 1;	//Free indicator is 1
}

int findStorage()
{
	int i, retVal=0;
	for(i=0;i<10;i++)
	{
		if(storageArr[i]==1)
		{
			retVal = 500*i;
			storageArr[i] = 0;
			break;
		}
	}
	return retVal;
}

void dispatcher (int task)
{
	if(task==1 && activeProcesses>0)
	{
		//Stop the current running process, send it to last and resume the next process in the queue
		clock_gettime(CLOCK_MONOTONIC, &tendGlobal);	//Stopping timer and adding timeDiff
		double totTime = ((double)tendGlobal.tv_sec + 1.0e-9*tendGlobal.tv_nsec) - ((double)tstartGlobal.tv_sec + 1.0e-9*tstartGlobal.tv_nsec);
		double th = numOfProcDone*100/totTime;
		writeLog(queue[0], 1, th);		//1 means running --> stopped
		int checkStat = sendSignal(queue[0], 19);
		if(checkStat==0)	//0 is expected//1 is trouble
		{
			clock_gettime(CLOCK_MONOTONIC, &tend);	//Stopping timer and adding timeDiff
			queueTime[0] += ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
			queueWaitTime[0] -= ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec);		//start the time for waiting
			cpuTime += ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
			//START: Update the PCB of that process
			updatePCB(queueDMptr[0], 0);	//second parameter 0 is for stop state
			//END: Update
		
			//Shift one position of queue
			int temp=queue[0];
			int temp2 = queueDMptr[0];
			double tempTime = queueTime[0];
			double tempWaitTime = queueWaitTime[0];
			double tempTrTime = queueTurnArTime[0];
			int i;
			for(i=0;i<activeProcesses-1;i++)
			{
				queue[i] = queue[i+1];
				queueDMptr[i] = queueDMptr[i+1];
				queueTime[i] = queueTime[i+1];
				queueWaitTime[i] = queueWaitTime[i+1];
				queueTurnArTime[i] = queueTurnArTime[i+1];
			}
			queue[activeProcesses-1] = temp;	//Current executing process went back of queue
			queueDMptr[activeProcesses-1] = temp2;
			queueTime[activeProcesses-1] = tempTime;
			queueWaitTime[activeProcesses-1] = tempWaitTime;
			queueTurnArTime[activeProcesses-1] = tempTrTime;
		
			//resume queue[0]
			writeLog(queue[0], 0, th);
			updatePCB(queueDMptr[0], 1);	//1 is for run state
			sendSignal(queue[0], 18);
			clock_gettime(CLOCK_MONOTONIC, &tstart);	//Starting timer
			queueWaitTime[0] += ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);	//Stopping the waiting time
		}
	}
	else if(task==2)				//Current running process has done execution//Process exited
	{
		numOfProcDone++;
		clock_gettime(CLOCK_MONOTONIC, &tend);	//Stopping timer and adding timeDiff
		queueTime[0] += ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
		cpuTime += ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
		queueTurnArTime[0] =  ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - queueTurnArTime[0];
		
		updatePCB(queueDMptr[0], 0);	//0 to stop
		
		clock_gettime(CLOCK_MONOTONIC, &tendGlobal);	//Stopping timer and adding timeDiff
		double totTime = ((double)tendGlobal.tv_sec + 1.0e-9*tendGlobal.tv_nsec) - ((double)tstartGlobal.tv_sec + 1.0e-9*tstartGlobal.tv_nsec);
		double th = numOfProcDone*100/totTime;
		
		writeLog(queue[0], 2, th);		//2- exited
		writeHistory(queue[0], queueTime[0]);	//Writing History
		freeDM();						//Free up the DataMemory of that process
		//Shift one position of queue
		int i;
		for(i=0;i<activeProcesses-1;i++)
		{
			queue[i] = queue[i+1];
			queueDMptr[i] = queueDMptr[i+1];
			queueTime[i] = queueTime[i+1];
			queueWaitTime[i] = queueWaitTime[i+1];			
			queueTurnArTime[i] = queueTurnArTime[i+1];
		}
		activeProcesses--;		//queueCap is decreased as one process is exited
		
		//resume queue[0]
		if(activeProcesses>0)
		{
			clock_gettime(CLOCK_MONOTONIC, &tendGlobal);	//Stopping timer and adding timeDiff
		double totTime = ((double)tendGlobal.tv_sec + 1.0e-9*tendGlobal.tv_nsec) - ((double)tstartGlobal.tv_sec + 1.0e-9*tstartGlobal.tv_nsec);
		double th = numOfProcDone*100/totTime;
		
			writeLog(queue[0], 0, th);
			updatePCB(queueDMptr[0], 1);
			printf("\nTask 2 Sending signal to %d", queue[0]);
			sendSignal(queue[0], 18);
			clock_gettime(CLOCK_MONOTONIC, &tstart);	//Starting timer
			queueWaitTime[0] += ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);	//Stopping the waiting time
		}
	}
	else if(task==3)
	{
		int i;
		if(activeProcesses>0)
		{
			clock_gettime(CLOCK_MONOTONIC, &tendGlobal);	//Stopping timer and adding timeDiff
		double totTime = ((double)tendGlobal.tv_sec + 1.0e-9*tendGlobal.tv_nsec) - ((double)tstartGlobal.tv_sec + 1.0e-9*tstartGlobal.tv_nsec);
		double th = numOfProcDone*100/totTime;	
			
			writeLog(queue[0], 1, th);		//1 means running --> stopped
			sendSignal(queue[0], 19);
		    clock_gettime(CLOCK_MONOTONIC, &tend);	//Stopping timer and adding time diff
		    queue[0] += ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
		    
			//START: Update the PCB of that process
			updatePCB(queueDMptr[0], 0);	//second parameter 0 is for stop state
			//END: Update
		}
		for(i=0;i<=newProcSub;i++)
		{
			char cmnd[100] = "../sh_script/execProc.sh ../process_c_out_ex/";
			strcat(cmnd, recProc2D[i]);
			char storage[30];
			char space[] = " ";
			strcat(cmnd, space); 
			//queueDMptr[activeProcesses+i] = (i+activeProcesses)*500;  
			//snprintf(storage, 10, "%d", (i+activeProcesses)*500);
			
			queueDMptr[activeProcesses+i] = findStorage();
			printf("Allocated queueDMptr=%d", queueDMptr[activeProcesses+i]);  
			snprintf(storage, 10, "%d", queueDMptr[activeProcesses+i]);
			
			strcat(cmnd, storage);
			system(cmnd);
			queue[activeProcesses+i] = retPID(recProc2D[i]);
			
			queueTime[activeProcesses+i] = 0;	//Initialized execution time to zero
			queueWaitTime[activeProcesses+i] = 0; //Initialized wait time to zero
			
			clock_gettime(CLOCK_MONOTONIC, &ttemp);	//Capturing the arrival time
			queueTurnArTime[activeProcesses+i] = ((double)ttemp.tv_sec + 1.0e-9*ttemp.tv_nsec);
			
			sendSignal(queue[activeProcesses+i], 19);	//Stopped the process
			queueWaitTime[activeProcesses+i] -= ((double)ttemp.tv_sec + 1.0e-9*ttemp.tv_nsec);	//Starting the waiting time
		}
		activeProcesses += newProcSub+1;
		newProcSub = 0;
		
		
		//Shift one position of queue
		int temp=queue[0];
		int temp2 = queueDMptr[0];
		double tempTime = queueTime[0];
		double tempWaitTime = queueWaitTime[0];
		double tempTrTime = queueTurnArTime[0];
		for(i=0;i<activeProcesses-1;i++)
		{
			queue[i] = queue[i+1];
			queueDMptr[i] = queueDMptr[i+1];
			queueTime[i] = queueTime[i+1];
			queueWaitTime[i] = queueWaitTime[i+1];
			queueTurnArTime[i] = queueTurnArTime[i+1];
		}
		queue[activeProcesses-1] = temp;	//Current executing process went back of queue
		queueDMptr[activeProcesses-1] = temp2;
		queueTime[activeProcesses-1] = tempTime;
		queueWaitTime[activeProcesses-1] = tempWaitTime;
		queueTurnArTime[activeProcesses-1] = tempTrTime;		
		
		
		//resume queue[0]
		clock_gettime(CLOCK_MONOTONIC, &tendGlobal);	//Stopping timer and adding timeDiff
		double totTime = ((double)tendGlobal.tv_sec + 1.0e-9*tendGlobal.tv_nsec) - ((double)tstartGlobal.tv_sec + 1.0e-9*tstartGlobal.tv_nsec);
		double th = numOfProcDone*100/totTime;
		
		writeLog(queue[0], 0, th);
		updatePCB(queueDMptr[0], 1);	//1 is for run state
		sendSignal(queue[0], 18);
		clock_gettime(CLOCK_MONOTONIC, &tstart);	//Starting timer
		queueWaitTime[0] +=  ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);	//Stopping wait time
	}
}
 
void exit_func (int i)
{
    signal(SIGINT,exit_func);
    printf("\nYou cannot kill me!!!\n");
}
 
int main ()
{
	clock_gettime(CLOCK_MONOTONIC, &tstartGlobal);	//Starting Global start timer
	//Create a DataMemory
	FILE *dm;
	dm = fopen("../DataMemory", "w");
	if(dm == NULL)
	{
	  printf("Error creating DM!");   
	  exit(1);             
	}
	
	char space1[100] = " ";
	char space2[2] = " ";
	int i;
	for(i=0;i<10;i++)
		storageArr[i] = 1;	//1 means available//0 means empty
		
	for(i=0;i<48;i++)
		strcat(space1, space2);
	for(i=0;i<300;i++)
	{
		fprintf(dm, space1);
		fprintf(dm, "\n");
	}
	fclose(dm);
	
	//Create GNUPLOT FILE
	FILE *fGNU;
	fGNU = fopen("../plotEff", "w");
	fprintf(fGNU, "Time CPUeff Throughput\n");	
	fclose(fGNU);
	
	//start eff monitor
	system("gnuplot live.gnu &");
	
	struct itimerval tout_val;
  	tout_val.it_interval.tv_sec = 0;
  	tout_val.it_interval.tv_usec = 0;
  	tout_val.it_value.tv_sec = 0; 
  	tout_val.it_value.tv_usec = INTERVAL; 
  	setitimer(ITIMER_REAL, &tout_val,0);
 	
  	signal(SIGALRM, sigRec);
  	signal(SIGQUIT, sigRec);
  	signal(SIGINT, exit_func);
  
   
  	while (1)
  	{}
  	return 0;
}

