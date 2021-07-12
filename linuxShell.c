#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<readline/readline.h> 
#include<readline/history.h> 

#define MAXCOM 1000 // max number of letters to be supported 
#define MAXLIST 100 // max number of commands to be supported 

// Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J") // \033[H to move cursor to topleft corner , \033[J to clear from cursor to end of screen

// Signal SIGINT handling
void sigintHandler(int sig_num)
{ 
    fflush(stdout); // flushes output buffer
}

// Greeting shell during startup 
void startShell() 
{ 
	clear();
	printf("\n---------------------------------------------------------------------------");
	printf("\n\n\t\t\t\tWelcome to Our Shell\n\n"); 
	printf("---------------------------------------------------------------------------");
	char* username = getenv("USER"); 
	printf("\n\n\nUSER is: @%s", username); 
	printf("\n"); 
	sleep(2);
	clear(); 
} 

// Function to take input 
int handleInput(char* str) 
{ 
	char* temp; 

	temp = readline("\n$ "); 
	if (strlen(temp) != 0) { 
		add_history(temp); 
		strcpy(str, temp); 
		return 0; 
	} else { 
		return 1; 
	} 
} 

// Function to print Current Directory. 
void printDir() 
{ 
	char* username = getenv("USER"); 
	char cwd[1024]; 
	getcwd(cwd, sizeof(cwd)); 
	printf("\n[%s@shell]-[%s]",username, cwd); 
} 

// Help command builtin 
void openHelp() 
{ 
	puts(
		"\n------------------------------------------------------------"
		"\n\tWELCOME TO SHELL HELP"
		"\n------------------------------------------------------------"
		"\nShell capabilities:"
		"\n>cd command"
		"\n>ls command"
		"\n>exit command"
		"\n>all other general commands available in UNIX shell"
		"\n>pipe handling 'one pipe'"
		"\n>improper space handling"
		"\n>Signal handling ctrl-c"); 

	return; 
} 


// Function where the system command is executed 
void execSysCommand(char** parsed) 
{ 
	// Forking a child 
	pid_t pid = fork(); 

	if (pid < 0) { 
		printf("\nFailed forking child.."); 
		return; 
	} else if (pid == 0) { 
		if (execvp(parsed[0], parsed) < 0) { 
			printf("\nCould not execute command.."); 
		} 
		exit(0); 
	} else { 
		// waiting for child to terminate 
		wait(NULL); 
		return; 
	} 
} 

// Function where the piped system commands is executed 
void execSysCommandPiped(char** parsed, char** parsedpipe) 
{ 
	// 0 is read end, 1 is write end 
	int pipefd[2]; 
	pid_t p1, p2; 

	if (pipe(pipefd) < 0) { 
		printf("\nPipe could not be initialized"); 
		return; 
	} 
	p1 = fork(); 
	if (p1 < 0) { 
		printf("\nCould not fork"); 
		return; 
	} 

	if (p1 == 0) { 
		// Child 1 executing.. 
		// It only needs to write at the write end 
		close(pipefd[0]); 
		dup2(pipefd[1], 1); 

		if (execvp(parsed[0], parsed) < 0) { 
			printf("\nCould not execute command 1.."); 
			exit(0); 
		} 
	} else { 
		// Parent executing 
		p2 = fork(); 

		if (p2 < 0) { 
			printf("\nCould not fork"); 
			return; 
		} 

		// Child 2 executing.. 
		// It only needs to read at the read end 
		if (p2 == 0) { 
			close(pipefd[1]); 
			dup2(pipefd[0], 0); 
			if (execvp(parsedpipe[0], parsedpipe) < 0) { 
				printf("\nCould not execute command 2.."); 
				exit(0); 
			} 
		} else { 
			// parent executing, waiting for two children 
			close(pipefd[1]); 
			close(pipefd[0]); 
			wait(NULL);  
			wait(NULL);
		} 
	} 
} 


// Function to execute builtin commands 
int execOwnCommand(char** parsed) 
{ 
	int NoOfOwnCmds = 4, i, switchOwnArg = 0; 
	char* ListOfOwnCmds[NoOfOwnCmds]; 
	char* username; 

	ListOfOwnCmds[0] = "exit"; 
	ListOfOwnCmds[1] = "cd"; 
	ListOfOwnCmds[2] = "help"; 
	ListOfOwnCmds[3] = "hello"; 

	for (i = 0; i < NoOfOwnCmds; i++) { 
		if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) { 
			switchOwnArg = i + 1 ; 
			break; 
		} 
	} 

	switch (switchOwnArg) { 
	case 1: 
		printf("Goodbye\n"); 
		exit(0); 
	case 2: 
		chdir(parsed[1]); 
		return 1; 
	case 3: 
		openHelp(); 
		return 1; 
	case 4: 
		username = getenv("USER"); 
		printf("\nHello %s.\nThis is our simple shell"
			"\nUse help to know more..\n", 
			username); 
		return 1; 
	default: 
		break; 
	} 

	return 0; 
} 

// function for finding pipe 
int parsePipe(char* str, char** strpiped) 
{ 
	int i; 
	for (i = 0; i < 2; i++) { 
		strpiped[i] = strsep(&str, "|"); 
		if (strpiped[i] == NULL) 
			break; 
	} 

	if (strpiped[1] == NULL) 
		return 0; // returns zero if no pipe is found. 
	else { 
		return 1; 
	} 
} 

// function for parsing command words 
void parseSpace(char* str, char** parsed) 
{ 
	int i; 

	for (i = 0; i < MAXLIST; i++) { 
		parsed[i] = strsep(&str, " "); 

		if (parsed[i] == NULL) 
			break; 
		if (strlen(parsed[i]) == 0) 
			i--; 
	} 
} 

int processString(char* str, char** parsed, char** parsedpipe) 
{ 

	char* strpiped[2]; 
	int piped = 0; 

	piped = parsePipe(str, strpiped); 

	if (piped) { 
		parseSpace(strpiped[0], parsed); 
		parseSpace(strpiped[1], parsedpipe); 

	} else { 
		parseSpace(str, parsed); 
	} 

	if (execOwnCommand(parsed)) 
		return 0; 
	else
		return 1 + piped; 
} 

int main() 
{ 
	char inputString[MAXCOM], *parsedArgs[MAXLIST]; 
	char* parsedArgsPiped[MAXLIST]; 
	int execFlag = 0; 
	startShell(); 
	signal(SIGINT, sigintHandler);

	while (1) { 
		// print shell line 
		printDir(); 
		// take input 
		if (handleInput(inputString)) 
			continue; 
		// process 
		execFlag = processString(inputString, 
		parsedArgs, parsedArgsPiped); 

		// 0 if there is no command 
		// or it is a built-in command, 
		// 1 if it is a system command 
		// 2 if it is including a pipe. 

		// execute 
		if (execFlag == 1) 
			execSysCommand(parsedArgs); 

		if (execFlag == 2) 
			execSysCommandPiped(parsedArgs, parsedArgsPiped); 
	} 
	return 0; 
} 