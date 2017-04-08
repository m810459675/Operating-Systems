#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>

#include"parse.h"

extern char **environ;

int pipeId[2];
char promptDisplay[512];
char *homePath;
int isExiting,isPromptRunning = 0;
int tempStdIn,tempStdOut,tempStdErr,processId,status,tempRcStdIn, isRcPresent=0 ,isRcProcessed =1,sigFlag=0;

void prCmd(Cmd cmd);


void execBuildIns(Cmd buildInCmd){
	if((strcmp(buildInCmd->args[0],"echo") == 0) && (buildInCmd->args[1]!=NULL)){
		int i=1;
		while(buildInCmd->args[i]!=NULL){
			printf("%s ",buildInCmd->args[i]);
			i++;
		}
		printf("\n");
	}
	else if(strcmp(buildInCmd->args[0],"cd") == 0){
		if(buildInCmd->args[1] == NULL){
			buildInCmd->args[1] = malloc(strlen(homePath));
			strcpy(buildInCmd->args[1],homePath);
		}
		if(buildInCmd->args[1][0]=='~'){
			char *cmdPath;
			int tempLength = 0;
			cmdPath = strtok(buildInCmd->args[1],"~");
			if(cmdPath)
				tempLength = strlen(cmdPath);
			buildInCmd->args[1] = malloc(strlen(homePath)+tempLength);
			strcpy(buildInCmd->args[1],homePath);
			if(cmdPath)
				strcat(buildInCmd->args[1],cmdPath);
		}
		if(	(buildInCmd->args[1])){
			char *pathToken;
			pathToken=strtok(buildInCmd->args[1],"/");
			if(buildInCmd->args[1][0] == '/'){
				chdir("/");
			}
			while(pathToken != NULL){
				if(pathToken[0]!='~'){
					chdir(pathToken);
				}
				pathToken = strtok(NULL,"/");
			}
		}
		else{
			fprintf(stderr,"No such file or directory\n");
		}
	}
	else if(strcmp(buildInCmd->args[0],"pwd") == 0){
		char *currentDirectory;
		currentDirectory = (char*)get_current_dir_name();
		printf("%s\n",currentDirectory);
	}
	else if(strcmp(buildInCmd->args[0],"nice") == 0){
		if(buildInCmd->args[1]!=NULL)
			setpriority(PRIO_PROCESS, 0, atoi(buildInCmd->args[1]));
		else
			setpriority(PRIO_PROCESS, 0, 4);

		//A child created by fork(2) inherits its parent's nice value. The nice value is preserved across execve(2).
		if(buildInCmd->args[1]!=NULL && (buildInCmd->args[2]!=NULL)){
			Cmd niceCmd;
			niceCmd = malloc(sizeof(struct cmd_t));
			niceCmd->args = malloc(sizeof(char*));
			niceCmd->args[0] = malloc(strlen(buildInCmd->args[2]));
			strcpy(niceCmd->args[0],buildInCmd->args[2]);
			prCmd(niceCmd);
		}
	}
	else if(strcmp(buildInCmd->args[0],"logout") == 0){
			exit(0);
	}
	else if(strcmp(buildInCmd->args[0],"setenv") == 0){
		if(buildInCmd->args[1] == NULL){
			// Below logic is referred from Stack over flow link mentioned in REFERANCES
			char **temp = environ;
			while(*temp){
				printf("%s\n",*temp);
				*temp++;
			}
			return;
		}
		else{
			if(buildInCmd->args[2] == NULL)
				strcpy(buildInCmd->args[2],"");
			setenv(buildInCmd->args[1],buildInCmd->args[2],1);
		}
	}
	else if(strcmp(buildInCmd->args[0],"unsetenv") == 0){
		if(buildInCmd->args[1] != NULL){
			unsetenv(buildInCmd->args[1]);
		}
		else{
			fprintf(stderr,"Missing argumnets. Usage:  unsetenv VAR\n");
		}
	}
	else if(strcmp(buildInCmd->args[0],"where") == 0){
		if(buildInCmd->args[1] != NULL){
			if((strcmp(buildInCmd->args[1],"echo")==0)||(strcmp(buildInCmd->args[1],"cd")==0)
						||(strcmp(buildInCmd->args[1],"pwd")==0)||(strcmp(buildInCmd->args[1],"nice")==0)
						||(strcmp(buildInCmd->args[1],"logout")==0)||(strcmp(buildInCmd->args[1],"setenv")==0)
						||(strcmp(buildInCmd->args[1],"unsetenv")==0)||(strcmp(buildInCmd->args[1],"where")==0)
						||(strcmp(buildInCmd->args[1],"kill")==0)){
				printf("%s is a built-in command\n",buildInCmd->args[1]);
			}

			//TODO - Check if printf is needed for all commands
			char outputPath[512]="";
			char *subPath,*temp;
			temp = strdup(getenv("PATH"));
			subPath = strtok(temp,":");
			while(subPath != NULL){
				strcpy(outputPath,subPath);
				strcat(outputPath,"/");
				strcat(outputPath,buildInCmd->args[1]);
				if(!checkPath(outputPath))
					printf("%s\n",outputPath);
				subPath = strtok(NULL,":");
			}
		}
		else{
			fprintf(stderr,"Missing argumnets. Usage:  where command\n");
		}
	}
	else if(strcmp(buildInCmd->args[0],"kill") == 0){
		if(buildInCmd->args[1] != NULL){
			char *job_number;
			job_number = strtok(buildInCmd->args[1],"%");
			if(kill(job_number, SIGTERM))
				fprintf(stderr,"Unable to kill the job %s\n", job_number);
		}
		else{
			fprintf(stderr,"Missing argumnets. Usage:  kill job_id\n");
		}

	}
}


