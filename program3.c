#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>


//limits
#define MAX_TOKENS 100
#define MAX_STRING_LEN 100

size_t MAX_LINE_LEN = 10000;
int token_count = 0;
int size = MAX_TOKENS;
char *this_token;
FILE *fp;
char **tokens;
char *line;

int check_sign(char** tokens[], int token_count, char *sign) {
  // check sign in given command
//  printf("Hiiii4\n");
  int sign_counter = 0;
  int i = 0;
  while(i<token_count)
  {
    if(strcmp(tokens[i],sign) == 0) {
      sign_counter = 1;
      break;
    }
    else{
      i++;
    }
  }
  return sign_counter;
}

void run_normal_command(char** tokens[])
{
//  printf("Inside run normal command\n");
  //printf("Running normal command\n");
  int pid= fork();              //create child

    if(pid==0){
        execvp(tokens[0],tokens);
        fprintf(stderr, "Child process could not do execvp\n");

    }else if (pid < 0)
    {
      printf("Creating new process failed.");
    }
    else{
        waitpid(pid, 0, 0);
        //printf("Child exited\n");
    }
}

int count_pipes(char** tokens[], int token_count)
{
  int pipe_count = 0;
  int i = 0;
  while(i<token_count)
  {
    if(strcmp(tokens[i],"|") == 0) {
      pipe_count ++;
      i++;
    }
    else{
      i++;
    }
  }

return pipe_count;
}

void input_redirection(char **tokens[], int token_count) {
//  printf("Inside input function\n");
	char *filename = tokens[token_count-1];
//  printf("File name retrived:%s\n",filename);

  	int stdin_dup = dup(STDIN_FILENO);
    //printf("Dup successful.\n");
  	// opening file in read only mode
  	int input_file = open(filename, O_RDONLY);
    //printf("file Opened\n");

  	if(input_file < 0) {
    fprintf(stderr, "Error: fail to open a new file %s.\n", filename);
  	} else {
    	dup2(input_file, STDIN_FILENO);
    //  printf("dup2 Compled\n");
    ////  printf("token_count%d\n",token_count);

    	int i = token_count - 2;
      //printf("i %d\n",i);
    //  printf("tokens[i]%s\n",tokens[i]);
      for(int i = token_count - 2; tokens[i] != NULL;i++)
      {
        tokens[i] = NULL;
      }
      //token_count = token_count - 2;
    	run_normal_command(tokens);


    	dup2(stdin_dup, STDIN_FILENO);
    //  printf("dup2 set to stdin\n");
      close(input_file);
    //  printf("File closed\n");
    }
}

void output_redirection(char **tokens[], int token_count) {
	  char *filename = tokens[token_count-1];
	int stdout_dup = dup(STDOUT_FILENO);

  int output_file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);


  if(output_file < 0) {
    fprintf(stderr, "Error: fail to open or create a new file %s.\n", filename);
  } else {

  	dup2(output_file, STDOUT_FILENO);

   	int i = token_count - 2;
    for(int i = token_count - 2; tokens[i] != NULL;i++)
    {
      tokens[i] = NULL;
    }

    run_normal_command(tokens);
    dup2(stdout_dup, STDOUT_FILENO);
   	close(output_file);

 	}
}

