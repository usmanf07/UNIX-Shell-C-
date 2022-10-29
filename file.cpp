#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
using namespace std;

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
	numPipe = i - 1;
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
		execvp(args[0], args);
	}
	else
	{
	//Parent Process
		wait(&status);
	}
}

void executePipeCommand(char** piped1, char** piped2)
{
	int pipefd[2];
	if (pipe(pipefd) < 0) 
	{
		cout<<"Error: Could Not Pipe";
		return;
	}
	pid_t retVal1, retVal2;
	retVal1 = fork();
	if(retVal1<0)
	{
	//fork Failed
		cout<<"Error: Could Not Fork";
		return;
	}
	else if(retVal1==0)
	{
	//Child Process 1
		close(pipefd[0]);
		dup2(pipefd[1], 1);
		if (execvp(piped1[0], piped1) < 0) 
		{
			cout<<"Error: Cannot execute the "<<piped1[0]<<" command!";
			return;
		}
		close(pipefd[1]);
		exit(0);
	}
	else
	{
	//Parent Process
		close(pipefd[1]);
		wait(NULL);
		//read
		retVal2 = fork();
		if (retVal2 == 0) 
		{
			close(0);
			close(pipefd[1]);
			dup2(pipefd[0], 0);
			if (execvp(piped2[0], piped2) < 0) 
			{
				cout<<"Error: Cannot execute the "<<piped2[0]<<" command!";
				return;
			}
			close(pipefd[0]);
			exit(0);
		} 
		else 
		{
			wait(NULL);
		}
	}
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
		int i = 0;
		int n1 = 0;
		int n2 = 0;
		while(i < numPipes)
		{
			char * piped1[100];
			char * piped2[100];
			n1 = 0;
			n2 = 0;
			char* cmd1 = new char[strlen(args[i]) + 1];
			strcpy(cmd1, args[i]);
			i++;
			char* cmd2 = new char[strlen(args[i]) + 1];
			strcpy(cmd2, args[i]);
			tokenizeSpace(cmd1, piped1, n1);
			tokenizeSpace(cmd2, piped2, n2);
			executePipeCommand(piped1, piped2);
			i++;
		}
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

	else if (strcmp(args[0], "hello") == 0)
	{
		string user = getenv("USER");
		cout<<"Hello "<<user<<", How Are You?";
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
