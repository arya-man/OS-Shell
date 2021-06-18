#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pwd.h>
#include<limits.h>
#include <unistd.h>
#include<time.h>
#include<dirent.h>
#include <fcntl.h>
#include<signal.h>
//#define _POSIX_SOURCE
#include<sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define one 1
#define zero 0
#define two 2
#define ll int
#define quant 200

#define BUFFERSIZE_TOKEN 64
#define DILIMITOR_WORD " \t\n\r"
#define DILIMITOR_COMMAND ";\n"
#define DILIMITOR_PIPE "|\n"

#define clear() printf("\033[H\033[J")

struct command
{
	char **argv;
	ll len;
};


char deviceName[100],currentWD[PATH_MAX];
char* userName;
char args[PATH_MAX];

ll childpid;
char * childProcName;

time_t timSt;
double waitFor;
char msg[500];
ll timeVLD = zero;
ll loopflg=one;
ll foreground=zero;
ll chldrun=zero;
ll shell_pid,Childpid=zero;

char * currentProcessName;

typedef struct backGroundJobs
{
	ll bcjt;
  char jobName[100];
  pid_t pid;
  char* status;
  struct backGroundJobs * next;
}backGroundJobs;
backGroundJobs* ThisJobs=NULL;

void HandlingPiping(char str[PATH_MAX]);

void executeInbuild(char* list[quant],ll len);

char *replaceWord(const char *s, const char *oldW,const char *newW)
{
    char *result;
    ll i, cnt = zero;
    ll newWlen = strlen(newW);
    ll oldWlen = strlen(oldW);
    for (i = zero; s[i] != '\0'; i++)
    {
        if (strstr(&s[i], oldW) == &s[i])
        {
            cnt=cnt+1;
            i =i+ oldWlen - one;
        }
    }
    result = (char *)malloc(i + cnt * (newWlen - oldWlen) + one);
    i = zero;
    while (*s)
    {
        if (strstr(s, oldW) == s)
        {
            strcpy(&result[i], newW);
            s = s+ oldWlen;
            i = i+ newWlen;
        }
        else
        {
            result[i] = *s;
            i++;
            *s++;
        }
    }
    result[i+zero] = '\0';
    return result;
}


ll add_job(char *stat,pid_t pid, char * name)
{
	ll ssup_bruh=one;
	backGroundJobs *new=(backGroundJobs*)malloc(sizeof(backGroundJobs));
	new->status = stat;
	new->pid=pid;
	ssup_bruh=ssup_bruh+one;
	strcpy(new->jobName,name);
	ssup_bruh=one;
	new->next=NULL;
	if (ThisJobs!=NULL)
	{
        backGroundJobs *temp=ThisJobs;

		for (;temp->next!=NULL;temp=temp->next);
		temp->next=new;
	}
	else
	{
        ThisJobs=new;
	}
}

ll delete_job(pid_t pid)
{
	ll osass3;
	foreground=zero;
	if (ThisJobs==NULL) {
		return one;
	}
	if (ThisJobs->pid==pid) {
		backGroundJobs *del=ThisJobs;
		ThisJobs=ThisJobs->next;
		free(del);
		return zero;
	}
	backGroundJobs *temp=ThisJobs,*prev;
	osass3++;
	for (prev=ThisJobs;temp->pid!=pid || temp==NULL;temp=temp->next)
	{
		if(temp==ThisJobs)
		{
			osass3=zero;
		}
		else if (temp!=ThisJobs)
		{
			prev=prev->next;
		}
	}

	//backGroundJobs *del=temp->next;
	if(temp!=NULL)
	{
		prev->next=temp->next;
		free(temp);
	}
}

ll search_job(pid_t pid)
{

	if(ThisJobs ==NULL)
	{
		return zero;
	}
	//printf("Searching for %d\n",pid);
	if (ThisJobs==NULL) {
		return zero;
	}
	if (ThisJobs->pid==pid) {
		//printf(" found");
		return one;
	}
	backGroundJobs *temp=ThisJobs;
	for (;temp->next!=NULL;temp=temp->next);
	{
		if(temp->pid==pid)
		{
			printf(" found");
			return one;
		}
	}
	return zero;
}