void execute_pipe_commands (char **tokens[], int token_count, int number_of_pipes) {
  int pid1;
  char **l_tokens[64];
  int l_token_count;
  int start;
  int previous_pipe_index = -1;
  int file_discriptors[2];
  int input_file_discriptor  = -1;
  int output_file_discriptor = -1;
  //printf("Pipe Count: %d\n",number_of_pipes);
  int number_of_commands = 0;

  for(int i = 0; i < token_count; i++) {
    if(i == token_count - 1 || strcmp(tokens[i], "|") == 0) {

      l_token_count = 0;

      if(previous_pipe_index <= 0) {

        start = 0;
        number_of_commands ++;

      } else {

        start = previous_pipe_index + 1;
        number_of_commands++;
      }

      if(i < token_count - 1) {

        previous_pipe_index = i;

        while(start < i) {
          l_tokens[l_token_count++] = tokens[start];
          start++;
        //  printf("Inside if while\n");
        }

        l_tokens[l_token_count] = NULL;

        pipe(file_discriptors);

        output_file_discriptor = file_discriptors[1];
      } else {

        while (start <= i) {
          l_tokens[l_token_count++] = tokens[start];
           start++;
        //   printf("Inside else while\n");
        }
        l_tokens[l_token_count] = NULL;

        output_file_discriptor = -1;
      }

      if (number_of_commands <= number_of_pipes && check_sign(l_tokens,l_token_count,">") == 1)
      {
        printf("Invalid combination of commands\n");
        return 0;
      }

      pid1 = fork();
      //printf("PID:%d\n",pid1 );
      if(pid1 < 0) {

        printf("Creating new process failed.");
      } else if(pid1 > 0) {
        waitpid(pid1, 0, 0);
      //  printf("pid Waiting\n");
        close(input_file_discriptor);
        close(output_file_discriptor);
        input_file_discriptor = file_discriptors[0];

      } else {
        //printf("Inside pid1 ==0\n");

        if(input_file_discriptor != -1 && input_file_discriptor != 0) {
          dup2(input_file_discriptor, STDIN_FILENO);
          close(input_file_discriptor);
        }

        if(output_file_discriptor != -1 && output_file_discriptor != 1) {
          dup2(output_file_discriptor, STDOUT_FILENO);
          close(output_file_discriptor);
        }


      //  run_normal_command(l_tokens);
      execute_commands(l_tokens,l_token_count);
      //run_normal_command(l_tokens);
        //printf("Commands executed\n");
        exit(0);
      }
    }
  }
}

void execute_commands (char **tokens[], int token_count) {

  if (tokens[0] != NULL && tokens[0] != "exit") {
    //printf("check sign%d\n",check_sign(tokens, "|"));
    if (check_sign(tokens, token_count, "|") == 1){
      //printf("Hiiii3\n");
      int number_of_pipes = count_pipes(tokens,token_count);
      execute_pipe_commands (tokens,token_count,number_of_pipes);
    } else if (check_sign(tokens,token_count, ">") == 1) {
      output_redirection(tokens,token_count);
    } else if (check_sign(tokens,token_count, "<") == 1){
      //printf("I was here. Token Count: %d\n", token_count);
      input_redirection(tokens,token_count);
    } else {
    //  printf("Hiiii2\n");
      run_normal_command(tokens);
    }
	}
  else
  {
    return 0;
  }
}

void initialize()
{

	// allocate space for the whole line
	assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

	// allocate space for individual tokens
	assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

	// open stdin as a file pointer
	assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);

}

void tokenize (char * string)
{
  token_count = 0;
	while ( (this_token = strsep( &string, " \t\v\f\n\r")) != NULL) {

		if (*this_token == '\0') continue;

		tokens[token_count] = this_token;

		//printf("Token %d: %s\n", token_count, tokens[token_count]);

		token_count++;

		// if there are more tokens than space ,reallocate more space
		if(token_count >= size){
			size*=2;

			assert ( (tokens = realloc(tokens, sizeof(char*) * size)) != NULL);
		}
	}
}

void read_command()
{

	// getline will reallocate if input exceeds max length
	assert( getline(&line, &MAX_LINE_LEN, fp) > -1);

	//printf("Shell read this line: %s\n", line);

	tokenize(line);
}

int run_command() {

	if (strcmp(tokens[0], "exit" ) == 0)
		exit(0);
  else{
    execute_commands(tokens,token_count);
  }
	return 0;
}

int main()
{
  initialize();

	while(1) {
    for(int i = 0; tokens[i] != NULL; i++)
    {
      tokens[i] = NULL;
    }
		printf("\nmysh> ");
		read_command();
    run_command();
	}
	return 0;
}
