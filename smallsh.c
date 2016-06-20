/*********************************************************************
 ** Program Filename: smallsh.c 
 ** Author: Peter Nguyen
 ** Date: 2/28/16
 ** CS 344-400, Program 3
 ** Description: This program is a mini-shell that supports three
 ** built-in commands. All other commands are executed via fork() and
 ** exec(). It also supports redirection of input/output.
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "dynamicArray.h"

const int MAX_LINE_LTH = 2049;
struct sigaction action;

// Function prototypes
int  commandPrompt(struct DynArr* list, char* statusMsg);
void cdCommand(char* token);
void statusCommand(char* statusMsg);
void exitCommand(struct DynArr* list);
void otherCommand(char* token, struct DynArr* list, char* statusMsg);
void getExitStatus(int status, char* statusMsg);
void redirectOutput(char* fileName);
void redirectInput(char* fileName);
void checkBackgroundJobs(struct DynArr* list, char* statusMsg);

int main()
{
  int exitShellFlag = 0;
  char statusMessage[256] = "no current foreground process\n";
  struct DynArr* processList; // stores background processes

  processList = createDynArr(10);
  
  // Define signal handler
  action.sa_handler = SIG_IGN;
  action.sa_flags = 0;
  sigfillset(&(action.sa_mask));
  sigaction(SIGINT, &action, NULL);

  // Execute the shell while command is not "exit"
  exitShellFlag = commandPrompt(processList, statusMessage);
  while (!exitShellFlag)
  {
    // Check for completion of any background jobs
    if (!isEmptyDynArr(processList))
      checkBackgroundJobs(processList, statusMessage);
    
    exitShellFlag = commandPrompt(processList, statusMessage);
  }

  deleteDynArr(processList);
  return 0;
}

/*********************************************************************
 ** commandPrompt
 ** Description: Gets the command line input from the user. Returns
 ** true (1) if the exit command was given
 ** Parameters: struct DynArr* list, char* statusMsg
 *********************************************************************/
int commandPrompt(struct DynArr* processList, char* statusMsg)
{
  char input[MAX_LINE_LTH];
  char* token;
  
  // Display command prompt and get input from user
  printf(": ");
  fflush(stdout);
  fgets(input, MAX_LINE_LTH, stdin);

  // Check for blank lines and comments
  if (input[0] == '\n' || input[0] == '#') 
    return 0;
  else
    input[strlen(input) - 1] = '\0'; // remove newline

  token = strtok(input, " ");

  // Check if one of the three built-in commands 
  // or some other command was input
  if (strcmp(token, "cd") == 0)
    cdCommand(token);
  else if (strcmp(token, "status") == 0)
    statusCommand(statusMsg);
  else if (strcmp(token, "exit") == 0)
  {  
    exitCommand(processList);
    return 1; // return true - exit shell
  }
  else
    otherCommand(token, processList, statusMsg);

  return 0;   // return false - no exit
} 

/*********************************************************************
 ** otherCommand
 ** Description: Executes any other commands that are not built into
 ** the shell by forking and passing it to exec function
 ** Parameters: char* token, struct DynArr* list, char* statusMsg
 *********************************************************************/
void otherCommand(char* token, struct DynArr* processList, char* statusMsg)
{
  pid_t childPID,
        endPID;
  int status,
      argIndex = 0,
      outRedirectFlag = 0, // various flags set if I/O redirection
      inRedirectFlag = 0,  // or backround process is specified
      backgroundFlag = 0;
  char* arg[513];
  char  outFileName[128],
        inFileName [128];

  // Check command input for I/O redirection or background
  // process. Otherwise, add the token to arguments array
  
  while (token != NULL)
  {
    if (strcmp(token, ">") == 0)
    {
      outRedirectFlag = 1;
      token = strtok(NULL, " "); // Get the output file name
      strcpy(outFileName, token);
    }
    else if (strcmp(token, "<") == 0)
    {
      inRedirectFlag = 1;
      token = strtok(NULL, " "); // Get the input file name
      strcpy(inFileName, token);
    }
    else if (strcmp(token, "&") == 0)
      backgroundFlag = 1;
    else
    {
      arg[argIndex] = token;
      argIndex++;
    }
    token = strtok(NULL, " ");
  }
  arg[argIndex] = NULL;

  // Fork the process

  childPID = fork();

  switch (childPID)
  {
    case -1: // Fork failure
      printf("smallsh: fork failed\n");
      fflush(stdout);
      exit(1);
      break;

    case 0: // Child: exec the command
      if (!backgroundFlag)
      {
        action.sa_handler = SIG_DFL;
        sigaction(SIGINT, &action, NULL);
      }

      // Perform any I/O redirection
      if (outRedirectFlag)
        redirectOutput(outFileName);

      if (inRedirectFlag)
        redirectInput(inFileName);

      // Redirect background process I/O to dev/null
      if (backgroundFlag && !outRedirectFlag)
        redirectOutput(NULL);

      if (backgroundFlag && !inRedirectFlag)
        redirectInput(NULL);

      // Execute command
      execvp(arg[0], arg);
      // if exec fails
      printf("smallsh: no such command\n");
      fflush(stdout);
      exit(1);
      break;

    default: // Parent: handle background or foreground process
      if (backgroundFlag)
      {
        printf("background pid is %d\n", childPID);
        fflush(stdout);
        pushDynArr(processList, childPID); // store background process ID
      }
      else
      {
        endPID = waitpid(childPID, &status, 0); // wait for child
        if (endPID != -1)                       // to finish (foreground)
          getExitStatus(status, statusMsg);
      }                                        
      break;                                    
  }
}

