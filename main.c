#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <limits.h>
#include <assert.h>
#include <signal.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "bash2.h"
#include "myHistory.h"
#include <err.h>
#define ANSI_COLOR_CYAN    "\x1b[36m" //color cyan
#define ANSI_COLOR_MAGENTA "\x1B[35m" //color magenta
#define ANSI_COLOR_RESET   "\x1b[0m" //defualt color

// Function declarations
void pipeItUp(char *pipeStr);
void BatchMode(int argc, char* argv[]); //BatchMode function

int main(int argc, char* argv[]) {

	//call BatchMode() function
	if (argc > 1) { //check is argument is greater than one
		BatchMode(argc, argv); //call BatchMode() function
		return 0; //exit
	} //if

	int shouldExit = 0; // Change to true when user enters exits
	char userInput[1000]; // User input
	char cwd[FILENAME_MAX]; // Prompt that user sees on each command
	char *commands[1000]; // Holds up to 1000 commands
	int numCommands = 0; // Counter for number of commands
	char *arguments[1000]; // Holds up to 1000 arguments for commands
	char *redirect_filename; // Point to spot in commands[i] where redirect
	int  redirect_type;   //-1 = none, 0=input, 1=ouptut, 2 = append
	int  rd_fd;
	char newPath[1024];
	char pathToken[FILENAME_MAX];
	char history[HISTORYCHUNK][CMDLENGTH]; //history array
	int hisCount = 0; //history counter
	int i = 0; //counter
	regex_t regex; //regex
	regex_t pipe_regex;
	regex_t bash2_regex;
	char* re_pattern = "^(\\s*;*)+$"; //regex pattern for user input
	char* pipe_pattern = "^(([^\\|]+)[\\|]{1}([^\\|]+)([\\|]{1}[^\\|]+){0,1})$";
	char* bash2_pattern = "^bash2\\s+[^ ]+$";
	int pipe_re = regcomp(&pipe_regex, pipe_pattern, REG_EXTENDED);
	int re_ret = regcomp(&regex, re_pattern, REG_EXTENDED);
	int b2_ret = regcomp(&bash2_regex, bash2_pattern, REG_EXTENDED);
	int status;	//variable parameter for waitpid() system call
	char *path;  // pointer to path environment variable
	pid_t pid;	//process id declaration

	// Loop until user types exit
	while(!shouldExit) {
	    //----------blocking signals----------------
	    //set background shell
		if(tcsetpgrp(fileno(stdin), getpgrp())<0){
			perror("error with tcsetpgrp background grouping"); //report error with background grouping
		} //if

        //set mask to block signals in background
		sigset_t mask;
		//mask the signals in the background
		sigfillset(&mask);
		//adminster the mask for all signals in the background
		if(sigprocmask(SIG_SETMASK, &mask, NULL)<0){
			perror("SIG_SETMASK blocking signal error\n"); //report error with sigprocmask
		} //if

        //----------end blocking signals----------------

		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			//print prompt (current working directory with cyan color
			printf(ANSI_COLOR_CYAN "%s$ "  ANSI_COLOR_MAGENTA, cwd);
		} else {
			perror("getcwd");
			exit(1);
		}

		if(fgets(userInput, sizeof(userInput), stdin)) {	// Scan for user input
			// Get rid of the \n from the user input
			re_ret = regexec (&regex, userInput, 0, NULL, 0);  //TODO ::comment needed
			if (re_ret == 0) { //TODO ::comment needed
				continue; //TODO ::comment needed
			}

			if(userInput[strlen(userInput) - 1] == '\n') { //TODO ::comment needed
				userInput[strlen(userInput) - 1] = '\0'; //TODO ::comment needed
			}
			if (hisCount >= 20) { //TODO ::comment needed
				insertCMD(history, userInput); //TODO ::comment needed
				hisCount++; //TODO ::comment needed
			} else {
			    hisCount++;
				strcpy(history[hisCount - 1], userInput); //TODO ::comment needed
			}

			char *token = strtok(userInput, ";");	// Tokenize user input by ;
			// While not reached end of token
			while (token != NULL) {
				/*if  (regexec(&regex,commands[i], 0, NULL, 0) == 0) {
                                      continue;
                                }*/
				//char *temp;
				//temp=token;
				commands[numCommands] = token; // Store the token as a command
				token = strtok(NULL, ";"); // Tokenize end by ;
				numCommands++; // + 1 command separated 
			}

			// Loop until number of commands. Get arguments of commands
			while (i < numCommands) {
				if  (regexec(&regex,commands[i], 0, NULL, 0) == 0) {
					i++;
					//printf("command not found\n");
					continue;
				}
				else if (regexec(&pipe_regex, commands[i], 0, NULL, 0) == 0) {
				    pipeItUp(commands[i]); //piping regex call function here
				    i++;
				    continue;
				}
				else if (regexec(&bash2_regex, commands[i], 0, NULL, 0) == 0) {
				    bash2(commands[i]);
				    i++;
				    continue;
				}
                
                // Scan commands[i] for a redirect character
				// IF we find, replace with \0 to make commands[i]
				//    only contain the command and arguments 
                //  Store location of redirect_filename (first character
                //  after the > < so that we can do the redirect)
				redirect_filename = commands[i];
				redirect_type = -1;
				while(*redirect_filename != '\0')
				{
				    if(*redirect_filename == '>')
				    {
				        redirect_type = 1;
				        *redirect_filename = '\0';
				        redirect_filename++;
				        break;
				    }
				    else if(*redirect_filename == '<')
				    {
				        redirect_type = 0;
				        *redirect_filename = '\0';
				        redirect_filename++;
				        break;
				    }
			        redirect_filename++;
				}
				
				// Get rid of spaces in redirect filename
				if(redirect_type >=0)
				{
				    while(*redirect_filename == ' ' || *redirect_filename =='\t')
				    {
				        redirect_filename++;
				    }
				}
                
				arguments[0] = strtok(commands[i], " \n");	// Tokenize by whitespace
				int t = 0;
				while (arguments[t] != NULL) {
					t++;
					arguments[t] = strtok(NULL, " \n");	// Tokenize by whitespace
				}

				if(strcmp(arguments[0], "exit") == 0 && arguments[1] == NULL) { // If user inputs "exit" command
					shouldExit = 1;	// Change conditional while loop variable to 1, so we can exit the loop
				} else if (strcmp(arguments[0], "cd") == 0) { // If user inputs "cd" command
					if(!arguments[1]) {	// If cd argument is empty
						if(chdir(getenv("HOME")) == -1) {
							perror("cd");
						}
					} else {
						if(chdir(arguments[1]) == -1) {
							perror("cd");
						}
					}
				} else if (strcmp(arguments[0], "myhistory") == 0 && arguments[1] == NULL) {
					displayHistory(history, hisCount);
				} else if(strcmp(arguments[0], "path") == 0) {
				     if(arguments[1] == NULL) {
				         path = getenv("PATH");
				         printf("%s\n", path);
				     }
				     else if(arguments[1][0] == '+') {
				         path = getenv("PATH");   // Get existing path
				         strcpy(newPath,path);  // Copy the old path into newPath variable
				         strcat(newPath,":");   // Add : delimiter
				         strcat(newPath,arguments[2]);  // Add our new path piece
				         if(setenv("PATH",newPath,1)<0) {        // Set PATH to our new path
				             printf("Failed to update path\n");
				         }
				     }
				     else if(arguments[1][0] == '-') {
				         path = getenv("PATH");   // Get existing path
				         char *t = pathToken;
				         *newPath = '\0';
				         while(*path != '\0')
				         {
				             if(*path != ':')
				             {
				                 *t = *path;
				                 t++;
				             }
				             else
				             {
				                 *t = '\0';
				                 t = pathToken;
				                 if(strcmp(pathToken,arguments[2])!=0)
				                 {
				                     if(strlen(newPath)!=0)
				                     {
				                         strcat(newPath,":");
				                     }
				                     strcat(newPath,pathToken);
				                 }
				             }
				             path++;
				         }
				         // Make sure we re-add the last path 
				         *t = '\0';
				         if(strcmp(pathToken,arguments[2])!=0)
		                 {
		                     if(strlen(newPath)!=0)
		                     {
		                         strcat(newPath,":");
		                     }
		                     strcat(newPath,pathToken);
		                 }
		                 if(setenv("PATH",newPath,1)<0) {        // Set PATH to our new path
				             printf("Failed to update path\n");
				         }
				     }
				     else
				     {
				         printf("Invalid arguments supplied to path command\n");
				     }
				     
				 }
				 else {
						if(redirect_type == 0)
						{
						    rd_fd = open(redirect_filename,O_RDONLY);
						    if(rd_fd < 0)
						    {
						        printf("Could not use: %s for std input\n",redirect_filename);
						        redirect_type = -1;
						    }
						}
						else if(redirect_type == 1)
						{
						    rd_fd = open(redirect_filename,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
						}	
						
						// Proceed to the fork(), exec() wait() model for all other commands
					    //fork
						pid=fork();
						if (pid == 0) {	// CHILD
							//----------signals for child----------------
							//set the group for foreground subprocesses
							if(setpgrp()<0){
								perror("error setting process group setpgrp()"); //error setting process group
							} //if
							//set the foreground subprocesses
							if(tcsetpgrp(fileno(stdin), getpgid(pid))<0){
								perror("error with tcsetpgrp grouping child"); //error with grouping child
							} //if
							//unmask signals for forground subprocesses
							sigemptyset(&mask);
							//unmask all signals in the forground subprocesses
							if(sigprocmask(SIG_SETMASK, &mask, NULL)<0){
								perror("SIG_SETMASK unblocking signal error\n"); //report error with sigpromask
							} //if
							
							  if(redirect_type == 0)
                            {
                                dup2(rd_fd,0);
                                close(rd_fd);
                            }
                            else if(redirect_type == 1)
                            {
                                dup2(rd_fd,1);
                                close(rd_fd);                                
                            }
							
							//----------end signals for child----------------
							if ((execvp(arguments[0], arguments)) < 0 || arguments[0] == "exit") {
								if (strcmp(commands[0], "bash2") == 0){
									printf("bash2 has too many or not enough arguments\n");
								}
								else {
									printf("The command %s does not exist or cannot be executed\n",commands[i]);
								}
							} //if

							exit(0);
						} else if (pid > 0) { // PARENT
							//----------signals for parent----------------
							//set parent id
							if(setpgid(pid, pid)<0){
								perror("error with setpgid() grouping parent process"); //report error for setpgid
							} //if
							//ignore when process in background tries to write to the terminal or set its modes
							if(signal(SIGTTOU, SIG_IGN)==SIG_ERR){
								perror("error has occured when ignoring sginal SIGTTOU\n"); //report error for SIGTTOU
							} //if
							//set background
							if(tcsetpgrp(fileno(stdin), getpgid(pid))<0){
								perror("error with tcsetpgrp grouping parent\n"); //report error when grouping parent
							} //if
							//wait for child to finish based on specific pid
							if(waitpid(-1, &status, WUNTRACED | WCONTINUED)<0){
								perror("error with waitpid()\n"); //report error with waitpid
							} //if
							//----------end signals for parent---------------
						} else { // FORK ERROR
							perror("fork error \n");
							exit(1);
						}//else
				}//else
				i++; //increment i for numCommands
			}//while (i < numCommands)
		}//if(fgets(userInput, 100, stdin));
	}//while(!shouldExit)
 	return 0; //return 0 to exit program
}//int main()


