#include "bash2.h"


//initializes String to null
String* string_init() {
	String* str = (String*)malloc(sizeof(String));
	if (str == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	str->str = NULL;
	str->len = 0;
}

//initialized String to value
String* new_string(char* s) {
	String* new = (String*)malloc(sizeof(String));
	if (new == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	new->str = (char*)malloc(sizeof(char) * (strlen(s) + 1));
	if (new->str == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	strcpy(new->str, s);
	new->len = strlen(s);
	return new;
}

//free memory
void free_string(String* string) {
	free(string->str);
	string->len = 0;
}

//free memory
void free_token(Token* token) {
	free_string(token->token);
	free_string(token->val);
	free_string(token->type);
}

//initialize token, assign assigns to to token, sets type and val to null
Token* new_token(char* t) {
	Token* new = (Token*)malloc(sizeof(Token));
	if (new == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	new->token = new_string(t);
	new->type = string_init();
	new->val = string_init();
	return new;
}

//initialize all to null
Token* token_init() {
	Token* new = (Token*)malloc(sizeof(Token));
	if (new == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	new->token = string_init();
	new->type = string_init();
	new->val = string_init();
	return new;
}

//create a substring
String* substring(char* string, int begin, int end) {

    //make sure indices are valid
	if (begin > end || begin < 0 || end > strlen(string)) {
		return NULL;
	}

	String* substr = string_init();
	//allocate memory for new substring
	substr->str = (char*)malloc(sizeof(char) * (end - begin + 2));
	if (substr->str == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	int i, j = 0;
	//copy characters one by one
	for (i = begin; i <= end; i++) {
		substr->str[j++] = string[i];
	}
	substr->str[j] = '\0'; //terminate string with null character
	substr->len = strlen(substr->str);
	return substr;
}

//create array of tokens
String** tokenize(String* line) {
    //allocate memory
	String** tokens = (String**)malloc(sizeof(String*) * (line->len + 1));
	if (tokens == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

    //more memory allocation
	for (int i = 0; i < line->len + 1; i++) {
		tokens[i] = string_init();
	}
	int i, begin = 0, end = 0, count = 0;
    
    //get new tokens
	for (i = 0; line->str[i] != '\0'; i++) {
		if (line->str[i] == ' ' || line->str[i] == '=' || line->str[i] == '+' || line->str[i] == '-' || line->str[i] == '*') {
			end = i - 1;
			tokens[count++] = substring(line->str, begin, end);
			tokens[count - 1]->len = strlen(tokens[count - 1]->str);
			tokens[count++] = substring(line->str, i, i);
			tokens[count - 1]->len = 1;
			begin = end + 2;
		}
	}
	tokens[count++] = substring(line->str, begin, i);
	tokens[count] = NULL;  //terminate array with null
	return tokens;
}

//tokenize specific instance when a variable is assigned to be a string
String** assign_string_tokenize(String* line) {
    //allocate memory
    String** tokens = (String**)malloc(sizeof(String*) * 3);
	if (tokens == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
    //allocate more memory
    for (int i = 0; i < 2; i++) {
		tokens[i] = string_init();
	}
	
	//get tokens
	int i, count = 0;
	for (i = 0; line->str[i] != '='; i++);
	tokens[count++] = substring(line->str, 0, i - 1);
	int firstquote = i + 2;
	for (i = firstquote; line->str[i] != '\0'; i++);
	int secondquote = i - 2;
	tokens[count++] = substring(line->str, firstquote, secondquote);
	tokens[count] = NULL;
	return tokens;
}

//create token for specific instance, when command display is called with quotes
//example: display "hello world"
String** tokenize_display(String* line) {
	int i;
	for (i = 0; line->str[i] != '\"'; i++);
	int firstquote = ++i;
	for (i = firstquote; line->str[i] != '\0'; i++);
	int secondquote = i - 2;
	String* str = substring(line->str, firstquote, secondquote);
	return tokenize(str);
}

//perform some syntax checking with regex
int analyze_line(char* line) {
	regex_t regex;
	char* pattern1 = "^[a-zA-Z_]\\w*[=]([$][a-zA-Z_]\\w*|[0-9]+)[-+*]([$][a-zA-Z_]\\w*|[0-9]+)$";	// <var><assignment>(<var>|<num>)<operator>(<var>|<num>)
	char* pattern2 = "^[a-zA-Z_]\\w*[=]([$][a-zA-Z_]\\w*|[0-9]+)$";										// <var><assignment>(<num>|<var>)
	char* pattern3 = "^display\\s\".+\"$";																// <display><whitespace><quote><string><quote>
	char* pattern4 = "^[a-zA-Z_]\\w*[=]\".+\"$";														// <var><assignment><quote><string><quote>
	char* pattern5 = "(^display\\s([$][a-zA-Z_]\\w*|[0-9]+))$";											// <display><whitespace>(<num>|<var>)
	char* pattern6 = "^[#].*$";																			// <comment>
	char* pattern7 = "^[\n]|\\s*$";																		// <whitespace>
	
	int val;
	val = regcomp (&regex, pattern1, REG_EXTENDED);		// compile pattern
	val = regexec (&regex, line, 0, NULL, 0);		// execute pattern matching
	if (val == 0) {
	    return ASSIGNOP;
	}

	val = regcomp (&regex, pattern2, REG_EXTENDED);		// compile pattern
	val = regexec (&regex, line, 0, NULL, 0);		// execute pattern matching
	if (val == 0) {
	    return ASSIGNNUM;
	}
	
	val = regcomp (&regex, pattern3, REG_EXTENDED);		// compile pattern
	val = regexec (&regex, line, 0, NULL, 0);		// execute pattern matching
	if (val == 0) {
	    return DISPQUOTE;
	}
	
	val = regcomp (&regex, pattern4, REG_EXTENDED);		// compile pattern
	val = regexec (&regex, line, 0, NULL, 0);		// execute pattern matching
	if (val == 0) {
	    return ASSIGNSTR;
	}
	
	val = regcomp (&regex, pattern5, REG_EXTENDED);		// compile pattern
	val = regexec (&regex, line, 0, NULL, 0);		// execute pattern matching
	if (val == 0) {
	    return DISPSTRNUM;
	}

	val = regcomp (&regex, pattern6, REG_EXTENDED);		// compile pattern
	val = regexec (&regex, line, 0, NULL, 0);		// execute pattern matching
	if (val == 0) {
	    return COMMENT;
	}

	val = regcomp (&regex, pattern7, REG_EXTENDED);		// compile pattern
	val = regexec (&regex, line, 0, NULL, 0);		// execute pattern matching
	if (val == 0) {
	    return WHITESPACE;
	}

	return -1;	// no match found
}

//check to see if variable has been assigned yet
int in_set(Token** vars, String* token) {
	for (int i = 0; vars[i] != NULL; i++) {
		if (strcmp(token->str, vars[i]->token->str) == 0) {
			return 1;
		}
	}
	return 0;
}

//insert new variable
void set_insert(Token** vars, String* token) {
    int i = 0;
    for (i = 0; vars[i] != NULL; i++);
    vars[i] = new_token(token->str);
}

//get value for variable
char* get_value(Token** vars, String* var) {
    for (int i = 0; vars[i] != NULL; i++) {
		if (strcmp(var->str, vars[i]->token->str) == 0) {
			return vars[i]->val->str;
		}
	}
}

//get variable type
char* get_type(Token** vars, String* var) {
    for (int i = 0; vars[i] != NULL; i++) {
		if (strcmp(var->str, vars[i]->token->str) == 0) {
			return vars[i]->type->str;
		}
	}
}

//display command
void display(Token** vars, String** tokens, int type) {
	//if quotes are used
	if (type == 4) {
		for (int i = 0; tokens[i] != NULL; i++) {
			if (tokens[i]->str[0] == '$' && tokens[i]->len > 1) {
				String* var = substring(tokens[i]->str, 1, tokens[i]->len - 1);
				if (in_set(vars, var)) {
					printf("%s", get_value(vars, var));
					continue;
				}
				else {
					printf("(variable not initialized)");
					continue;
				}
				free_string(var);
				free(var);
			}
			printf("%s", tokens[i]->str);
		}
		printf("\n");
	}
	else {
		regex_t regex;
    	char* pattern = "^[0-9]+$";
    	int val = regcomp(&regex, pattern, REG_EXTENDED);
		if (!regexec(&regex, tokens[2]->str, 0, NULL, 0)) {
			printf("%s\n", tokens[2]->str);
		}
		else {
			String* var = substring(tokens[2]->str, 1, tokens[2]->len - 1);
			if (in_set(vars, var)) {
				printf("%s\n", get_value(vars, var));
			}
			else {
				printf("%s has not been initialized.\n", var->str);
			}
		}
	}
}

//update value of variable
void update_val(Token** vars, String* var, char* newval) {
    for (int i = 0; vars[i] != NULL; i++) {
		if (strcmp(var->str, vars[i]->token->str) == 0) {
		    free(vars[i]->val);
			vars[i]->val = new_string(newval);
			return;
		}
	}
}

//update variable type
void update_type(Token** vars, String* var, char* type) {
    for (int i = 0; vars[i] != NULL; i++) {
		if (strcmp(var->str, vars[i]->token->str) == 0) {
		    free(vars[i]->type);
			vars[i]->type = new_string(type);
			return;
		}
	}
}

//assign value to variable
void assign(Token** vars, String** tokens, int type) {
    regex_t regex;
    char* pattern = "^[0-9]+$";
    int value;
    char newval[15];
    String *newvar, *var1, *var2;
    int val = regcomp(&regex, pattern, REG_EXTENDED);
    if (type == ASSIGNOP) {
	    if (in_set(vars, tokens[0])) {  //variable exists already
		   if (!regexec(&regex, tokens[2]->str, 0, NULL, 0) && !regexec(&regex, tokens[4]->str, 0, NULL, 0)) {  //use regex to determine if two integers are used instead of variables ex: a=2+4
		       //do operation
		       if (strcmp(tokens[3]->str, "+") == 0) {
		           value = atoi(tokens[2]->str) + atoi(tokens[4]->str);
		       }
		       else if (strcmp(tokens[3]->str, "-") == 0) {
		           value = atoi(tokens[2]->str) - atoi(tokens[4]->str);
		       }
		       else {
		           value = atoi(tokens[2]->str) * atoi(tokens[4]->str);
		       }
		       //update value and type
		       sprintf(newval, "%d", value);
		       update_val(vars, tokens[0], newval);
		       update_type(vars, tokens[0], "int");
		   }
		   else if (!regexec(&regex, tokens[2]->str, 0, NULL, 0)) {     //regex to check if token on left side of operand is a number
		       var1 = substring(tokens[4]->str, 1, tokens[4]->len - 1);
		       if (!in_set(vars, var1)) {
		           printf("%s is not initialized\n", var1->str);
		           return;
		       }
		       
		       if (strcmp(get_type(vars, var1), "string") == 0) {
		           printf("%s is String\n", var1->str);
		           return;
		       }
		       
		       //do operation
		       if (strcmp(tokens[3]->str, "+") == 0) {  
		           value = atoi(tokens[2]->str) + atoi(get_value(vars, var1));
		       }
		       else if (strcmp(tokens[3]->str, "-") == 0) {
		           value = atoi(tokens[2]->str) - atoi(get_value(vars, var1));
		       }
		       else {
		           value = atoi(tokens[2]->str) * atoi(get_value(vars, var1));
		       }
		
		       //assign new value and update type
		       sprintf(newval, "%d", value);
		       update_val(vars, tokens[0], newval);
		       update_type(vars, tokens[0], "int");
		   }
		   else if (!regexec(&regex, tokens[4]->str, 0, NULL, 0)) { //regex to see if token on right side of operand is a number
		       var1 = substring(tokens[2]->str, 1, tokens[2]->len - 1);
		       if (!in_set(vars, var1)) {
		           printf("%s is not initialized\n", var1->str);
		           return;
		       }
		       
		       if (strcmp(get_type(vars, var1), "string") == 0) {
		           printf("%s is String\n", var1->str);
		           return;
		       }
		       
		       //do operation
		       if (strcmp(tokens[3]->str, "+") == 0) {
		           value = atoi(get_value(vars, var1)) + atoi(tokens[4]->str);
		       }
		       else if (strcmp(tokens[3]->str, "-") == 0) {
		           value = atoi(get_value(vars, var1)) - atoi(tokens[4]->str);
		       }
		       else {
		           value = atoi(get_value(vars, var1)) * atoi(tokens[4]->str);
		       }
		       //update value and type
		       sprintf(newval, "%d", value);
		       update_val(vars, tokens[0], newval);
		       update_type(vars, tokens[0], "int");
		   }
		   else {
		       //get tokens on each side of operand, strip dollar sign from each
		       var1 = substring(tokens[2]->str, 1, tokens[2]->len - 1);
		       var2 = substring(tokens[4]->str, 1, tokens[4]->len - 1);
		       
		       if (!in_set(vars, var1) && !in_set(vars, var2)) {
		           printf("%s and %s are not initialized\n", var1->str, var2->str);
		           return;
		       }
		       else if(!in_set(vars, var1)) {
		           printf("%s is not initialized\n", var1->str);
		           return;
		       }
		       else if(!in_set(vars, var2)) {
		           printf("%s is not initialized\n", var2->str);
		           return;
		       }
		       
		       if (strcmp(get_type(vars, var1), "string") == 0 && strcmp(get_type(vars, var2), "string") == 0) {
		           printf("%s and %s are Strings\n", var1->str, var2->str);
		           return;
		       }
		       else if (strcmp(get_type(vars, var1), "string") == 0) {
		           printf("%s is a String\n", var1->str);
		           return;
		       }
		       else if (strcmp(get_type(vars, var2), "string") == 0) {
		           printf("%s is a String\n", var2->str);
		           return;
		       }
		       
		       
		       if (strcmp(tokens[3]->str, "+") == 0) {
		           value = atoi(get_value(vars, var1)) + atoi(get_value(vars, var2));
		       }
		       else if (strcmp(tokens[3]->str, "-") == 0) {
		           value = atoi(get_value(vars, var1)) - atoi(get_value(vars, var2));
		       }
		       else {
		           value = atoi(get_value(vars, var1)) * atoi(get_value(vars, var2));
		       }
                //update value and type
		       sprintf(newval, "%d", value);
		       update_val(vars, tokens[0], newval);
		       update_type(vars, tokens[0], "int");
		   }
		}
        else {  //variable has not been assigned yet
            if (!regexec(&regex, tokens[2]->str, 0, NULL, 0) && !regexec(&regex, tokens[4]->str, 0, NULL, 0)) {
		       if (strcmp(tokens[3]->str, "+") == 0) {
		           value = atoi(tokens[2]->str) + atoi(tokens[4]->str);
		       }
		       else if (strcmp(tokens[3]->str, "-") == 0) {
		           value = atoi(tokens[2]->str) - atoi(tokens[4]->str);
		       }
		       else {
		           value = atoi(tokens[2]->str) * atoi(tokens[4]->str);
		       }
		       //add new variable to set
		       //update variable and type
		       set_insert(vars, tokens[0]);
		       sprintf(newval, "%d", value);
		       update_val(vars, tokens[0], newval);
		       update_type(vars, tokens[0], "int");
		   }
		   else if (!regexec(&regex, tokens[2]->str, 0, NULL, 0)) {
		       var1 = substring(tokens[4]->str, 1, tokens[4]->len - 1);
		       if (!in_set(vars, var1)) {
		           printf("%s is not initialized\n", var1->str);
		           return;
		       }
		       
		       if (strcmp(get_type(vars, var1), "string") == 0) {
		           printf("%s is a String\n", var1->str);
		           return;
		       }
		       
		       if (strcmp(tokens[3]->str, "+") == 0) {
		           value = atoi(tokens[2]->str) + atoi(get_value(vars, var1));
		       }
		       else if (strcmp(tokens[3]->str, "-") == 0) {
		           value = atoi(tokens[2]->str) - atoi(get_value(vars, var1));
		       }
		       else {
		           value = atoi(tokens[2]->str) * atoi(get_value(vars, var1));
		       }
		       //insert new variable into set
		       //update value and type
		       set_insert(vars, tokens[0]);
		       sprintf(newval, "%d", value);
		       update_val(vars, tokens[0], newval);
		       update_type(vars, tokens[0], "int");
		   }
		   else if (!regexec(&regex, tokens[4]->str, 0, NULL, 0)) {
		       var1 = substring(tokens[2]->str, 1, tokens[2]->len - 1);
		       if (!in_set(vars, var1)) {
		           printf("%s is not initialized\n", var1->str);
		           return;
		       }
		       
		       if (strcmp(get_type(vars, var1), "string") == 0) {
		           printf("%s is a String\n", var1->str);
		           return;
		       }
		       
		       if (strcmp(tokens[3]->str, "+") == 0) {
		           value = atoi(get_value(vars, var1)) + atoi(tokens[4]->str);
		       }
		       else if (strcmp(tokens[3]->str, "-") == 0) {
		           value = atoi(get_value(vars, var1)) - atoi(tokens[4]->str);
		       }
		       else {
		           value = atoi(get_value(vars, var1)) * atoi(tokens[4]->str);
		       }
		       //insert variable into set
		       //update value and type
		       set_insert(vars, tokens[0]);
		       sprintf(newval, "%d", value);
		       update_val(vars, tokens[0], newval);
		       update_type(vars, tokens[0], "int");
		   }
		   else {
		       //strip dollar sign from both variables
		       var1 = substring(tokens[2]->str, 1, tokens[2]->len - 1);
		       var2 = substring(tokens[4]->str, 1, tokens[4]->len - 1);
		       
		       if (!in_set(vars, var1) && !in_set(vars, var2)) {
		           printf("%s and %s are not initialized\n", var1->str, var2->str);
		           return;
		       }
		       else if(!in_set(vars, var1)) {
		           printf("%s is not initialized\n", var1->str);
		           return;
		       }
		       else if(!in_set(vars, var2)) {
		           printf("%s is not initialized\n", var2->str);
		           return;
		       }
		       
		       if (strcmp(get_type(vars, var1), "string") == 0 && strcmp(get_type(vars, var2), "string") == 0) {
		           printf("%s and %s are Strings\n", var1->str, var2->str);
		           return;
		       }
		       else if (strcmp(get_type(vars, var1), "string") == 0) {
		           printf("%s is a String\n", var1->str);
		           return;
		       }
		       else if (strcmp(get_type(vars, var2), "string") == 0) {
		           printf("%s is a String\n", var2->str);
		           return;
		       }
		       
		       
		       if (strcmp(tokens[3]->str, "+") == 0) {
		           value = atoi(get_value(vars, var1)) + atoi(get_value(vars, var2));
		       }
		       else if (strcmp(tokens[3]->str, "-") == 0) {
		           value = atoi(get_value(vars, var1)) - atoi(get_value(vars, var2));
		       }
		       else {
		           value = atoi(get_value(vars, var1)) * atoi(get_value(vars, var2));
		       }
		       
		       //inserr variable into set
		       //update value and type
		       set_insert(vars, tokens[0]);
		       sprintf(newval, "%d", value);
		       update_val(vars, tokens[0], newval);
		       update_type(vars, tokens[0], "int");
		   }
        }
    }
    else if (type == ASSIGNNUM) { //is this case a variable is being assigned a single value, ex: a=3 or a=$d
        if(in_set(vars, tokens[0])) {
            if (!regexec(&regex, tokens[2]->str, 0, NULL, 0)) {
                update_val(vars, tokens[0], tokens[2]->str);
                update_type(vars, tokens[0], "int");
            }
            else {
                var1 = substring(tokens[2]->str, 1, tokens[2]->len - 1);
		        if (!in_set(vars, var1)) {
		            printf("%s is not initialized\n", var1->str);
		            return;
	            }
	            
	            update_val(vars, tokens[0], get_value(vars, var1));
	            update_type(vars, tokens[0], get_type(vars, var1));
            }
        }
        else {
            newvar = substring(tokens[0]->str, 0, tokens[0]->len - 1);
            if (!regexec(&regex, tokens[2]->str, 0, NULL, 0)) {
                set_insert(vars, newvar);
                update_val(vars, tokens[0], tokens[2]->str);
                update_type(vars, tokens[0], "int");
            }
            else {
                var1 = substring(tokens[2]->str, 1, tokens[2]->len - 1);
		        if (!in_set(vars, var1)) {
		            printf("%s is not initialized\n", var1->str);
		            return;
	            }
	            
	            set_insert(vars, tokens[0]);
	            update_val(vars, tokens[0], get_value(vars, var1));
	            update_type(vars, tokens[0], get_type(vars, var1));
            }
        }
    }
    else {
        if (in_set(vars, tokens[0])) {
            update_val(vars, tokens[0], tokens[1]->str);
			update_type(vars, tokens[0], "string");
        }
        else {
            set_insert(vars, tokens[0]);
			update_val(vars, tokens[0], tokens[1]->str);
			update_type(vars, tokens[0], "string");
        }
        
	}
}

void bash2(char* filename) {
    //get filename
    String* fname = substring(filename, 6, strlen(filename) - 1);
  
	regex_t regex;
	char* pattern = "^\\w+.sh$";    //regex to test if valid file name given
	int val = regcomp(&regex, pattern, REG_EXTENDED);
	val = regexec(&regex, fname->str, 0, NULL, 0);

	if (val != 0){
		printf("invalid file\n");   //not a .sh file
		return;
	}
	
	FILE* file;
    file = fopen(fname->str,"r");		// open file
   	if (file == NULL) {			// check if file opened
      		printf("error opening file");   
      		return;            
	}
    free_string(fname); free(fname);
	char input[CMDLEN];
	String* line = string_init();   //line of file
	String** line_tokens;   //array of tokens
	Token** vars = (Token**)malloc(sizeof(Token*) * 10000);     //array of variables seen
	if (vars == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < 10000; i++) {
		vars[i] = NULL; //allocate memory
	}

	int line_analysis, lineNum = 1;
	while (fgets(input, CMDLEN, file) != NULL) {    //get line from file

		if(input[strlen(input) - 1] == '\n') {		// get rid of newline
			input[strlen(input) - 1] = '\0';
		}

		line = new_string(input);
		line_analysis = analyze_line(line->str);    //regex evaluator

		switch (line_analysis) {    //what to do with line
			case ASSIGNOP: {
				line_tokens = tokenize(line);
				assign(vars, line_tokens, ASSIGNOP);
				break;
			}
			case ASSIGNNUM: {
				line_tokens = tokenize(line);
				assign(vars, line_tokens, ASSIGNNUM);
				break;
			}
			case ASSIGNSTR: {
				line_tokens = assign_string_tokenize(line);
				assign(vars, line_tokens, ASSIGNSTR);
				break;
			}
			case DISPQUOTE: {
				line_tokens = tokenize_display(line);
				display(vars, line_tokens, DISPQUOTE);
				break;
			}
			case DISPSTRNUM: {
				line_tokens = tokenize(line);
				display(vars, line_tokens, DISPSTRNUM);
				break;
			}
			case COMMENT: {
				//comment
				//do nothing
				break;
			}
			case WHITESPACE: {
				//whitespace
				//do nothing
				break;
			}
			default: {
				printf("Error: line %d\n", lineNum);    //syntax error
				break;
			}
		}
		free(line->str);
		free(line);
		/*if (lineNum > 1) {
			int i;
			for (i = 0; line_tokens[i] != NULL; i++) {
				free_string(line_tokens[i]);
				free(line_tokens[i]);
			}
			free(line_tokens[i + 1]);
			free(line_tokens);
		}*/
		lineNum++;
	}
	//free tokens
	fclose(file);  
}