#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
 
int main ()
{
	char cmnd[100] = "sh_script/start_it.sh";
	system(cmnd);
  	return 0;
}

