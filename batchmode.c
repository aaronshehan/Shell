void BatchMode(int argc, char* argv[]) {

	if (argc > 1) {
	
   		FILE *file;
      		file = fopen(argv[1],"r");
   		if (file == NULL) {
      			printf("error opening file");   
      			exit(1);             
   		}


		int shouldExit = 0;		// Change to true when user enters exits 
		char userInput[1000];		// User input
		//char prompt[1000] = "Major2>";	// Prompt that user sees on each command
		char *commands[1000];    	// Holds up to 1000 commands
		int numCommands = 0;		// Counter for number of commands
		char *arguments[1000];		// Holds up to 1000 arguments for commands
		int i = 0;

		pid_t pid;

   		while (fgets(userInput, 1000, file) != NULL) {
			if (shouldExit)
				break;

			printf("%s\n", userInput);
			char *token = strtok(userInput, ";");	// Tokenize user input by ;
			
			// While not reached end of token
			while (token != NULL) {
				commands[numCommands] = token;		// Store the token as a command
				token = strtok(NULL, ";");		// Tokenize end by ;
				numCommands++;				// + 1 command separated by ;
			}

			// Loop until number of commands. Get arguments of commands
			while (i < numCommands) {
				arguments[0] = strtok(commands[i], " \n");	// Tokenize by whitespace
				
				int t = 0;
				
				while (arguments[t] != NULL) {
					t++;
					arguments[t] = strtok(NULL, " \n");	// Tokenize by whitespace
				}

				// If user inputs 'exit'
				if((strstr(arguments[0],"exit"))) {
					shouldExit = 1;	// Change conditional while loop variable to 1, so we can exit the loop
				} else {	// Proceed to the fork(), exec() wait() model		
					if ((pid = fork()) == 0) { 	// CHILD
						
						// If the command is not valid then output error message
						if ((execvp(arguments[0], arguments)) < 0 || arguments[0] == "exit") {
							printf("Error! Command not recognized.\n");
						}

						exit(0);
					} else if (pid > 0) {		// PARENT

						wait(&pid); // Wait for child
					} else {			// FORK ERROR

						perror("fork error \n");
						exit(1);
					}//else
				}//else

				i++;
			}//while (i < numCommands)
		}	

		fclose(file);
	}

	else {
		printf("no arg\n");
	}
}
