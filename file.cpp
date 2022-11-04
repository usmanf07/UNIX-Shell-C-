#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>          
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>
using namespace std;

//Global Redirection Flags
int redInputFlag;
int redOutputFlag;
int redInputLoc;
int redOutputLoc;
vector<string> history;
	
int getCommandLength(char *args[])	//Function for getting number of commands
{
    	int len = 0;
	while(args[len] != NULL)
	{
		len++;
	}
    	return len;
}

char *deleteArgs(char *args[])	//Function for cleaning the arguments array
{
    int len = getCommandLength(args);
   	for(int i = 0; i < len; i++)
        args[i] = NULL;

    return *args;
}

//Tokenize the commands on space
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
//Function for getting the redirection command without < > signs
char *redirectArgs(char *args[], char *redArgs[])
{
    int i = 0, j = 0;
    while(args[i] != NULL)
    {
        if(strcmp(args[i],"<") != 0 && strcmp(args[i],">") != 0)
            redArgs[j] = args[i];
        
        else
            j--;
        
        i++;
        j++;
    }
    
    return *redArgs;
}
//Tokenize the commands on pipes and count number of pipes
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
//Function for assigning redirection location and input or output to global variables
char *checkForRedirection(char *args[], char *redArgs[])
{
    int i = 0;
    while(args[i] != NULL)
	{
        if(strcmp(args[i],"<") == 0 || strcmp(args[i],">") == 0 || strcmp(args[i],"|") == 0)
		{
            if(strcmp(args[i],"<") == 0){
                redInputFlag = 1;	//Check the input flag
                redInputLoc = i;	//Save the location index
            }
            else{
                redInputFlag = 0;
            }
			//Do same for output flag
            if(strcmp(args[i],">") == 0){
                redOutputFlag = 1;
                redOutputLoc = i;
            }
            else{
                redOutputFlag = 0;
            }
            
            *redArgs = deleteArgs(redArgs);	//Clear and copy new args
            *redArgs = redirectArgs(args, redArgs);
        }
        i++;
    }
    return *redArgs;
}