//Below logic referrred from stack flow link in the REFERANCES
int checkDirectory(char *inputPath){
	int returnValue;
	struct stat *buf;
	buf = malloc(sizeof(struct stat));
	if (stat(inputPath, buf) == 0 && S_ISDIR(buf->st_mode))
		returnValue = 1;
	else
		returnValue =  0;
	free(buf);
	return returnValue;
}

int checkPath(char *cmd_path){
	if(access(cmd_path,F_OK) !=0 )
		return 1;
	if( access(cmd_path,R_OK|X_OK) !=0 || checkDirectory(cmd_path))
		return 1;
	return 0;
}

void prPipe(){
	Pipe cmdLineIn;
	int childProcessId;
	while(1){
		char *endValue = "end";
		if(isPromptRunning == 0  && (isRcProcessed ==1)){
			printf("%s",promptDisplay);
			fflush(stdout);
			isPromptRunning = 1;
		}
		if((cmdLineIn = parse()) == NULL){
			isPromptRunning = 0;
			continue;
		}

		if(strcmp(endValue,cmdLineIn -> head -> args[0])==0) {
			if(isRcPresent && (isRcProcessed ==0)){
				dup2(tempRcStdIn,0);
				close(tempRcStdIn);
				isPromptRunning = 0;
				isRcProcessed = 1 ;
				continue;
			}
			return;
		}
		isPromptRunning = 0;
		while(cmdLineIn != NULL && sigFlag != 1){
			Cmd currentCmd = cmdLineIn -> head;
			Cmd temp  = NULL;
			Cmd lastCmd;
			childProcessId = processId = isExiting = 0;
			//Back up the streams
			tempStdIn = dup(0);
			tempStdOut = dup(1);
			tempStdErr = dup(2);

			while(currentCmd!=temp){
				lastCmd = currentCmd;
				while(lastCmd->next != temp){
					lastCmd=lastCmd->next;
				}
				temp = lastCmd;
				if(currentCmd != temp){
					pipe(pipeId);
					childProcessId = fork();
					if(childProcessId != 0) {
						temp = currentCmd; // this makes the parent to complete the loop
						close(pipeId[1]);
						dup2(pipeId[0],0);
						break;
					}
					else{
						close(pipeId[0]);
						dup2(pipeId[1],1);
						isExiting = 1; // This is child and should exit post completion
					}
				}
			}
			prCmd(lastCmd);
			waitpid(childProcessId,&status,0);
			if(isExiting)
				exit(0);
			//Rollback the streams
			dup2(tempStdIn,0);
			dup2(tempStdOut,1);
			dup2(tempStdErr,2);
			//Close the temp streams
			close(tempStdErr);
			close(tempStdOut);
			close(tempStdIn);

			cmdLineIn = cmdLineIn -> next;
		}
	}
}

