/*
    Author: Polizois Siois 8535
*/
/*
    Faculty of Electrical and Computer Engineering AUTH
    Semester Project for Operating Systems (7th semester)
*/
/*
		Implementation of a linux shell.
		Supported modes:
		- Interactive mode with command prompt
		- Batch mode for script execution
*/
/*
    How to run each mode:
		- Interactive mode : Run the executable binary with no arguements
		- Batch mode       : Runt the executable binary followed by the name of the script you wish to run
*/
/*
		Tips:
		- You can run more than one commands in one line(B.mode)/prompt(I.mode). Just use the ";" and "&&" seperators.
		- Line length should not exceed MAXLINE defined below
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>


#define PROMPT "siois_8535" //	choose your prompt message
#define MAXBUFFER 650       //	Maximum number of characters the input buffer can take
#define MAXLINE 512         //	Maximun number of characters, so that an input line can be considered valid

/*
	In "paths[][]" you can define the paths in which the program will look
	for theexcecutable binaries in order to run your commands.
	If you want to run a command not included in these predefined paths,
	you will have to type it using its absolute path(and not just the name).
*/
const char paths[][MAXLINE] = {"/bin/", "/usr/bin/", "./"};
const int pathNum = sizeof(paths)/MAXLINE; //	No need to mess with this (calculates the number of paths)

void remSpaces(char *buffer);
void sepSetup(char *buffer);
void child(char** args);
void father(int pid, int *runNext);
void runCommand(char *command, int iter, int *changeNext);
void getArgs(char *command, char** args);

int main(int argc, char *argv[])
{
	char buffer[MAXBUFFER];
	char command[MAXBUFFER];
	int i, iter, runNext, batch=0;
	FILE *input, *script;
	int scriptEmpty=1;

	/*
		if argc==2 we are in batch mode (batch=1)  -  argv[0]='program name' / argv[1]='batchFile name'
		if argc==1 we are in interactive mode (batch=0)
		if argc>2 error
	*/
	if(argc==2) batch=1;
	else if(argc>2) { fprintf(stderr, "Wrong number of arguements\n"); return -1; }

	/*
		If in batch mode       : open batchFile for reading and set this file as input source (get commands from this file)
		If in interactive mode : set stdin as input source (get commands from stdin)
	*/
	if(batch)
	{
		script=fopen(argv[1], "r");
		if(!script) { fprintf(stderr, "Unable to open file!\n"); return -1; }
		input = script;
	}
	else input = stdin;

	//Infinite loop for reading commands from batchFile or printing prompt and reading commands from stdin
	for(;;)
	{
		/*
			When commands on the same line are seperated by ";" or "&&", runNext is used to determine if a command should be executed or not
			- by default runNext = 1 (true)
			-	if a command is followed by "&&" and fails, it sets runNext to 0 (false), so that the following command cannot be executed
			- if a command if followed by ";", it sets runNext to 1 (true) regardless its own excecution or not, so that the following command can be executed
		*/
		runNext =1;	i=0; iter=0;
		if(!batch)printf("%s>", PROMPT);            // print the prompt (if in interacive mode)
		if(!fgets(buffer, MAXBUFFER, input)) break; // Get the input (from stdin or batchFile) - if EOF, break the loop

		/*
			The extra whitespaces are calculated int the number of characters per line
			If the line lenght is greater than MAXLINE, print according message and continue to parse next line
			I have (-1) in the condition because the last character is the newLine character and i dont want to count it
		*/
		if(strlen(buffer)-1 <= MAXLINE)
		{
		 	remSpaces(buffer);                  // Removing all the extra whitespaces from the parsed line
			if(batch)
				if(strlen(buffer)) scriptEmpty=0; // If after removing the unnesessarry whitespaces the buffer is not empty, then the script is not empty (batch mode)
		 	sepSetup(buffer);                   // Removing the whitespaces before and after the seperators


			while(buffer[i]!='\0')                        // Reads the buffer char by char until it reaches the end of it
			{
				if(buffer[i]==';')                          // If ";" seperator is found, try to execute the command it follows
				{
					if(runNext) runCommand(command, iter, NULL);
					runNext=1; iter=0; i++;
				}
				else if(buffer[i]=='&' && buffer[i+1]=='&') // If "&&" seperator is found, try to execute the command it follows
				{
					if(runNext) runCommand(command, iter, &runNext);
					i+=2; iter=0;
				}
				else command[iter++]=buffer[i++];           // If the character is not seperator, it is a command character
			}
			if(runNext) runCommand(command, iter, NULL);     // if reached the end of the buffer, try to execute the last remaining command

		}else printf("Exceeded the limit of %d characters per line\n", MAXLINE);
	}

	if(batch)                                          // in batch mode
	{
		fclose(script);                                  // close the batchFile
		if(scriptEmpty) printf("Nothing to execute!\n"); // if batchFile was empty,  print according message
	}
	return 0;
}