void pipeItUp(char *pipeStr) {
	char *pipe1 = strtok(pipeStr, "|");
	char *pipe2 = strtok(NULL, "|");
	char *pipe3 = strtok(NULL, "\n");
	char *pipeArgs1[100];
	char *pipeArgs2[100];
	char *pipeArgs3[100];
	int fd1[2];
	int fd2[2];
	
	pid_t pid1, pid2, pid3, pid4;
	int i = 0;

	pipeArgs1[i] = strtok(pipe1, " \n");
	
	while (pipeArgs1[i] != NULL) {
		i++;
		pipeArgs1[i] = strtok(NULL, " \n");
	}

	i = 0;
	pipeArgs2[i] = strtok(pipe2, " \n");

	while (pipeArgs2[i] != NULL) {
		i++;
		pipeArgs2[i] = strtok(NULL, " \n");
	}
	
	if (pipe3 != NULL) {	// If a 2nd '|' exists
		int i = 0;
		pipeArgs3[i] = strtok(pipe3, " \n");

		while (pipeArgs3[i] != NULL) {
			i++;
			pipeArgs3[i] = strtok(NULL, " \n");
		}
	}

	// Piping

	if (pipe(fd1) == -1) {
		printf("Pipe1 error");
		return;
	}

	if(pipe3 != NULL) {
		if (pipe(fd2) == -1) {
			printf("Pipe2 error");
			return;
		}
	}

	pid1 = fork();
	if (pid1 < 0) {			// ERROR1
		printf("Fork1 error");
		return;
	} else if (pid1 == 0) {	// CHILD1
		close(fd1[0]);
		dup2(fd1[1], fileno(stdout));

		close(fd1[1]);
		if(pipe3 != NULL) {
			close(fd2[0]);
			close(fd2[1]);
		}

		if ((execvp(pipeArgs1[0], pipeArgs1)) < 0 || pipeArgs1[0] == "exit") {
			printf("Exec1 error");
			exit(1);
		} //if
	} else {				// PARENT1
		pid2 = fork();

		if (pid2 < 0) {			// ERROR2
			printf("Fork2 error");
			return;
		} else if (pid2 == 0) {	// CHILD2
			dup2(fd1[0], fileno(stdin));
			if (pipe3 != NULL) {
				dup2(fd2[1], fileno(stdout));
				close(fd1[0]);
			} else {
				close(fd1[1]);
			}

			
			close(fd1[1]);
			if(pipe3 != NULL) {
				close(fd2[0]);
				close(fd2[1]);
			}
			
			if ((execvp(pipeArgs2[0], pipeArgs2)) < 0 || pipeArgs2[0] == "exit") {
				printf("Exec2 error");
				exit(1);
			}
		} else {				// PARENT2
			if (pipe3 != NULL) {	// If second '|' exists

				pid3 = fork();
				if (pid3 < 0) {			// ERROR3
					printf("Fork3 error");
					return;
				} else if (pid3 == 0) {	// CHILD3
					dup2(fd2[0], fileno(stdin));

					close(fd1[0]);
					close(fd1[1]);
					close(fd2[0]);
					close(fd2[1]);

					if ((execvp(pipeArgs3[0], pipeArgs3)) < 0 || pipeArgs3[0] == "exit") {
						printf("Exec3 error");
						exit(1);
					}
				} 
				else {				// PARENT3
					close(fd1[0]);
					close(fd1[1]);
					close(fd2[0]);
					close(fd2[1]);
					wait(NULL);
				}
			} else {
				close(fd1[0]);
				close(fd1[1]);
				if (pipe3 != NULL) {
					close(fd2[0]);
					close(fd2[1]);
				}
			}
			wait(NULL);
		}
		wait(NULL);	
	}
}