ll change_to_running(pid_t pid)
{

	if(ThisJobs ==NULL)
	{
		return zero;
	}
	if (ThisJobs->pid==pid) {
		ThisJobs->status = "Stopped";
		return one;
	}
	backGroundJobs *temp=ThisJobs;
	for (;temp->next!=NULL;temp=temp->next);
	{
		if(temp->pid==pid)
		{
			ThisJobs->status = "Stopped";
			return one;
		}
	}
	return zero;
}

ll is_job_running(pid_t pid)
{

	if(ThisJobs ==NULL)
	{
		return zero;
	}
	if (strcmp(ThisJobs->status,"Running")==zero) {
		//printf(" found");
		return one;
	}
	backGroundJobs *temp=ThisJobs;
	for (;temp->next!=NULL;temp=temp->next);
	{
		if(strcmp(ThisJobs->status,"Running")==zero)
		{
			return one;
		}
	}
	return zero;
}

ll get_job_pid(ll number)
{
	backGroundJobs *temp=ThisJobs;
	
	if(ThisJobs ==NULL)
	{
		return zero;
	}

	if(number==one && temp)
		return temp->pid;
		
	for(ll i=zero;i<number;i++)
	{
		if(temp==NULL)
			return -1;
		else
			temp=temp->next;
	}
	
	return temp->pid;
}

char* get_job_name(ll number)
{
	backGroundJobs *temp=ThisJobs;
	
	if(number==one && temp)
		return temp->jobName;
		
	for(ll i=zero;i<number;i++)
	{
		if(temp!=NULL)
            temp=temp->next;
		else
            return "";
			
	}
	
	return temp->jobName;
}

void update_job_status()
{
	
	backGroundJobs *temp=ThisJobs;
	if(ThisJobs==NULL)
	{
		return;
	}
	else{
		ll i=zero;
		for(i=zero;temp!=NULL;i++)
		{
			ll pid = temp->pid;
			char path[quant];
			ll proc;
			char stat;
			sprintf(path,"/proc/%d/stat",pid);
			FILE * filepointer = fopen(path,"r");
			if(filepointer==NULL)
			{
				//printf("No status File, no process with pid %d.\n",pid);
				return ;
			}
            fscanf(filepointer, "%d %*s %c",&proc,&stat);
			sprintf(path,"/proc/%d/stat",pid);
			char* process_status;
            if(stat=='S')
			{
				process_status = "Sleeping";
			}
            else if(stat=='Z')
			{
				process_status="Zombie";
			}
			
			else if(stat=='R')
			{
				process_status = "Running";
			}
			
			temp->status = process_status; temp=temp->next;
		}
	}

}
void setEnvPressed(char *list[quant], ll len)
{
	if(len<2 && len > 3)
	{
		fprintf(stdout,"Wrong usage: -> setenv var [val]\n");
	}
	else if(len==2)
	{
		if(setenv(list[one],"",one)==zero)
		{	fprintf(stdout,"Environment Variable Set : %s = %s\n",list[one],""); }
		else{
			fprintf(stdout,"Unable ot set enviroment variable\n");
		}

	}
	else
	{
		if(setenv(list[one],list[2],one)==zero)
		{
			fprintf(stdout,"Environment Variable Set : %s = %s\n",list[one],list[2]);
		}else{
			fprintf(stdout,"Unable ot set enviroment variable\n");
		}
	}
}

void unSetEnvPressed(char* list[quant],ll len)
{
	if(len!=2)
	{
		fprintf(stdout,"Wrong Usage: -> unsetenv var\n");
	}
	else
	{
		if(unsetenv(list[one])==zero)
		{
			fprintf(stdout,"Successfully Usetted the enviroment variable %s.\n",list[one]);
		}
	}
}

