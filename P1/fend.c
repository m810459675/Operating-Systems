#include <sys/ptrace.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <sys/user.h>
#include <asm/ptrace.h>
#include <sys/wait.h>
#include <asm/unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <string.h>


struct sandbox {
  pid_t child;
  const char *progname;
};

struct sandb_syscall {
  int syscall;
  void (*callback)(struct sandbox*, struct user_regs_struct *regs);
};

struct sandb_syscall sandb_syscalls[] = {
	{__NR_read,            NULL},
	{__NR_write,           NULL},
	{__NR_open,            NULL},
	{__NR_execve,          NULL},
	{__NR_openat,          NULL},
	{__NR_mkdir,		   NULL},
	{__NR_fchmodat,          NULL},
	{__NR_unlinkat ,          NULL},
	{__NR_lstat  ,          NULL},
};

char config_file[80];
char prog_name[80];
int tempFile;
char filePath[256];
const int long_size = sizeof(long);
void getParentpath(char *fileName, char *parentPath){
	int parentLength = strlen(fileName);
	parentLength--;
	parentLength--;
	while(fileName[parentLength] != '/') {
		parentLength--;
	}
	strncpy(parentPath,fileName,parentLength);
	parentPath[parentLength] ='\0';
}
void putdata(pid_t child, long addr,char *str)
{   
    int i;
	int j=0;
    union u {
            long val;
            char chars;
    }data;
    i = 0;
	while(j==0) {
	if(str[i] != '\0'){
		ptrace(PTRACE_POKEDATA, child,addr + i, str[i]);
		data.val = ptrace(PTRACE_PEEKDATA, child, addr+i, NULL);
		i++;
		}
	else{
		ptrace(PTRACE_POKEDATA, child,addr + i, '\0');
		j++;
		}
	}
}
void getdata(pid_t child, long addr, char *str)
{   
    int i;
	int j=0;
    union u {
            long val;
            char chars;
    }data;
    i = 0;
    while(j==0) {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i, NULL);
	if(data.chars != '\0'){
		str[i] = data.chars;
		i++;
	}
	else{
		str[i] = '\0';
		j++;
	}
    }
    str[i] = '\0';
}

void sandb_kill(struct sandbox *sandb) {
	fprintf(stderr, "Terminating %s: unauthorized access of %s\n", prog_name, filePath);
	 kill(sandb->child, SIGKILL);
	 wait(NULL);
	 exit(EXIT_FAILURE);
}
void search_Config(char *file_Name, char *permission );

void sandb_handle_syscall(struct sandbox *sandb) {
	int i;
	struct user_regs_struct regs;
	if(ptrace(PTRACE_GETREGS, sandb->child, NULL, &regs) < 0)
			err(EXIT_FAILURE, "[SANDBOX] Failed to PTRACE_GETREGS:");
	unsigned int flag = regs.rsi;
	

  for(i = 0; i < sizeof(sandb_syscalls)/sizeof(*sandb_syscalls); i++) {
        if(regs.orig_rax == sandb_syscalls[i].syscall) {
			int size;
			char temp[256];
			char filePermission[3]="   ";
			
			//ls - file
			if(regs.orig_rax == __NR_lstat){
				getdata(sandb->child,regs.rdi,filePath);
				search_Config(filePath,filePermission);
				if(filePermission[0] != ' '){
					if(filePermission[0] == '0'){
						//printf("%s file name has %s permission\n", filePath, filePermission);
						//sandb_kill(sandb);
						putdata(sandb->child, regs.rdi ,"/tmp/PersDeniedFile");

					}
				}				
			}
			
			//rm
			if(regs.orig_rax == __NR_unlinkat){
				//printf("Inside unlink");
				getdata(sandb->child,regs.rsi,filePath);
				search_Config(filePath,filePermission);
				if(filePermission[0] != ' '){
					if(filePermission[1] == '0'){
								//printf("%s file name has %s permission\n", filePath, filePermission);
						//sandb_kill(sandb);
						putdata(sandb->child, regs.rsi ,"/tmp/PersDeniedFile");
					}
				}
				
			}
			
			//chmod
			if(regs.orig_rax == __NR_fchmodat){
				
				getdata(sandb->child,regs.rsi,filePath);
				search_Config(filePath,filePermission);
				//printf("Inside chmod %s file name has %s permission\n", filePath, filePermission);
				if(filePermission[0] != ' '){
					if((filePermission[2] == '0' )|| (filePermission[1] == '0') ){
								//printf("%s file name has %s permission\n", filePath, filePermission);
						//sandb_kill(sandb);
						putdata(sandb->child, regs.rsi ,"/tmp/PersDeniedFile");
					}
				}
			}

			//exec
			if(regs.orig_rax == __NR_execve){
				getdata(sandb->child,regs.rdi,filePath);
				search_Config(filePath,filePermission);
				if(filePermission[0] != ' '){
					if(filePermission[3] == '0'){
						//printf("%s file name has %s permission\n", filePath, filePermission);
						//sandb_kill(sandb);
						putdata(sandb->child, regs.rdi ,"/tmp/PersDeniedFile");
					}
				}
			}
			
			//mkdir
			if(regs.orig_rax == __NR_mkdir){
				char path[1000];
				//printf("inside mkdir");
				getdata(sandb->child,regs.rdi,filePath);
				search_Config(filePath,filePermission);
				if(filePermission[0] != ' '){
					if((filePermission[1] == '0')){
						//printf("%s file name has %s permission\n", path, filePermission);
						//sandb_kill(sandb);
						putdata(sandb->child, regs.rdi ,"/tmp/perDeniedDir");
					}
				}
				
				getParentpath(filePath,path);
				search_Config(path,filePermission);
				if(filePermission[0] != ' '){
					if((filePermission[1] == '0') || (filePermission[2] == '0')){
						//printf("%s file name has %s permission\n", path, filePermission);
						//sandb_kill(sandb);
						putdata(sandb->child, regs.rdi ,"/tmp/perDeniedDir");
					}
				}	
			}		
			
			//openAt syscall
			if(regs.orig_rax == __NR_openat){
				//printf("Inside open at");
				getdata(sandb->child,regs.rsi,filePath);
				search_Config(filePath,filePermission);
				if(filePermission[0] != ' '){
					if(filePermission[0] == '0'){
								//printf("%s file name has %s permission\n", filePath, filePermission);
						//sandb_kill(sandb);
						putdata(sandb->child, regs.rsi ,"/tmp/perDeniedDir");
					}
				}
			}
			
			//read and write
			if (regs.orig_rax == __NR_open){
				getdata(sandb->child,regs.rdi,filePath);
				search_Config(filePath,filePermission);
				if(filePermission[0] != ' '){
					if((flag & 1 ) == 0 ){
						//printf("Inside open - read");
						if(filePermission[0] == '0'){
						//printf("%s file name has %s permission\n", filePath, filePermission);
						//sandb_kill(sandb);
						putdata(sandb->child, regs.rdi ,"/tmp/PersDeniedFile");
						}
					} 
					else if((flag & 1 ) == 1 ) {
						//printf("Inside open - write");
						if(filePermission[1] == '0'){
									//printf("%s file name has %s permission\n", filePath, filePermission);
							putdata(sandb->child, regs.rdi ,"/tmp/PersDeniedFile");
							//sandb_kill(sandb);
						}
					}
					else if((flag & 2) == 2){
						//printf("Inside open - read write");
						if(filePermission[0] == '0' || filePermission[1] == '0') {
									//printf("%s file name has %s permission\n", filePath, filePermission);
							//sandb_kill(sandb);
							putdata(sandb->child, regs.rdi ,"/tmp/PersDeniedFile");
					}
				}
			}
      return;
    }
		}
  }

  if(regs.orig_rax == -1) {
    printf("[SANDBOX] Segfault ?! KILLING !!!\n");
        sandb_kill(sandb);
  }
}

