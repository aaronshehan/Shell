#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define CHUNKSIZE 10
#define CMDLEN 1000
#define ASSIGNOP 1
#define ASSIGNNUM 2
#define ASSIGNSTR 3
#define DISPQUOTE 4
#define DISPSTRNUM 5
#define COMMENT 6
#define WHITESPACE 7

struct String {
	char* str;
	size_t len;
} typedef String;

struct Token {
	String* token;
	String* type;
	String* val;
} typedef Token;

String* string_init();

String* new_string(char* s);

void free_string(String* string);

void free_token(Token* token);

Token* new_token(char* t);

String* getLine();

String* substring(char* string, int begin, int end);

String** tokenize(String* line);

int analyze_line(char* line);

void update_val(Token**, String* var, char* newval);

void update_type(Token** vars, String* var, char* type);

char* get_value(Token** vars, String* var);

char* get_type(Token** vars, String* var);

int in_set(Token** vars, String* token);

void set_insert(Token** vars, String* token);

String** tokenize_display(String* line);

String** assign_string_tokenize(String* line);

void display(Token** vars, String** tokens, int type);

void assign(Token** vars, String** tokens, int type);

void bash2(char* filename);