void fgPressed(char **list)
{
	if(list[one]==NULL)
		printf("Syntax Error!\n");
	else
	{
		char* ptr;
		pid_t pid=strtol(list[one],&ptr,10);
		//pid_t pid=make_int(list[one]),
		pid_t wpid;
		ll status;
		
		wpid=waitpid(pid, &status,WUNTRACED);
		for(;!WIFEXITED(status) && !WIFSIGNALED(status);)
        {
            wpid=waitpid(pid, &status,WUNTRACED);
        }
	}
}

void bgPressed(char* list[quant],ll len)
{
	/*	ll currentProcess = getpid();
	if(currentProcess!=shell_pid)
	{
	}*/
	if(len !=2)
	{
		printf("Wrong Usage : bg <val>\n");
	}
	char *ptr;
	ll jobnumber = strtol(list[one],&ptr,10);
	ll chkpid;
	if((chkpid=get_job_pid(jobnumber))==-1)
	{
		printf("No such jobid\n");
		return;
	}
	if(search_job(chkpid)==one)
	{
		if(is_job_running(chkpid)==one)
		{
			fprintf(stdout,"Already Running\n");
			return;
		}
		else
		{
			
			change_to_running(chkpid);
			kill(chkpid,SIGTTIN);
			kill(chkpid,SIGCONT);
		}

	}
}

void overKillPressed()
{
	backGroundJobs *curr=ThisJobs;
	for(;curr!=NULL;curr=curr->next)
	{
		pid_t pid=curr->pid;
		kill(pid,SIGQUIT);
	}
}


void kjobPressed(char **list)
{
	if(list[one]==NULL || list[2]==NULL)
	{
		printf("Invalid Syntax usage -> kjob <jobNumber> <signalNumber>");
		return;
	}
	char* ptr1, *ptr2;
	pid_t pid=strtol(list[one],&ptr1,10);
	ll signal=strtol(list[2],&ptr2,10);
	kill(pid,signal);
}

void jobsPressed()
{
	//printf("helo\n");
	backGroundJobs *curr=ThisJobs;
	ll i;
	for (i=one;curr!=NULL;curr=curr->next,i++) {
		printf("[%d] %s %s [%d]\n",i,curr->status,curr->jobName,curr->pid);
	}
	//return one;
}




void pinfoEntered(char* list[quant],ll len)
{
	char path[quant], path1[quant],path2[quant];
	char exePath[100];
	char stat;
	ll proc;
	char pid[10];
	if(len==2)
		sprintf(pid,"%s",list[one]);
	else{
		ll a = getpid();
		sprintf(pid,"%d",a);
	}
	
    sprintf(path2,"/proc/%s/exe",pid);
    ll trash=90;
	sprintf(path,"/proc/%s/stat",pid);
    trash--;
	sprintf(path1,"/proc/%s/statm",pid);
    ll new_trash=trash;
	FILE * filepointer = fopen(path,"r");
	if(filepointer==NULL)
	{
		printf("No status File, no process with pid %s.\n",pid);
		return;
	}
	else
	{
		trash=5;
	}
	fscanf(filepointer, "%d %*s %c",&proc,&stat);
	printf("Process Statues -- %c\n",stat);
	FILE * filepointer1 = fopen(path1,"r");

	if(filepointer!=NULL)
	{
		trash--;
	}

	else if(filepointer==NULL)
	{
		printf("No statm File.\n");
		return;
	}
	long long unsigned ll mem;
	fscanf(filepointer1,"%lu",&mem);
	printf("pid -- %s\n",pid);
	printf("%lu, {Virtual memory}\n\n",mem);
	fclose(filepointer);
	readlink(path2,exePath,sizeof(exePath));
	printf(" -%s\n\n",exePath);


}

void ctrlZ()
{
	if(getpid()!=shell_pid)
		return;
	printf("%d,%d\n",shell_pid,Childpid);

	if(Childpid!=shell_pid && Childpid!=zero)
	{
		char * name = get_job_name(Childpid);
		add_job("Stopped",Childpid,name);
		kill(Childpid,SIGTTIN);
		kill(Childpid,SIGTSTP);
		
	}
	signal(SIGTSTP, ctrlZ);
	

}


void eleminate(ll sig){
  loopflg = zero;
}