/*********************************************************************
 ** cdCommand
 ** Description: Executes the change directory command
 ** Parameters: char* token
 *********************************************************************/
void cdCommand(char* token)
{
  int status;

  // If no argument is input, just change to home directory
  token = strtok(NULL, " ");
  if (token == NULL)
  {
    status = chdir(getenv("HOME"));
    if (status != 0)
    {
      printf("smallsh: unable to change directory\n");
      fflush(stdout);
    }
  }
  else
  // Change to the specified directory
  {
    status = chdir(token);
    if (status != 0)
    {
      printf("smallsh: unable to change directory\n");
      fflush(stdout);
    }
  }
}

/*********************************************************************
 ** exitCommand
 ** Description: Kills all running processes and exits the shell
 ** Parameters: struct DynArr* processList
 *********************************************************************/
void exitCommand(struct DynArr* processList)
{
  pid_t endPID;
  int status;
  struct DynArrIter* processIter;
  processIter = createDynArrIter(processList);

  initDynArrIter(processList, processIter);
  
  // Iterate through the jobs list and kill each process
  if (!isEmptyDynArr(processList))
  {
    while (hasNextDynArrIter(processIter))
    {
      TYPE bgrndPID = nextDynArrIter(processIter);
      kill(bgrndPID, SIGTERM);
      endPID = waitpid(bgrndPID, &status, 0);
    }
  }
}

/*********************************************************************
 ** statusCommand
 ** Description: Prints the exit status or terminating signal of the
 ** last foreground process
 ** Parameters: char* statusMsg 
 *********************************************************************/
void statusCommand(char* statusMsg)
{
  printf("%s", statusMsg);
  fflush(stdout);
}

/*********************************************************************
 ** getExitStatus 
 ** Description: Gets the exit value or termination signal of a 
 ** process and updates the current foreground status message
 ** Parameters: int status, char* statusMsg
 *********************************************************************/
void getExitStatus(int status, char* statusMsg)
{
  if (WIFEXITED(status))
    sprintf(statusMsg, "exit status %d\n", WEXITSTATUS(status));
  else if (WIFSIGNALED(status))
    sprintf(statusMsg, "terminated by signal %d\n", WTERMSIG(status));
  else
    sprintf(statusMsg, "unknown status\n");
}

/*********************************************************************
 ** redirectOutput
 ** Description: Redirects output to the file name passed to it or
 ** to dev/null if no file specified and a background process
 ** Parameters: char* fileName (if NULL, dev/null is used)
 *********************************************************************/
void redirectOutput(char* fileName)
{
  int fileDescriptor,
      fileDescriptor2;
  
  // Open file or dev/null for writing
  if (fileName == NULL)
    fileDescriptor = open("/dev/null", O_WRONLY);
  else
    fileDescriptor = open(fileName, O_WRONLY|O_CREAT|O_TRUNC, 0644);
  if (fileDescriptor == -1)
  {
    printf("smallsh: unable to open %s for output\n", fileName);
    fflush(stdout);
    exit(1);
  }

  // Redirect the output
  fileDescriptor2 = dup2(fileDescriptor, 1);
  if (fileDescriptor2 == -1)
  {
    printf("smallsh: dup2 failed\n");
    fflush(stdout);
    exit(2);
  }
}

/*********************************************************************
 ** redirectInput
 ** Description: Redirects input to the file name passed to it or
 ** to dev/null if no file specified and a background process
 ** Parameters: char* fileName (if NULL, dev/null is used)
 *********************************************************************/
void redirectInput(char* fileName)
{
  int fileDescriptor,
      fileDescriptor2;
  
  // Open file or dev/null for reading
  if (fileName == NULL)
    fileDescriptor = open("/dev/null", O_RDONLY);
  else
    fileDescriptor = open(fileName, O_RDONLY);
  if (fileDescriptor == -1)
  {
    printf("smallsh: unable to open %s for input\n", fileName);
    fflush(stdout);
    exit(1);
  }

  // Redirect the input
  fileDescriptor2 = dup2(fileDescriptor, 0);
  if (fileDescriptor2 == -1)
  {
    printf("smallsh: dup2 failed\n");
    fflush(stdout);
    exit(2);
  }
}

/*********************************************************************
 ** checkBackgroundJobs
 ** Description: Goes through the list of background jobs and checks
 ** if each one is done. If so, prints that the job is done and
 ** the job's exit status or termination signal
 ** Parameters: struct DynArr* processList, char* statusMsg
 *********************************************************************/
void checkBackgroundJobs(struct DynArr* processList, char* statusMsg)
{
  pid_t endPID;
  int status;
  char lastForegroundMsg[256];
  struct DynArrIter* processIter;
  processIter = createDynArrIter(processList);

  // Iterate through each background process
  initDynArrIter(processList, processIter);
  while (hasNextDynArrIter(processIter))
  {
    TYPE bgrndPID = nextDynArrIter(processIter);
    endPID = waitpid(bgrndPID, &status, WNOHANG);

    // Check if background process has finished
    if (endPID == bgrndPID)
    {
      printf("background pid %d is done: ", bgrndPID);
      fflush(stdout);

      // Print exit status and restore last foreground exit message
      strcpy(lastForegroundMsg, statusMsg);
      getExitStatus(status, statusMsg);
      printf("%s", statusMsg);
      fflush(stdout);
      strcpy(statusMsg, lastForegroundMsg);

      removeDynArrIter(processIter); // remove job from list
    }
  }
}