//Main Function for exectuing commands without pipes
void executeCommand(char* args[])
{
	int fd;
	int fileError = 0;
	pid_t retVal;
	retVal=fork();
	int status;
	//Check if input flag or output flag is true, then get file descriptor from index
	if(redInputFlag==1)
	{
		fd = open(args[redInputLoc], O_RDONLY,0);
	}
	if(redOutputFlag==1)
	{
		fd = open(args[redOutputLoc], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	}
	if(fd<0)
	{
		fileError = 1;
	}
	
	if(retVal<0)
	{
	//fork Failed
	
	}
	else if(retVal==0)
	{
	//Child Process
		if(redInputFlag == 1){
			if(fd < 0){
				// file descriptor error
				cout<<"Error: Cannot open input file!";
				return;
			}
			else{
			// Redirect using dup2
			dup2(fd, 0);
			close(fd);
			args[redInputLoc]=NULL;
			}
		}
		else if(redOutputFlag == 1){
			if(fd < 0){

				cout<<"Error: Cannot open output file!";
				return;
			}
			else{
				// Redirect output using dup2 to file descriptor
				dup2(fd, 1);
				close(fd);
				args[redOutputLoc]=NULL;
			}
		}
		//Execute command
		if(execvp(args[0], args) < 0)
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

//Function for exectuing only one pipe command
void executePipeCommand(char* args[], int inputfd, int outputfd, char output[])
{
	char *redArgs[1024];
    	*redArgs = checkForRedirection(args, redArgs); //Check If there is redirection

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
		if (inputfd != 0)	//Change file descriptors to pipe
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
		
		 if(redInputFlag == 1 || redOutputFlag == 1)	//if its standard then execute redirection args, else normal
		 {
            		executeCommand(redArgs);
        	}
        	
		else
		{
			if(execvp(args[0], args) < 0)
			{
				cout<<"Error: Cannot execute the "<<args[0]<<" command!";
				return;
			}
		}
	}
	else	//Parent
		wait(NULL);	
}

//Function for checking if the command contains pipes, will return true if found any
bool checkForPipe(char s[])
{
	for(int i = 0; i < strlen(s); i++)
	{
		if(s[i] == '|')
			return true;
	}
	return false;
}

//Function for executing multiple pipe commands
bool PipeCommand(char s[])
{
	//Simple Pipe syntax errors
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
		tokenizePipe(temp, args, numPipes);	//Tokenize the command with pipes

		//Seperate the last command as its function is to read only not write
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
		while(i < numPipes)	//Execute all pipe commands one by one using the single pipe functions
		{
			n = 0;
			char* cmd = new char[strlen(args[i]) + 1];
			strcpy(cmd, args[i]);
			tokenizeSpace(cmd, pipedArgs, n);	//tokenize the one command on space

			pipe(pipefd);
			executePipeCommand(pipedArgs, pipeInput, pipefd[1], temp);
			close(pipefd[1]);
			pipeInput = pipefd[0];
			deleteArgs(pipedArgs);	//clear original array
			i++;
		}
		//Execution for last command using one child
		if(fork() == 0)
		{
			//child
			if(pipeInput != 0)
			{
				close(0);
				dup(pipeInput);
			}
			execvp(lastcmd[0], lastcmd);
		}
		else	//parent
			wait(NULL);
		return true;
	}

	return false;
}

//Function for executing simple terminal commands
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
	if(strcmp("pwd",args[0]) == 0)
	{
		char cwd[1024];
		cout<<getcwd(cwd, sizeof(cwd));
		return true;
    	}
    	if(strcmp("history",args[0]) == 0)
	{
		history.pop_back();
		if(history.size() > 0)
		{
			int size = 10;
			if(history.size() < 10)
				size = history.size();
			
			for (int i = 0; i < size; i++) 
			{
		    		cout << i + 1 <<": "<<history[i] << endl;
			}
        	}
        	else
        	{
        		cout<<"No history to show!"<<endl;
        	}
		return true;
    	}
    	
	if(strcmp("!!",args[0]) == 0)
	{
		history.pop_back();
		if(history.size() > 0)
		{
			cout<<"Executing last command: "<<history[history.size()-1]<<endl;
			string temp = history[history.size()-1];
			strcpy(s, temp.c_str());
			return false;
		}
		else
        	{
        		cout<<"There are no commands in history!"<<endl;
        		return true;
        	}
		
    	}
    	
    	if(s[0] == '!')
    	{
    		int size = strlen(s);
    		int index = 0;
    		bool flag = false;
    		for(int i = 1; i < size; i++)
    		{
    			if(isdigit(s[i]))
    			{
    				index = 10*index + (s[i] - '0');
    				flag = true;
    			}
    		}
    		
    		history.pop_back();
			
		if(history.size() > 0)
		{	
			if(flag && index < history.size())
			{
				if(index != 0)
					index = history.size() - index;
				else
					index = 0;
				cout<<"Executing command: "<<history[index]<<endl;
				string temp = history[index];
				strcpy(s, temp.c_str());
				return false;
			}
			else if(index > history.size())
			{
				cout<<"There is no command on this index in history!"<<endl;
				return true;
			}
			else
			{
				cout<<"Invalid history index access! Use it like this: !2 where 2 is the index number"<<endl;
				return true;
			}
		}
		
		else
        	{
        		cout<<"There are no commands in history!"<<endl;
        		return true;
        	}
    		return true;
    	}

	return false;
}

int main()
{
	char s[1024];
	while(1)
	{
		cout<<"\nusmanshell@root$ ";
		cin.getline(s, 1024);
		history.push_back(s);
		
		if(BuiltinCommand(s))	//1. Check if command is Simple else move next
			continue;
		
		if(PipeCommand(s))	//1. Check if command has pipes else move next
			continue;

		else
		{
			char *temp = new char[strlen(s) + 1];
			strcpy(temp, s);
			char* args[100];
			int size = 0;
			tokenizeSpace(temp, args, size);
			char *redArgs[1024];
		    *redArgs = checkForRedirection(args, redArgs);		//3. Check if command has redirection signs else move next
				if(redInputFlag == 1 || redOutputFlag == 1)
				{
				
		            	executeCommand(redArgs);
		        }
		        else
		        {
		            executeCommand(args);		//5. Execute simple commands without pipes or redirection
		        }
                }
			
	}
}