void ctrlC()
{

	if(getpid()!=shell_pid)
		return;

	if(Childpid!=zero)
	{
		fprintf(stdout,"\nSending SIGINT to %d.\n",Childpid);
		kill(Childpid,SIGINT);
		
	}
	signal(SIGINT, ctrlC);
	
}


void replaceAll(char * str, char oldChar, char newChar)
{
  ll i = zero;
  while(str[i+zero] != '\0')
  {
      if(str[i+zero] == oldChar)
      {
          str[i+zero] = newChar;
      }
      i+=1;
  }
}


void checkChild()
{
	// This function checks if the child process is still alive or not
	ll st;
	ll *status;
	pid_t a;
	status = &st;
	a= waitpid(-1, status, WNOHANG);
	if(a>zero)
	{
		fprintf(stderr,"pid %d exicted Normally\n",a);
		delete_job(a);
		foreground=zero;
	}
}

ll foreGroundProcess(char* line[quant], ll len)
{
	//Execution of foreground process
	line[len]=NULL;
	currentProcessName = line[one];
	pid_t pid;
  	ll status;
	pid = fork();
	Childpid=pid;
	if (pid == zero)
	{
		// Child process
		if (execvp(line[zero], line) == -1) 
		{
			perror("lsh");
		}
		foreground =one;
		exit(EXIT_FAILURE);
	} 
	else if (pid < zero) 
	{
		// Error forking
		perror("lsh");
	} 
	else 
	{
				waitpid(pid, &status, WUNTRACED);
		for(; !WIFEXITED(status) && !WIFSIGNALED(status) ; )
        {
            waitpid(pid, &status,WUNTRACED);
        }
	}

	return one;
}

ll backGroundProcess(char* line[quant], ll len)
{
	line[len]=NULL;
	pid_t pid;
	pid = fork();
	childpid=getpid();
	Childpid=pid;

	if (pid == zero)
	{
		// Child process
		childpid=getpid();
		printf("pid-- %d\n",childpid);
		if (execvp(line[zero], line) == -1) 
		{
			perror("lsh");
		}
		exit(EXIT_FAILURE);
	} 
	else if (pid < zero) 
	{
		// Error forking
		perror("Error Handled: lsh");
	}
	else
	{
		//add_job(pid,line[zero]);
		// DO NO THING HERE!!
	}
	char path[quant];
	ll proc;
	char stat;
	sprintf(path,"/proc/%d/stat",pid);
	FILE * filepointer = fopen(path,"r");
	if(filepointer==NULL)
	{
		printf("No status File, no process with pid %d.\n",pid);
		return zero ;
	}
	sprintf(path,"/proc/%d/stat",pid);
	fscanf(filepointer, "%d %*s %c",&proc,&stat);
	char* process_status;
	if(stat=='R')
	{
		process_status = "Running";
	}
	else if(stat=='S')
	{
		process_status = "Sleeping";
	}
	else if(stat=='Z')
	{
		process_status="Zombie";
	}
	add_job(process_status,pid,line[zero]);
	return one;
}

ll switchForeOrBackGround(char* line[quant], ll len)
//ll switchForeOrBackGround(char** args)
{
	if(strcmp(line[len-1],"&")==zero)
	{
		//Back Ground Process call
		line[len-one]=NULL;
		len-=one;
		return backGroundProcess(line,len);
	}

	else
	{
		//ForeGround Process call
		childProcName = line[zero];
		return foreGroundProcess(line,len);
	}
	
}

ll chkFlgOrPath(char* list[quant],ll len)
{
	if(len==one)
		return zero;
	ll ret=zero;
	ll i;
	for(i=one;i<len;i++)
	{
		if(strcmp(list[i],"-l")==zero)
			ret+= 2;
		else if(strcmp(list[i],"-a")==zero)
			ret+= one;
		else if(strcmp(list[i],"-la")==zero)
			return 3;
		else if(strcmp(list[i],"-al")==zero)
			return 3;	
	}
	return ret;
}

