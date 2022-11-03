#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
using namespace std;

int getCommandLength(char *args[]){
    int len = 0;
    while(args[len] != NULL)
	{
        len++;
    }
    return len;
}

char *deleteArgs(char *args[])
{
    int len = getCommandLength(args);
   	for(int i = 0; i < len; i++)
        args[i] = NULL;

    return *args;
}

//Tokenize
void tokenizeSpace(char str[], char** args, int &size)
{
	char *token = strtok(str, " ");
	args[0] = token;
	
	int i = 1;
	while (token != NULL)
	{
		token = strtok(NULL, " ");
		args[i] = token;
		i++;
	}
	size = i - 1;
}

void tokenizePipe(char str[], char** args, int &numPipe)
{
	char *token = strtok(str, "|");
	args[0] = token;
	
	int i = 1;
	while (token != NULL)
	{
		token = strtok(NULL, "|");
		args[i] = token;
		i++;
	}
	numPipe = i - 2;
}

void executeCommand(char s[])
{
	char *temp = new char[strlen(s) + 1];
	strcpy(temp, s);
	char* args[100];
	int size = 0;
	tokenizeSpace(temp, args, size);

	pid_t retVal;
	retVal=fork();
	int status;
	
	if(retVal<0)
	{
	//fork Failed
	
	}
	else if(retVal==0)
	{
	//Child Process
		if (execvp(args[0], args) < 0) 
		{
			cout<<"Error: Cannot execute the "<<args[0]<<" command!";
			return;
		}
	}
	else
	{
	//Parent Process
		wait(&status);
	}
}

void executePipeCommand(char* args[], int inputfd, int outputfd, char output[])
{
	pid_t retVal;
	retVal = fork();
	if(retVal < 0)
	{
	//fork Failed
		cout<<"Error: Could Not Fork";
		return;
	}
	else if(retVal == 0)
	{
	//Child Process
		if (inputfd != 0)
		{
            close(0);
			dup(inputfd);
            close(inputfd);
        }
        if (outputfd != 1)
		{
            close(1);
            dup(outputfd);
            close(outputfd);
        }
		
		if (execvp(args[0], args) < 0) 
		{
			cout<<"Error: Cannot execute the "<<args[0]<<" command!";
			return;
		}
	}
	else	//Parent
		wait(NULL);	
}

bool checkForPipe(char s[])
{
	for(int i = 0; i < strlen(s); i++)
	{
		if(s[i] == '|')
			return true;
	}
	return false;
}

bool PipeCommand(char s[])
{
	if(s[0] == '|')
	{
		cout<<"syntax error near unexpected token '|'";
		return true;
	}

	else if(s[strlen(s) - 1] == '|')
	{
		cout<<"syntax error near unexpected token '|'";
		return true;
	}

	else if(checkForPipe(s))
	{
		
		char *temp = new char[strlen(s) + 1];
		strcpy(temp, s);
		char* args[100];
		int numPipes = 0;
		tokenizePipe(temp, args, numPipes);
		char *lastcmd[1024];
		int lastcmdsize = 0;
		char * s1 = new char[strlen(args[numPipes]) + 1];
		strcpy(s1, args[numPipes]);
		tokenizeSpace(s1, lastcmd, lastcmdsize);
		
		int pipefd[2];
		int pipeInput;
		int i = 0;
		int n = 0;
		char* pipedArgs[100];
		while(i < numPipes)
		{
			n = 0;
			char* cmd = new char[strlen(args[i]) + 1];
			strcpy(cmd, args[i]);
			tokenizeSpace(cmd, pipedArgs, n);

			pipe(pipefd);
			executePipeCommand(pipedArgs, pipeInput, pipefd[1], temp);
			close(pipefd[1]);
			pipeInput = pipefd[0];
			deleteArgs(pipedArgs);
			i++;
		}
		if(fork() == 0)
		{
		if(pipeInput != 0)
		{
			close(0);
			dup(pipeInput);
   		}
			if(execvp(lastcmd[0], lastcmd)<0)
			{
				cout<<"Error: Could Not Execute "<<lastcmd[0]<<" command";
				return true;
			}
		}
		else
			wait(NULL);
		return true;
	}

	return false;
}

bool BuiltinCommand(char s[])
{
	char* args[100];
	int size = 0;
	char *temp = new char[strlen(s) + 1];
	strcpy(temp, s);
	tokenizeSpace(temp, args, size);

	if (strcmp(args[0], "exit") == 0)
	{
		cout<<"Exiting....";
		exit(0);
	}

	else if (strcmp(args[0], "cd") == 0){
		chdir(args[1]);
		return true;
	}

	return false;
	/////////////History Handling -- TBD//////////
}

int main()
{
	char s[1024];
	while(1)
	{
		cout<<"\nusmanshell@root$ ";
		cin.getline(s, 1024);
		if(BuiltinCommand(s))
			continue;
		
		if(PipeCommand(s))
			continue;

		else
			executeCommand(s);
	}
}