// BatchMode function
void BatchMode(int argc, char* argv[]) {
	//open file
	if (argc > 1) { //check if argument is greater than one

		FILE *file; //file
			file = fopen(argv[1],"r"); //open file
   		if (file == NULL) {
      			printf("error opening file\n"); //error openining file
      			exit(1); //exit
		} //if

	int shouldExit = 0; // Change to true when user enters exits
	char userInput[1000]; // User input
	char cwd[FILENAME_MAX]; // Prompt that user sees on each command
	char *commands[1000]; // Holds up to 1000 commands
	int numCommands = 0; // Counter for number of commands
	char *arguments[1000]; // Holds up to 1000 arguments for commands
	char history[HISTORYCHUNK][CMDLENGTH]; //history array
	int hisCount = 0; //history counter
	int i = 0; //counter
	char *redirect_filename; // Point to spot in commands[i] where redirect
	int  redirect_type;   //-1 = none, 0=input, 1=ouptut, 2 = append
	int  rd_fd;
	char newPath[1024];
	char pathToken[FILENAME_MAX];
	char *path;  // pointer to path environment variable

	regex_t regex; //regex
	regex_t pipe_regex;
	regex_t bash2_regex;
	char* re_pattern = "^(\\s*;*)+$";
	char* pipe_pattern = "^(([^\\|]+)[\\|]{1}([^\\|]+)([\\|]{1}[^\\|]+){0,1})$";
	char* bash2_pattern = "^bash2\\s+[^ ]+$";
	int pipe_re = regcomp(&pipe_regex, pipe_pattern, REG_EXTENDED);
	int re_ret = regcomp(&regex, re_pattern, REG_EXTENDED);
	int b2_ret = regcomp(&bash2_regex, bash2_pattern, REG_EXTENDED);
	//variable parameter for waitpid() system call
	int status;
	//process id declaration
	pid_t pid;
	// Loop until user types exit
		while(fgets(userInput, 1000, file)!=NULL) {
			if(shouldExit)
				break;

			//----------blocking signals----------------
			//set background shell
			if(tcsetpgrp(fileno(stdin), getpgrp())<0){
				perror("error with tcsetpgrp background grouping"); //report error with background grouping
			} //if

			//set mask to block signals in background
			sigset_t mask;
			//mask the signals in the background
			sigfillset(&mask);
			//adminster the mask for all signals in the background
			if(sigprocmask(SIG_SETMASK, &mask, NULL)<0){
				perror("SIG_SETMASK blocking signal error\n"); //report error with sigprocmask
			} //if

			//----------end blocking signals----------------

			/*if (getcwd(cwd, sizeof(cwd)) != NULL) {
				//print prompt (current working directory with cyan color
				printf(ANSI_COLOR_CYAN "%s$ "  ANSI_COLOR_MAGENTA, cwd);
			} else {
				perror("getcwd() error");
				exit(1);
			}*/

			//if(fgets(userInput, sizeof(userInput), file)) {	// Scan for user input
				// Get rid of the \n from the user input
				re_ret = regexec (&regex, userInput, 0, NULL, 0);  //TODO ::comment needed
				if (re_ret == 0) { //TODO ::comment needed
					continue; //TODO ::comment needed
				}

				if(userInput[strlen(userInput) - 1] == '\n') { //TODO ::comment needed
					userInput[strlen(userInput) - 1] = '\0'; //TODO ::comment needed
				}
				printf(ANSI_COLOR_CYAN "%s\n" ANSI_COLOR_MAGENTA, userInput);
				
				if (hisCount >= 20) { //TODO ::comment needed
				    insertCMD(history, userInput); //TODO ::comment needed
				    hisCount++; //TODO ::comment needed
			    } else {
			        hisCount++;
				    strcpy(history[hisCount - 1], userInput); //TODO ::comment needed
			    }

				char *token = strtok(userInput, ";");	// Tokenize user input by ;
				// While not reached end of token
				while (token != NULL) {
					commands[numCommands] = token; // Store the token as a command
					token = strtok(NULL, ";"); // Tokenize end by ;
					numCommands++; // + 1 command separated by ;
				}
			
				// Loop until number of commands. Get arguments of commands
				while (i < numCommands) {
					 if  (regexec(&regex,commands[i], 0, NULL, 0) == 0) {
						i++;
						//printf("command not found\n");
						continue;
					 }
					 else if (regexec(&pipe_regex, commands[i], 0, NULL, 0) == 0) {
				    	pipeItUp(commands[i]); //piping regex call function here
				    	i++;
				        continue;
					}

					else if (regexec(&bash2_regex, commands[i], 0, NULL, 0) == 0) {
				    	bash2(commands[i]);
				    	i++;
				    	continue;
					}
					
					
                // Scan commands[i] for a redirect character
				// IF we find, replace with \0 to make commands[i]
				//    only contain the command and arguments 
                //  Store location of redirect_filename (first character
                //  after the > < so that we can do the redirect)
				redirect_filename = commands[i];
				redirect_type = -1;
				while(*redirect_filename != '\0')
				{
				    if(*redirect_filename == '>')
				    {
				        redirect_type = 1;
				        *redirect_filename = '\0';
				        redirect_filename++;
				        break;
				    }
				    else if(*redirect_filename == '<')
				    {
				        redirect_type = 0;
				        *redirect_filename = '\0';
				        redirect_filename++;
				        break;
				    }
			        redirect_filename++;
				}
				
				// Get rid of spaces in redirect filename
				if(redirect_type >=0)
				{
				    while(*redirect_filename == ' ' || *redirect_filename =='\t')
				    {
				        redirect_filename++;
				    }
				}
                
					
					
					


					arguments[0] = strtok(commands[i], " \n");	// Tokenize by whitespace
					int t = 0;
					while (arguments[t] != NULL) {
						t++;
						arguments[t] = strtok(NULL, " \n");	// Tokenize by whitespace
					}

					if(strcmp(arguments[0], "exit") == 0 && arguments[1] == NULL) { // If user inputs "exit" command
						shouldExit = 1;	// Change conditional while loop variable to 1, so we can exit the loop
					} else if (strcmp(arguments[0], "cd") == 0) { // If user inputs "cd" command
						// TODO:: Change prompt to show path
						if(!arguments[1]) {	// If cd argument is empty
							if(chdir(getenv("HOME")) == -1) {
								perror("cd");
							}
						    } else {
							    if(chdir(arguments[1]) == -1) {
								perror("cd");
							}
						}
					} else if (strcmp(arguments[0], "myhistory") == 0 && arguments[1] == NULL){
						    displayHistory(history, hisCount);
					} 
					else if(strcmp(arguments[0], "path") == 0) {
				     if(arguments[1] == NULL) {
				         path = getenv("PATH");
				         printf("%s\n", path);
				     }
				     else if(arguments[1][0] == '+') {
				         path = getenv("PATH");   // Get existing path
				         strcpy(newPath,path);  // Copy the old path into newPath variable
				         strcat(newPath,":");   // Add : delimiter
				         strcat(newPath,arguments[2]);  // Add our new path piece
				         if(setenv("PATH",newPath,1)<0) {        // Set PATH to our new path
				             printf("Failed to update path\n");
				         }
				     }
				     else if(arguments[1][0] == '-') {
				         path = getenv("PATH");   // Get existing path
				         char *t = pathToken;
				         *newPath = '\0';
				         while(*path != '\0')
				         {
				             if(*path != ':')
				             {
				                 *t = *path;
				                 t++;
				             }
				             else
				             {
				                 *t = '\0';
				                 t = pathToken;
				                 if(strcmp(pathToken,arguments[2])!=0)
				                 {
				                     if(strlen(newPath)!=0)
				                     {
				                         strcat(newPath,":");
				                     }
				                     strcat(newPath,pathToken);
				                 }
				             }
				             path++;
				         }
				         // Make sure we re-add the last path 
				         *t = '\0';
				         if(strcmp(pathToken,arguments[2])!=0)
		                 {
		                     if(strlen(newPath)!=0)
		                     {
		                         strcat(newPath,":");
		                     }
		                     strcat(newPath,pathToken);
		                 }
		                 if(setenv("PATH",newPath,1)<0) {        // Set PATH to our new path
				             printf("Failed to update path\n");
				         }
				     }
				     else
				     {
				         printf("Invalid arguments supplied to path command\n");
				     }
				     
				 }

					
					
					else {
					    
						if(redirect_type == 0)
						{
						    rd_fd = open(redirect_filename,O_RDONLY);
						    if(rd_fd < 0)
						    {
						        printf("Could not use: %s for std input\n",redirect_filename);
						        redirect_type = -1;
						    }
						}
						else if(redirect_type == 1)
						{
						    rd_fd = open(redirect_filename,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
						}	
						
					    
					    
							// Proceed to the fork(), exec() wait() model for all other commands
							//fork
							pid=fork();
							if (pid == 0) {	// CHILD
								//----------signals for child----------------
								//set the group for foreground subprocesses
								if(setpgrp()<0){
									perror("error setting process group setpgrp()"); //error setting process group
								} //if
								//set the foreground subprocesses
								if(tcsetpgrp(fileno(stdin), getpgid(pid))<0){
									perror("error with tcsetpgrp grouping child"); //error with grouping child
								} //if
								//unmask signals for forground subprocesses
								sigemptyset(&mask);
								//unmask all signals in the forground subprocesses
								if(sigprocmask(SIG_SETMASK, &mask, NULL)<0){
									perror("SIG_SETMASK unblocking signal error\n"); //report error with sigpromask
								} //if
								
							  if(redirect_type == 0)
                            {
                                dup2(rd_fd,0);
                                close(rd_fd);
                            }
                            else if(redirect_type == 1)
                            {
                                dup2(rd_fd,1);
                                close(rd_fd);                                
                            }
								
								
								
								
								//----------end signals for child----------------
								if ((execvp(arguments[0], arguments)) < 0 || arguments[0] == "exit") {
									if (strcmp(commands[0], "bash2") == 0){
									    printf("bash2 has too many or not enough arguments\n");
								    }
								    else {
									    printf("The command %s does not exist or cannot be executed\n",commands[i]);
								    }
								} //if

								exit(0);
							} else if (pid > 0) { // PARENT
								//----------signals for parent----------------
								//set parent id
								if(setpgid(pid, pid)<0){
									perror("error with setpgid() grouping parent process"); //report error for setpgid
								} //if
								//ignore when process in background tries to write to the terminal or set its modes
								if(signal(SIGTTOU, SIG_IGN)==SIG_ERR){
									perror("error has occured when ignoring sginal SIGTTOU\n"); //report error for SIGTTOU
								} //if
								//set background
								if(tcsetpgrp(fileno(stdin), getpgid(pid))<0){
									perror("error with tcsetpgrp grouping parent\n"); //report error when grouping parent
								} //if
								//wait for child to finish based on specific pid
								if(waitpid(-1, &status, WUNTRACED | WCONTINUED)<0){
									perror("error with waitpid()\n"); //report error with waitpid
								} //if
								//----------end signals for parent---------------
							} else { // FORK ERROR
								perror("fork error \n");
								exit(1);
							}//else
					}//else
					i++; //increment i for numCommands
				}//while (i < numCommands)

			//}//if(fgets(userInput, 100, stdin));
			//fclose(file); //close file
		} //while(!shouldExit)
			fclose(file);
	} //if(argc>1)
	else {
		printf("no arg\n"); //print no argument if argument not found
	} //else
} //void BatchMode()