ll parsePath(char* list[quant],ll len)
{
	ll i;
	for(i=one;i<len;i++)
	{
		if(list[i][zero]!='-' && list[i][zero]!='&')
		{
			return i;
		}
	}
	return -1;
}
void cronjob(char** argv,ll len)
{
	ll l=0,r=zero;
	char* comd[quant];
	for(ll z=0;z<10;z=z+1)
		comd[z]=(char*)malloc(quant*sizeof(char));
	ll i=zero;
	while(strcmp(argv[l],"-c")!=zero)
		l=l+1;
	i=zero;
	while(strcmp(argv[r],"-t")!=zero)
		r=r+one;
	l=l+one;
	r=r+one;
	printf("%d\n",l);
	printf("%d\n",r);
	printf("yo1\n");
	ll p=zero;i=l;
	while(i!=r-one)
	{
		strcpy(comd[p],argv[i]);
		p=p+one;
		i=i+one;
	}
	printf("yo2\n");
	comd[p+zero]=NULL;
	i=zero;
	while(comd[i+zero]!=NULL)
	{
		printf("%s\n",comd[i]);
		i=i+one;
	}
	ll interval=atoi(argv[r]);
	ll total=atoi(argv[r+two]);
	printf("total:%d\n",total);
	printf("interval:%d\n",interval);
	while(total>zero)
	{
		ll pid=fork();
		if(pid==zero)
		{
//		while(total>0)
//		{
			sleep(interval);
			execvp(comd[zero],comd);
//		}
		}
		else if(pid>zero) 
		{
			wait(NULL);
			total-=interval;
//			printf("%d\n",total);
		}
	}
}

void executeInbuild(char* list[quant],ll len)
{
	// Here I am going to implement the 
	// buid in methods defined in 
	// OS Assignement2.pdf
	checkChild();
	if(len==zero)
	{
		return;
	}
	if(strcmp("clear",list[zero])==zero)
		clear();

	else if(strcmp("fg",list[zero])==zero)
		fgPressed(list);

	else if(strcmp("bg",list[zero])==zero)
		bgPressed(list,len);


	else if(strcmp("kjob",list[zero])==zero)
		kjobPressed(list);

	else if(strcmp("jobs",list[zero])==zero)
		jobsPressed();

	else if(strcmp("overkill",list[zero])==zero)
		overKillPressed();

	else if(strcmp("setenv",list[zero])==zero)
		setEnvPressed(list,len);

	else if(strcmp("bg",list[zero])==zero)
		bgPressed(list,len);

	else if(strcmp("unsetenv",list[zero])==zero)
		unSetEnvPressed(list,len);

	else if(strcmp("cd",list[zero])==zero)
	{
		if(len<one)
		{
			printf("Enter the path after cd ,Usgae -> cd <pathname>\n");
		}
		char* replceTilda = list[one];
		if(list[one][zero]=='~'){
			replceTilda = replaceWord(list[one],"~",currentWD);
		//		printf("%s\n",replceTilda );
		}
		if(chdir(replceTilda)==-1)
			printf("No dir called : %s\n",list[one]);
	}

	else if(strcmp("pwd",list[zero])==zero)
	{
		char pwd[PATH_MAX];
		getcwd(pwd,sizeof(pwd));
		printf("%s\n",pwd);
	}
	else if(strcmp("echo",list[zero])==zero)
	{
		ll i=one;
		for(;i<len;i++)
			printf("%s ",list[i]);
		printf("\n");
		// Implement here the meathods echo
		//printf("echo yet not def\n");
	}

	else if(strcmp("ls",list[zero])==zero)
	{
		struct dirent *de;
		DIR* dr = NULL;
		ll flg=zero;
		for(ll i=one;i<len;i++)
		{
			if(list[i][zero]!='-' && list[i][zero]!='&')
			{
				flg = one;
				//char * path;
				dr = opendir(list[i]);
				break;
			}
		}
		if(flg==zero) 	dr = opendir(".");

    char* file[5000];
    if (dr == NULL)
    {
        printf("Could not open file directory" );
        return;
    }
    ll noFile=zero;
    char* space = "  ";
    long ll blksize =zero;
    ll infoLvl = chkFlgOrPath(list,len);
    for (; (de = readdir(dr)) != NULL ;)
    {
    	file[noFile++] = de->d_name;
    	if(infoLvl==zero){
				if(strcmp( ".",de->d_name)!=zero && strcmp("..",de->d_name)!=zero)
					printf("%s%s", file[noFile-one],space);
			}
			else if(infoLvl==one)
				printf("%s%s", file[noFile-one],space);

			else if ((infoLvl==2 && strcmp( ".",de->d_name)!=zero && strcmp("..",de->d_name)!=zero)||infoLvl==3)
			{
				struct stat fileStat;
				struct passwd *pwd;	    
				char date[12];
				ll addrsI = parsePath(list,len);
				if(addrsI>zero)
				{
					char path1[100];
					sprintf(path1,"%s%s",list[addrsI],file[noFile-one]);
					if(stat(path1,&fileStat)<zero)
						return;
				}
				else{
					if(stat(file[noFile-one],&fileStat) < zero)    
		        return ;
				}

		    printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
		    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
		    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
		    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
		    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
		    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
		    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
		    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
		    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
		    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
		    printf(" %ld ",fileStat.st_nlink);

	    	if ((pwd = getpwuid(fileStat.st_uid)) != NULL)
			    printf(" %-8.8s", pwd->pw_name);
				else printf(" %-8d", fileStat.st_uid);
	    	
	    	if ((pwd = getpwuid(fileStat.st_gid)) != NULL)
			    printf(" %-8.8s", pwd->pw_name);
				else printf(" %-8d", fileStat.st_gid);
	    	
	    	printf(" %8ld ",fileStat.st_size);
	    	strftime(date, 20, "%b %d %H:%M ", localtime(&(fileStat.st_mtime)));
	    	printf(" %s ",date);
	    	blksize = blksize + fileStat.st_blocks;
				printf("%s\n", file[noFile-one]);

			}
		}
		if(infoLvl>=2)
		{
	    blksize/=2;
	 		printf("total: %ld\n",blksize);
		 	blksize=zero;
		 	printf("\n");
		}
		printf("\n");
		closedir(dr);
	}
	

	else if(strcmp("pinfo",list[zero])==zero)
	{
		pinfoEntered(list,len);
	}
	else if(strcmp("cronjob",list[zero])==zero)
	{
		cronjob(list,len);
	}
	else switchForeOrBackGround(list,len);
}

