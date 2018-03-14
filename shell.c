#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXARGS 128
#define MAXLINE 100
#define MAXCOMMAND 100
#include <unistd.h>
#include <sys/types.h>   // 提供类型 pid_t 的定义
#include <sys/wait.h>
#include <errno.h>
#include<fcntl.h>
typedef struct pipe_node
{
  char *pipe_argv[MAXARGS];
  struct pipe_node* next;
}pipe_node;
char his[MAXCOMMAND][MAXLINE];
int k = 0;
int flag = 0;
void eval(char *cmdline);
int parseline(char* buf,char** argv,int* p_argc);
void creat_pipe(char* buf);
int pipe_line(char* buf,pipe_node* p_head,char** file);
int builtin_command(char** argv);
int main()
{
    char cmdline[MAXLINE];//command line
    while(1)
    {
        printf("$");//prompt
        flag = 0;
        fgets(cmdline,MAXLINE,stdin);
        strcpy(his[k++],cmdline);
        if(feof(stdin))
            exit(0);
        //evaluate
        eval(cmdline);
    }
    return 0;
}
void eval(char *cmdline)
{
    char *argv[MAXARGS];//argument list
    char buf[MAXLINE];//command line
    int argc = 0;
    int bg;// function parseline return value
    int fd;
    pid_t pid;//process id
    strcpy(buf,cmdline);
    bg = parseline(buf,argv,&argc);

    if(flag==3)
    {
        creat_pipe(buf);
        return;
    }
    if(argv[0]==NULL)
        return;
    if(!builtin_command(argv))
    {
        if(flag==1)
        {
            if((pid = fork())==0)
            {
                fd = dup(1);
                close(1);
                open(argv[argc-1],O_WRONLY|O_CREAT,0777);
                //lseek(fd,0,SEEK_SET);
                if(execv(argv[0],argv)<0)
                {
                    printf("%s: Command not found.\n",argv[0]);
                    exit(0);
                }
            }
        }
        if(flag==4)
        {
            if((pid = fork())==0)
            {
              fd = dup(0);
              close(0);
              if(open(argv[argc-1],O_RDONLY,0777)<0)
              {
                  printf("File not found!\n");
                  exit(0);
                }
                if(execv(argv[0],argv)<0)
                {
                  printf("%s:Command not found.\n",argv[0]);
                  exit(0);
                }
              }
            }
        if(flag==2)
        {
            if((pid=fork())==0)
            {

                fd = dup(1);
                close(1);
                open(argv[argc-1],O_WRONLY|O_APPEND|O_CREAT,0777);
                //lseek(fd,0,SEEK_END);
                if(execv(argv[0],argv)<0)
                {

                    printf("%s: Command not found.\n",argv[0]);
                    exit(0);
                }

                printf("fd = %d\n",fd);
            }
        }
        if(flag==0)
        {
            if((pid = fork())==0)
            {
                if(execv(argv[0],argv)<0)
                {
                    printf("%s: Command not found.\n",argv[0]);
                    exit(0);
                }
            }
        }
        if(!bg)
        {
          int status;
          if(waitpid(pid,&status,0)<0)
          {
            printf("waitfg:waitpid error\n");
          }

        }
        else
        {
          printf("%d %s",pid, cmdline);
        }
  }
    return;
}
int parseline(char* buf,char** argv,int*p_argc)
{
    char *delim;//
    int bg = 0;// background or not?
    char* p_temp;

    buf[strlen(buf)-1] = ' ';
    while(*buf&&(*buf==' '))
        buf++;
    p_temp = buf;
    if((delim = strchr(p_temp,'|')))
    {
      flag = 3;
      buf[strlen(buf)-1] = '\0';
    }
    else
    {
      while((delim = strchr(p_temp,'>')))//redirection override
      {
          if(flag==1)
            flag = 2;
          else
          {
            flag = 1;
          }
          *delim = ' ';
      }
      if((delim = strchr(p_temp,'<')))
      {
        flag  = 4;
        *delim = ' ';
      }
        while((delim = strchr(buf,' ')))
        {
            *delim = '\0';
            argv[(*p_argc)++] = buf;
            buf = delim+1;
            while(*buf&&(*buf==' '))
                buf++;
        }
        argv[*p_argc] = NULL;
        if(*p_argc == 0)
            return 1;
        if((bg = (*argv[(*p_argc)-1]=='&'))!=0)
        {
            argv[--(*p_argc)] = NULL;
        }

    }
    return bg;
}
void creat_pipe(char* buf)
{
  int fd[2];
  int k = 0;
  int bg;
  int status;
  int fd_pre;
  int fd_file;
  pid_t pid;
  char* file = NULL;
  pipe_node* p_head;
  p_head = (pipe_node*)malloc(sizeof(pipe_node));
  pipe_node* p_temp = p_head;
  bg = pipe_line(buf,p_head,&file);
  while(p_temp->next)
  {
    pipe(fd);
    if((pid = fork())!=0)
    {
      if(waitpid(pid,&status,0)<0)
      {
        printf("wait error!\n");
      }
      fd_pre = fd[0];
      //close(fd[0]);
      //close(fd[1]);
      //if(execv(p_temp->pipe_argv[0],p_temp->pipe_argv)<0)
      //{
        //printf("error!\n");
      //}
      p_temp = p_temp->next;
    }
    else
    {
      if(p_temp==p_head)
      {
        p_temp = p_temp->next;
        //close(fd[0]);
        if(flag==4)
        {
          fd_file = open(file,O_RDONLY,0777);
          if(fd_file<0)
          {
            printf("file not found!\n");
            exit(0);
          }
          close(0);
          dup(fd_file);
        }
        close(1);
        dup(fd[1]);
        close(fd[1]);
        close(fd_file);
        fd_pre = fd[0];
      }
      else
      {
        p_temp = p_temp->next;
        if(p_temp->next ==NULL)
        {
          close(fd[1]);
          close(0);
          dup(fd_pre);
          if(flag==1)
          {
            fd_file = dup(1);
            close(1);
            open(file,O_CREAT|O_WRONLY,0777);
          }
          if(flag==2)
          {
            fd_file = dup(1);
            close(1);
            open(file,O_WRONLY|O_APPEND,0777);
          }
          close(fd[0]);
          close(fd_file);
          close(fd_pre);
        }
        else
        {
          //close(fd[1]);
          close(0);
          dup(fd_pre);
          close(fd_pre);
          //close(fd[0]);//read from pipe
          close(fd[0]);
          close(1);
          dup(fd[1]);
          close(fd[1]);
          fd_pre = fd[0];
        }
      }
      if(execv(p_temp->pipe_argv[0],p_temp->pipe_argv)<0)
      {
        printf("error\n");
        exit(0);
      }
    }
  }
}
int pipe_line(char* buf,pipe_node* p_head,char**file)
{
  int argc;
  char* delim;
  int bg = 0;
  char* pip_delim = NULL;
  pipe_node *p_temp = p_head;
  pipe_node *p_par = p_head;
  while((delim = strchr(buf,'|')))
  {
    argc = 0;
    *delim = '\0';
    p_temp = (pipe_node*)malloc(sizeof(pipe_node));
    p_par->next = p_temp;
    while(*buf&&(*buf==' '))
      buf++;
    while((pip_delim = strchr(buf,' ')))
    {
      *pip_delim = '\0';
      p_temp->pipe_argv[argc++] = buf;
      buf = pip_delim+1;
      while(*buf&&(*buf==' '))
        buf++;
    }
    p_temp->pipe_argv[argc++] = buf;
    buf = delim+1;
    p_par = p_temp;
  }
  if(*buf)
  {
    argc = 0;
    p_temp = (pipe_node*)malloc(sizeof(pipe_node));
    p_par->next = p_temp;
    while((pip_delim = strchr(buf,'>')))
    {
      if(flag==1)
        flag=2;
      else
        flag = 1;
      *pip_delim = ' ';
    }
    if(pip_delim = strchr(buf,'<'))
    {
      flag = 4;
      *pip_delim = ' ';
    }
    while((pip_delim = strchr(buf,' ')))
    {
      *pip_delim = '\0';
      p_temp->pipe_argv[argc++] = buf;
      buf = pip_delim+1;
      while(*buf&&(*buf==' '))
        buf++;
    }
    p_temp->pipe_argv[argc++] = buf;
    if(flag!=3)
    {
      *file = p_temp->pipe_argv[argc-1];
    }
    p_temp->next = NULL;
  }
  if((bg = (*(p_temp->pipe_argv[argc-1])=='&'))!=0)
  {
    p_temp->pipe_argv[--argc] = NULL;
  }
  return bg;
}
int builtin_command(char**argv)
{
  int i;
  int sum;
  if(!strcmp(argv[0],"cd"))
  {
    if(chdir(argv[1])==-1)
    {
      printf("can't find path\n");
    }
    return 1;
  }
  if(!strcmp(argv[0],"exit"))
  {
    exit(0);
  }
  if(!strcmp(argv[0],"history"))
  {
    if(argv[1]!=NULL)
    {
      sum = 0;
      for(i = 0;argv[1][i]!='\0';i++)
      {
        sum=sum*10+(argv[1][i]-'0');
      }
      k = (sum>k)?k:sum;
    }
    for(i = 0;i<k;i++)
    {
      printf("%s\n",his[i]);
    }
    return 1;
  }
  return 0;
}