/*
	Removes all the extra whitespaces from buffer and updates its content.
	- replaces double, triple, etc whitespaces with a single whitespace
	- removes whitespaces from the start of the line (the command name needs to be at the start)
*/
void remSpaces(char *buffer)
{
	int iter=0;
	int i=0;
	char temp[MAXLINE];

	while(buffer[i] != '\n')
	{
		if(buffer[i] == ' ')
		{
			if(iter) temp[iter++]=' '; //Ignore strarting spaces
			i++;
			while(buffer[i] == ' ') i++;
		}
		else temp[iter++] = buffer[i++];
	}
	temp[iter] = '\0';
	sprintf(buffer, "%s", temp);
}

//Removes whitespaces before and after the "&&" and ";" seperators and updates the buffer.
void sepSetup(char *buffer)
{
	int iter=0;
	int i=0;
	char temp[MAXLINE];

	while(buffer[i] != '\0')
	{
		if(buffer[i]==' ' && buffer[i+1]==';' && buffer[i+2]==' ') { temp[iter++]=';'; i+=3; }
		else if (buffer[i]==' ' && buffer[i+1]==';') { temp[iter++]=';'; i+=2; }
		else if (buffer[i]==';' && buffer[i+1]==' ') { temp[iter++]=';'; i+=2; }
		else temp[iter++] = buffer[i++];
	}
	temp[iter] = '\0';
	sprintf(buffer, "%s", temp);

	iter=0;
	i=0;
	while(buffer[i] != '\0')
	{
		if(buffer[i]==' ' && buffer[i+1]=='&' && buffer[i+2]=='&' && buffer[i+3]==' ') { temp[iter++]='&'; temp[iter++]='&'; i+=4; }
		else if (buffer[i]==' ' && buffer[i+1]=='&' && buffer[i+2]=='&') { temp[iter++]='&'; temp[iter++]='&'; i+=3; }
		else if (buffer[i]=='&' && buffer[i+1]=='&' && buffer[i+2]==' ') { temp[iter++]='&'; temp[iter++]='&'; i+=3; }
		else temp[iter++] = buffer[i++];
	}
	temp[iter] = '\0';
	sprintf(buffer, "%s", temp);
}

/*
	Meant to run on a child process
	Runs the command stored in args[0] with the arguements stored in the following args positions(args[following 0]).
	If the command has its own path icluded, it is called from this path
	If the command has no path included(only the name), it is called from one of the paths defined at the start of this code (paths[][])
	Prints the result of the command (output or error message)
	Exits the process
*/
void child(char** args) //child
{
	int j=0, ownPath=0;
	char *cmd = args[0];
	char comWithPath[512];
	int error;

	while(args[0][j]) if(args[0][j++] == '/') {ownPath=1; break;}
	j=0;

	if(ownPath) error = execvp(args[0], args);
	else
	while(j<pathNum)
	{
		args[0] = cmd;
		sprintf(comWithPath, "%s%s", paths[j++], args[0]);
		args[0] = comWithPath;
		error = execvp(args[0], args);
		if(!error) break;
	}

	if(error) perror(""); // If the command failed, print its error message
	exit(error);

}
/*
	Takes a string that contains a command with is arguements seperated with a whitespace
	Breaks the string to single seperated strings and stores them in args (args[0]-command name, args[1]-1st arg, args[2]-2nd arg, etc)
*/
void getArgs(char *command, char** args)
{
	int count = 0;
	int j=0;
	char *cmd;
	char *token;

	token = strtok(command, " ");                       // get the first token
	cmd = token;
	args[0] = cmd;
	j=1;
	while( token != NULL )
		{ token = strtok(NULL, " "); args[j++] = token; } // walk through other tokens
}

/*
	Meant to run on the father process
	Waits for a child process to return
	If the child process was not successful, it sets the value of runNext to 0
*/
void father(int pid, int *runNext)
{
	int status;

	wait(&status);
	if(status) {if(runNext!=NULL) *runNext=0;/*printf("pid %d: Command failure! : %d\n", pid, status);*/}
	//else printf("pid %d success!\n", pid);
}

/*
	Gets an unfinalized string that contains a command with its arguements seperated by a single whitespace.
	It finalises the string and seperates it to arguements.
	If the command is "quit" or "cd" it executes it.
	In any other case it forks a child process to execute the command and waits for it to finish.
*/
/*
	- command contains the string mentioned above
	- end indicates the end of the string
	- changeNext can take the address of runNext, or NULL
		- if the command is followed by ";" or '\0', put NULL in changeNext so runNext is not affected
		- if the command is followed by "&&", put the address of runNext in changeNext so runNext turns 0 in case of command failure
*/
void runCommand(char *command, int end, int *changeNext)
{
	int pid, count = 0, j = 0;
	char **args;

	command[end]='\0';
	while(command[j]!='\0') if(command[j++]==' ') count++;
	if(!command[0]) return;
	args = (char **)malloc((count+2)*sizeof(char*));

	getArgs(command, args);

	if(!strcmp(args[0], "quit\0")) exit(0);           // exit program
	else if(!strcmp(args[0], "cd\0")) chdir(args[1]); // change working directory of process
	else
	{
		pid = fork();
		if(pid == 0) child(args);                 // child
		else if(pid > 0) father(pid, changeNext); // father
		else {perror("fork error"); exit(-1);}    // error
	}

}