void outputFileAt(char* list[quant],ll len, ll n, ll appendOrNot)
{
	ll targetFilefd;
	if(appendOrNot==one)
		targetFilefd = open(list[n+one],O_WRONLY | O_RDONLY |O_APPEND | O_CREAT , 0644 );
	else targetFilefd = open(list[n+one],O_WRONLY | O_RDONLY | O_CREAT , 0644 );
	if(targetFilefd<zero)
	{
		printf("Cannot open file %s.\n",list[n+one]);
		return;
	}
	if(dup2(targetFilefd,one)!=one)
	{
		printf("Error in shifting the filepointer to %s.\n",list[n+one]);
		return;
	}
	len-=2;
	executeInbuild(list,len);
	close(targetFilefd);
	freopen("/dev/tty","w",stdout);
	return;
}

char *getUserName()
{
  uid_t uid = geteuid();
  struct passwd *pw = getpwuid(uid);
  if (pw)
  {
    return pw->pw_name;
  }
  return "";
}

void inputFileFrom(char* list[quant],ll len,ll n)
{
	if(n==len-one)
	{
		printf("Missing Input file name;\nUsage-> python test_code.py < input.txt \n");
		return;
	}
	list[n] = NULL;
	char* targetFilename = list[n+one];
	ll targetFilefd = open(targetFilename, O_RDONLY, 0644);
	if(targetFilefd == -one)
		perror("Failed to open file");
	close(zero); 
	if(dup2(targetFilefd, zero) == -one) 
		perror("dup2 fail");
	
	close(targetFilefd);
	//freopen("/dev/tty","r",stdin);
	printf("Here\n");
}
void chkFile(char* list [quant],ll len)
{
	ll n=zero;
	ll inrediredted = zero;
	ll outredirected = zero;
	ll redirected = zero;
    for(; n<len ;)
	{
		if(strcmp(list[n],"<")==zero)
		{
			inrediredted =one;
			redirected=one;
			printf("InputFile redirection %s.\n",list[n+one]);
			inputFileFrom(list,len,n);
		}
		else if(strcmp(list[n],">")==zero)
		{
			outredirected=one;
			redirected = one;
			if(n==len-one)
			{
				printf("Output file is missing.\n Usage -> command > filename.txt\n");
				return;
			}
			printf("Redirected to file %s!!\n",list[n+one]);
			outputFileAt(list,len,n,zero);
		}
		else if(strcmp(list[n],">>")==zero)
		{
			outredirected=one;
			redirected = one;
			if(n==len-one)
			{
				printf("Output file is missing.\n Usage -> command >> filename.txt\n");
				return;
			}
			printf("Appended to file %s!!\n",list[n+one]);
			outputFileAt(list,len,n,one);
		}
		
		n++;
	}
	if(redirected==zero)
	{
		executeInbuild(list,len);
	}
	if(inrediredted==one)
	{
		//freopen("/dev/tty","r",stdin);
	}

	if(outredirected==one)
	{
		//freopen("/dev/tty","w",stdout);
	}
	executeInbuild(list,len);
}