//Below logic taken from prof's main.c
void prCmd(Cmd inputCmd){
	int temp;
	if(inputCmd == NULL)
		return;
	//Input redirection
	if(inputCmd->in == Tin){
		temp = open(inputCmd->infile,O_RDONLY);
		dup2(temp,0);
		close(temp);
	}

	//Ouput Redirection
	switch ( inputCmd->out ){
		case TappErr:
			temp = open(inputCmd->outfile,O_CREAT|O_APPEND|O_WRONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
			dup2(temp,2);
			close(temp);
			//no break here as >>& also appends the output
		case Tapp:
			temp = open(inputCmd->outfile,O_CREAT|O_APPEND|O_WRONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
			dup2(temp,1);
			close(temp);
			break;
		case ToutErr:
			temp = open(inputCmd->outfile,O_CREAT|O_TRUNC|O_WRONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
			dup2(temp,2);
			close(temp);
			//no break here as >& also redirects the output
		case Tout:
			temp = open(inputCmd->outfile,O_CREAT|O_TRUNC|O_WRONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
			dup2(temp,1);
			close(temp);
			break;
		case Tpipe:
			//TODO
			break;
		case TpipeErr:
			//TODO
			break;
	}

	//execute the input cmd
	if((strcmp(inputCmd->args[0],"echo")==0)||(strcmp(inputCmd->args[0],"cd")==0)
			||(strcmp(inputCmd->args[0],"pwd")==0)||(strcmp(inputCmd->args[0],"nice")==0)
			||(strcmp(inputCmd->args[0],"logout")==0)||(strcmp(inputCmd->args[0],"setenv")==0)
			||(strcmp(inputCmd->args[0],"unsetenv")==0)||(strcmp(inputCmd->args[0],"where")==0)
			||(strcmp(inputCmd->args[0],"kill")==0))
		execBuildIns(inputCmd);
	else{
		if(strstr(inputCmd->args[0],"/")== NULL) {
			//Search for the command in the path varables
			char argCmdPath[512];
			char *subPath,*temp;
			temp = strdup(getenv("PATH"));
			subPath = strtok(temp,":");
			while(subPath != NULL){
				strcpy(argCmdPath,subPath);
				strcat(argCmdPath,"/");
				strcat(argCmdPath,inputCmd->args[0]);
				if(!checkPath(argCmdPath))
					break;
				subPath = strtok(NULL,":");
			}
			strcpy(inputCmd->args[0],argCmdPath);
		}
		if(access(inputCmd->args[0],F_OK) == -1 ){
			fprintf(stderr,"command not found\n");
			return;
		}
		if(access(inputCmd->args[0],R_OK|X_OK) == -1  || checkDirectory(inputCmd->args[0])){
			fprintf(stderr,"permission denied\n");
			return;
		}
		processId = fork();
		if(processId == 0){
			execvp(inputCmd->args[0],inputCmd->args);
			exit(0);
		}
		else if(processId != -1){
			waitpid(processId,&status,0);
		}
	}
}

void initHandler(){
	printf("\n%s",promptDisplay);
	fflush(stdout);
}

void termHandler(){
	killpg(getpgrp(),SIGTERM);
	exit(0);
}

int main(){
	strcpy(promptDisplay,"");
	signal(SIGTSTP,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGINT,initHandler);
	signal(SIGTERM,termHandler);
	char hostname[512];
	if(gethostname(hostname,sizeof(hostname)) == 0)
		strcpy(promptDisplay,hostname);
	strcat(promptDisplay,"% ");
	char rcFilePath[512];
	homePath = getenv("HOME");
	strcpy(rcFilePath,strdup(homePath));
	strcat(rcFilePath,"/.ushrc");
	int	tempFd = open(rcFilePath,O_RDONLY);
	if(tempFd != -1){
		tempRcStdIn = dup(0);
		dup2(tempFd,0);
		close(tempFd);
		isRcPresent = 1;
		isRcProcessed = 0;
		}
	prPipe();
}
