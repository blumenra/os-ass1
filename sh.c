// Shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"

// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

//OUR IMPL.
#define MAX_HISTORY 16 
#define MAX_CMD_SIZE 128
#define NULL 0

int indexOfNext = 0;
char historyArr[MAX_HISTORY][MAX_CMD_SIZE];

void history_append(char*);
void printHistory(void);
void initCmdArray(char*);
char* getHistoryCMD(int, char*);

void getFirstDollarVar(char*, int*, int*);
void append_str(char*, char*);
int getIndexOfChar(char, char*);
void resetStr(char*);

void resetStr(char* buf){

  int i;
  for(i=0; i < strlen(buf); i++){
    
    buf[i] = 0;
  }
}

int getIndexOfChar(char c, char* str){

  //printf(1, "strlen: %d\n", strlen(str));  
  int i;
  for(i=0; i < strlen(str); i++){

    //printf(1, "c: %c\n", c);
    //printf(1, "str[%d]: %c\n", i, str[i]);
    if(c == str[i])
      return i;
  }

  return -1;
}

void append_str(char* buf, char* str){

  strncpy(buf+strlen(buf), str, strlen(str));
}

void getFirstDollarVar(char* buf, int* index, int* len){

  *index = getIndexOfChar('$', buf);
  if(*index < 0){
    *len = -1;
    return;
  }

  int end1 = getIndexOfChar('$', buf+*index+1);
  int end2 = getIndexOfChar(' ', buf+*index+1);
  if(end1 <= 0){
    if(end2 <= 0)
      *len = strlen(buf+*index); // len is the last $var len including the '$'
    else
      *len = end2-*index+1; // include ' '
  }
  else
    if(end2 <= 0)
      *len = end1-*index;
    else
      if(end1 < end2)
        *len = end1-*index;
      else
        *len = end2-*index;
}

void initCmdArray(char* array){

  int i;
  int length = strlen(array);
  for(i=0; i < length; i++){

    array[i] = NULL;
  }
}

void history_append(char* cmd){

  if(cmd[0] == '\n')
    return;

  cmd[strlen(cmd)-1] = 0;  // chop \n
    
  strcpy(historyArr[indexOfNext], cmd);

  indexOfNext++;
  indexOfNext = indexOfNext % MAX_HISTORY;

}

void printHistory(){

  int i;
  // if there were no 16 cmds yet
  if(historyArr[indexOfNext][0] == NULL){

    for(i=0; i < indexOfNext; i++){
      printf(1, "    %d:  %s\n", i+1, historyArr[i]);
    }
  }
   else{
     int j;
     i = indexOfNext;
     for(j=0; j < MAX_HISTORY; j++){
       printf(1, "    %d:  %s\n", j+1, historyArr[i]);
       i++;
       i = i % MAX_HISTORY;
     } 
   }
}

char* getHistoryCMD(int index, char* buf){

  strcpy(buf, historyArr[index-1]);
  return buf;
}

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit();

  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit();
    exec(ecmd->argv[0], ecmd->argv);
    printf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      printf(2, "open %s failed\n", rcmd->file);
      exit();
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if(fork1() == 0)
      runcmd(lcmd->left);
    wait();
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait();
    wait();
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    break;
  }
  exit();
}

int
getcmd(char *buf, int nbuf)
{
  printf(2, "$ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  int fd;

  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  // creates 16 new buffers for the history commands
  int i;
  for(i=0; i < MAX_HISTORY; i++){
    initCmdArray(historyArr[i]); // initializes tmp with NULL
  }

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    
    //Add command to history array
    history_append(buf); // chops '\n' of buf in side

    //TESTS. REMOVE ME
    char retVal[MAX_CMD_SIZE];
    char retVal2[MAX_CMD_SIZE];
    char* var = "x";
    char* value = "alon";
    char* var2 = "y";
    char* value2 = "123";
    if(setVariable(var, value) == 0){
        printf(1, "Set %s to %s properly\n", var, value);
      
      if(setVariable(var2, value2) == 0)
        printf(1, "Set %s to %s properly\n", var2, value2);

      if(getVariable(var, retVal) == 0)
        printf(1, "returned %s=%s\n", var, retVal);

      if(getVariable(var2, retVal2) == 0)
        printf(1, "returned %s=%s\n", var2, retVal2);

      if(setVariable(var, value2) == 0)
        printf(1, "Set %s to %s properly\n", var, value2);

      if(getVariable(var, retVal2) == 0)
        printf(1, "returned %s=%s\n", var, retVal2);

      if(getVariable("bla", retVal2) == 0)
        printf(1, "returned %s=%s\n", "bla", retVal2);

      if(remVariable(var) == 0)
        printf(1, "returned %s=%s\n", var, retVal2);
    }

    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Chdir must be called by the parent, not the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        printf(2, "cannot cd %s\n", buf+3);
      continue;
    }

    //HISTORY
    if(strcmp(buf, "history") == 0){
      printHistory();
      continue;
    }
    else if(strncmp(buf, "history -l", 10) == 0){ // history -l ##

      char cmd[MAX_CMD_SIZE-10];
      int index = atoi(buf+11);

      if(index > 16){
	      printf(2, "index %d is invalid! Choose between 1-16\n", index);
        continue;
      }
      
      getHistoryCMD(index, cmd); //get the index-th cmd in history
      
      // IF the chosen cmd from hiatory is "history"..
      if(strcmp(cmd, "history") == 0){
        printHistory();
	      continue;
      }
      
      if(fork1() == 0)
        runcmd(parsecmd(cmd));
      wait();

      continue; 
    }



    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait();
  }
  exit();
}

void
panic(char *s)
{
  printf(2, "%s\n", s);
  exit();
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    printf(2, "leftovers: %s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while(peek(ps, es, "&")){
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;

  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;
  }
  return cmd;
}