ll chkInpt(char** list,ll len)
{
	ll i,flg=zero;
		if(strcmp(list[zero],"ls")==zero)
		{
			
			return one;
		}
	
	
}

ll executeCommand (ll in, ll out, struct command *cmd)
{
	if(chkInpt(cmd->argv,cmd->len)==one)
	{
		if(in != zero)
        {
          dup2 (in, zero);
          close (in);
		  
        }

      	if (out != one)
        {
          dup2 (out, one);
          close (out);
        }
		chkFile(cmd->argv,cmd->len);
		return one;
	}
	pid_t pid =fork();
	if(pid==zero)
    {
    	if(in != zero)
        {
          dup2 (in, zero);
          close (in);
        }

      	if (out != one)
        {
          dup2 (out, one);
          close (out);
        }

		//chkFile(cmd->argv,cmd->len);
      	if (execvp (cmd->argv [zero], (char * const *)cmd->argv) == -1)
      	{
      		fprintf(stderr, "Error executing the intermidiate command\n");
      		exit(EXIT_FAILURE);
      	}
      	return one;
    }

  return pid;
}

void fork_pipes (ll b, struct command *cmd)
{
	//ll errLog = open("errLog",O_WRONLY | O_APPEND | O_CREAT,0644);
	//dup2(errLog,2);
  	ll i,result;
  	pid_t pid,ppid;
  	ll in, fd [2];
  	in = zero;

  	for (i=zero;i<b-one;++i)
    {
      pipe (fd);
      executeCommand (in,fd[one],cmd+i);
      close(fd[one]);
      in=fd[zero];
    }
  	if (in != zero)
    	dup2 (in, zero);
	pid=fork();
	if(pid==zero)
  	{
		  chkFile(cmd[i].argv,cmd[i].len);
	}
  	else
  	{
  		ppid = waitpid(pid,&result,zero);
  		for(; !WIFEXITED(result) && !WIFSIGNALED(result) ;)
  			ppid = waitpid(pid,&result,zero);
  	}
  //dup2(2,errLog);
  //close(errLog);
}

//char **pipe_split_line(char *line)
struct command pipe_split_line(char*line)
{
	ll bufsize=BUFFERSIZE_TOKEN, position=zero;
	char **tokens=malloc(bufsize*sizeof(char *));
	char *token;
	char * dfgh;
	if(!tokens)
	{
		fprintf(stderr, "Buffer Allocation Error aboarding\n");
		exit(one);
	}
	token=strtok(line, DILIMITOR_WORD);
	for( ;token!=NULL ;)
	{
		tokens[position+zero]=token;
		position=position+one;
		if(position>=bufsize)
		{
			bufsize = bufsize + BUFFERSIZE_TOKEN;
			tokens=realloc(tokens, bufsize*sizeof(char*));
			if(tokens==0)
			{
				fprintf(stderr, "Allocation error\n");
				exit(one);
			}
		}
		token=strtok(NULL, DILIMITOR_WORD);
	}
	tokens[position+zero]=NULL;
	struct command cmd ;
	cmd.len = position;
	cmd.argv = tokens;
	
