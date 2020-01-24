// Udayaan Nath
// 2017119, CSE
// udayaan17119@iiitd.ac.in

#include<stdio.h>
#include<stdlib.h> 
#include<string.h>
#include<sys/types.h>
#include<pwd.h>
#include<sys/wait.h>
#include<unistd.h>
#include<errno.h>

/* 
get sub-array of argv to pass to execv
*/
int get_argv(int start, int end, char* argv[], char* ptr[]) {

    int i;
    int maxlen = 0;
    
    for (i=0;i<end-start;i++){
        ptr[i] = argv[start+i];
    }
    /* argv ends with a NULL pointer, 
    otherwise Bad address in execv */
    ptr[i] = (char*)0; 
    
    return (end-start+1);
}


int main(int argc, char *argv[])
{
    /* code */
    uid_t ruid = getuid();
    int exit_status;
    uid_t suid = 0;
    int i;
    int* pipe_index = (int *)malloc(sizeof(int));
    int num_commands = 1;

    /* check for pipes in '|' format */
    for (i=0;i<argc;++i) {
        if (strcmp(argv[i],"|")==0){
            pipe_index[num_commands-1] = i;
            num_commands+=1;
            pipe_index = (int *)realloc(pipe_index, num_commands*sizeof(int));
        }
    }
    pipe_index[num_commands-1] = argc;

    /* initializing pipe for each piped command */  
    int fds[num_commands][2];
    for (i=0;i<num_commands;++i) {
            if((exit_status=pipe(fds[i]))!=0){
                printf("%s\n",strerror(errno));
                exit(EXIT_FAILURE);
        }
    }


    int p; 
    char **argvs;
    int size;
    int c = 0;
    int start=1;
    int proc = num_commands-1;

    /* execute each piped command */
    while(c!=num_commands) {
        
        size = pipe_index[c] - start + 1;
        argvs = (char**)malloc(size*sizeof(char*));
        size = get_argv(start,pipe_index[c],argv,argvs);
        int offset=0;

        /* set uid to other user uid */
        if(c==0 && (strcmp(argvs[0],"-u")==0)){
            char *usr_name = argvs[1];
            struct passwd* user; 
            if((user=getpwnam(usr_name))==NULL){
                printf("User doesn't exist\n!");
                exit(EXIT_FAILURE);
            }
            suid = user->pw_uid; 
            seteuid(suid);
            offset = 2;
            
        }
        
        if((p=fork())==-1){
            printf("%s\n",strerror(errno));
            exit(EXIT_FAILURE);
        }

        if(p==0) {
            if(c>0) {
                /* get output from previous piped command as input */
                dup2(fds[c-1][0],0);
            }
            
            /* change stdout of current proc to writting end of the pipe */
            close(1);
            dup(fds[c][1]);
            close(fds[c][0]);

            if((exit_status = execv(argvs[offset],argvs+offset))!=0){
                printf("%s\n",strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        close(fds[c][1]);
        waitpid(p,&exit_status,0);
        
        /* revoke the priviledges */
        setuid(ruid);

        if(exit_status!=0){
            proc = c;
            break;
        }
        
        start = pipe_index[c]+1;
        c+=1;

        free(argvs);

    }

    char buf[1];
    int nbytes;
    while((nbytes=read(fds[proc][0],buf,1))>0){
        printf("%s",buf);
    }

    exit(exit_status);

}