void sandb_init(struct sandbox *sandb, int argc, char **argv) {
  pid_t pid;

  pid = fork();

  if(pid == -1)
    err(EXIT_FAILURE, "[SANDBOX] Error on fork:");

  if(pid == 0) {

    if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0)
      err(EXIT_FAILURE, "[SANDBOX] Failed to PTRACE_TRACEME:");
    if(execvp(argv[0], argv) < 0)
      err(EXIT_FAILURE, "[SANDBOX] Failed to execv:");

  } else {
    sandb->child = pid;
    sandb->progname = argv[0];
    wait(NULL);
  }
}

void sandb_run(struct sandbox *sandb) {
  int status;

  if(ptrace(PTRACE_SYSCALL, sandb->child, NULL, NULL) < 0) {
    if(errno == ESRCH) {
      waitpid(sandb->child, &status, __WALL | WNOHANG);
      sandb_kill(sandb);
    } else {
      err(EXIT_FAILURE, "[SANDBOX] Failed to PTRACE_SYSCALL:");
    }
  }

  wait(&status);

  if(WIFEXITED(status))
    exit(EXIT_SUCCESS);

  if(WIFSTOPPED(status)) {
    sandb_handle_syscall(sandb);
  }
}
void search_Config(char *file_Name, char *permission ){
          FILE *cfp;
          if((cfp = fopen(config_file, "r")) != NULL ){
                  char buf[256];
                  char glob_pattern[250];
                  char temp[3];
                  while((fgets(buf,256,cfp)) != NULL) {
                        sscanf(buf,"%s %s",temp,glob_pattern);
                        if (fnmatch(glob_pattern, file_Name, FNM_PATHNAME|FNM_NOESCAPE) == 0){
                sscanf(temp,"%s", permission);
                  }
                }
        fclose(cfp);
  }
}

int main(int argc, char **argv) {
	struct sandbox sandb;
	tempFile = creat("/tmp/PersDeniedFile", 0000);
	char mode[] = "0000";
	int tempPermission;
	tempPermission = strtol(mode, 0, 8);
	chmod ("/tmp/PersDeniedFile",tempPermission);	
	mkdir("/tmp/perDeniedDir", 0000);
  if(argc < 2) {
        printf("Must provide a config file");
        exit(1);
  }
  else if(strcmp(argv[1],"-f") == 0){
        strcpy(config_file, argv[2]);
        strcpy(prog_name, argv[3]);
		sandb_init(&sandb, argc-3, argv+3);
  }
  else{
        strcpy(prog_name, argv[1]);
        strcpy(config_file, getenv("HOME"));
        strcat(config_file, "/.fendrc");
        //default config file
        if( access( "./.fendrc", F_OK ) != -1){
                        strcpy(config_file, "./.fendrc");

        }
        else if( access( config_file, F_OK ) == -1)  {
                        printf("Must provide a config file");
                        exit(1);
        }
		sandb_init(&sandb, argc-1, argv+1);
  }
  
  //sandb_init(&sandb, argc-1, argv+1);
  for(;;) {
    sandb_run(&sandb);
  }
  return EXIT_SUCCESS;
}