	return cmd;
}

ll pipe_split_command(char* line)
{
	ll counter=zero;
	ll bufsize = BUFFERSIZE_TOKEN, position = zero;
	char *token,*token2,*saveptr,*saveptr2,*list[quant];
	struct command cmd[quant];
	token = strtok_r(line, DILIMITOR_COMMAND,&saveptr);

	for ( ;token != NULL ;)
	{
		token2 = strtok_r(token, DILIMITOR_PIPE,&saveptr2);
	//	printf("%s\n",token2);
		for ( ;token2!=NULL ;)
		{
			list[counter] = token2;	
			counter++;
			token2 = strtok_r(NULL, DILIMITOR_PIPE,&saveptr2);
		}
		if(counter>one)
			{
				ll gg = zero;
				for( ;gg<counter ;)
				{
					cmd[gg]=pipe_split_line(list[gg]);
					gg=gg+one;
				}
				ll trash=one;
				fork_pipes(counter,cmd);
			}
		token = strtok_r(NULL, DILIMITOR_COMMAND,&saveptr);
		ll trash=0;
		counter=zero;
		
	}
	return one;
}
void HandlingPiping(char str[PATH_MAX])
{
	pipe_split_command(str);
}

void clearScr()
{
	clear();
}

void splitcommands(char str[PATH_MAX])
{
	char dilim = '|';
	if(strchr(str,dilim)==NULL)
	{

		char *dimlem = " \n";
		char *dimlem1 = ";";
		char* anotherPointer;
		char* anotherPointer1;
		ll looper1=zero;
		char* token;
		char* token1;
		token1 = strtok_r(str,dimlem1,&anotherPointer1);
		for(;token1!=NULL;)
		{
			char* argumentList[quant];
			ll looper=zero;
			token = strtok_r(token1,dimlem,&anotherPointer);
			for(;token!=NULL;)
			{
				argumentList[looper]=token;
                looper+=1;
				token = strtok_r(NULL,dimlem,&anotherPointer);
			}		
			token1 = strtok_r(NULL,dimlem1,&anotherPointer1);
			chkFile(argumentList,looper);

		}
	}
	else
	{
		HandlingPiping(str);
	}
		
}


char* read_command()
{
	ssize_t bufferS = zero;
	char* command = NULL;
	if(getline(&command,&bufferS,stdin)==-1){
		printf("Cannot Read pls enter again.\n");//Cannot read the line
		return read_command();
	}
	
	return command;
}




ll main()
{
	shell_pid = getpid();
	printf("Shell initialized with Process id : %d\n",shell_pid);
	char shellsWD[quant];
	getcwd(currentWD,sizeof(currentWD));
	gethostname(deviceName,sizeof(deviceName));
	userName = getUserName();
	char* args;
	signal(SIGINT,ctrlC);
	signal(SIGTSTP,ctrlZ); 
	while(one)
	{
		Childpid=zero;
		ll a = dup(zero);
		ll b = dup(one);
		getcwd(shellsWD,sizeof(shellsWD));
		char *cwd1 = shellsWD;
		if(strncmp(currentWD,shellsWD,strlen(currentWD))==zero)
				cwd1 = replaceWord(shellsWD,currentWD,"");

		printf("<%s@%s:%s/>\n",userName,deviceName,cwd1);
		//		printf("<%s@%s:~%s>",userName,deviceName,currentWD);
		args = read_command();
		if(strcmp("quit\n",args)==zero)
		{
			free(args);
			//printf("status: %d\n",kill(shell_pid,zero));
			exit(one);
		}
		splitcommands(args);
		update_job_status();
		free(args);
		dup2(a,zero);
		dup2(b,one);
		close(a);
		close(b);
		
	}

}