//COP4610
//Project 1 Starter Code
//example code for initial parsing

//*** if any problems are found with this code,
//*** please report them to the TA

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>


int numCommands = 0;
typedef struct
{
	char** tokens;
	int numTokens;
} instruction;

typedef struct
{
    char* cmd;
    int pid;
} process;

typedef struct
{
    process *array;
    size_t used;
    size_t size;
} Array;

void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);
void expandEnv(instruction* instr_ptr);
char* pathResolution(instruction* instr_ptr);
char* findExec(char *cmd);
void execute(char* path, instruction* instr_ptr);
void printPrompt();
void piping(instruction* instr_ptr);
void ioRedirection(instruction* instr_ptr);
void builtIns(instruction* instr_ptr);
char* shortcutRes(char* instr_ptr);
int fileExists(const char *path);
int insert(process process1);
void checkProcesses();
void initializeProcess();
void executeBg(instruction* instr_ptr, int i, char* path);

process processes[100];


int main() {
//    initializeProcess();
	char* token = NULL;
	char* temp = NULL;
	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;

	while (1) {
        printPrompt();
		// loop reads character sequences separated by whitespace
        //scans for next token and allocates token var to size of scanned token

        // loop reads character sequences separated by whitespace
		do {
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);
			temp = (char *) malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;
			for (i = 0; i < strlen(token); i++) {
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
					if (i - start > 0) {
						memcpy(temp, token + start, i - start);
						temp[i - start] = '\0';
						addToken(&instr, temp);

					}

					char specialChar[2];
					specialChar[0] = token[i];
					specialChar[1] = '\0';

					addToken(&instr, specialChar);

					start = i + 1;

				}
			}

			if (start < strlen(token)) {
				memcpy(temp, token + start, strlen(token) - start);
				temp[i - start] = '\0';
				addToken(&instr, temp);
			}

			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;
		} while ('\n' != getchar());    //until end of line is reached

		char* execPath = (char*) malloc(1000);

		numCommands++;
		addNull(&instr);
		expandEnv(&instr);
		//shortcutRes(&instr);
		//ioRedirection(&instr);
		execPath = pathResolution(&instr);
		//printf("pre exec");
		execute(execPath, &instr);
        piping(&instr);
//        execPath = pathResolution(&instr);
//		printf("pre exec");
//		execute(execPath, &instr);
        builtIns(&instr);
		printTokens(&instr);
		clearInstruction(&instr);
//		checkProcesses();
	}

	return 0;
}

//reallocates instruction array to hold another token
//allocates for new token within instruction array
void addToken(instruction* instr_ptr, char* tok)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**) malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;
	instr_ptr->numTokens++;
}

void printTokens(instruction* instr_ptr)
{
	int i;
	printf("Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
			printf("%s\n", (instr_ptr->tokens)[i]);
	}
}

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);

	free(instr_ptr->tokens);

	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
}

// If file or directory exists, return 1; otherwise 0.
int fileExists(const char *path)
{
    if(access(path, F_OK) == -1)
        return 0;

    return 1;
}

char* shortcutRes(char* instr_ptr)
{
    char* envvar = (char*) malloc(1000);
	char* path_name = (char*) malloc(1000);
    char* path = (char*) malloc(1000);
    char *parent = (char*)malloc(1000);
    char *cwd = (char*) malloc(1000);

    // Copies input into path_name
    strcpy(path_name, instr_ptr);

    if (instr_ptr[0] != '/') {
        // If relative path:
        path = strtok(path_name, " /");
        while(path!=NULL) {
            if (strcmp(path, "~") == 0) {
                strcpy(envvar, getenv("HOME"));
                // concatenates path after first ~/
                while (path != NULL) {
                    if (strcmp(path, "~") != 0) {
                        strcat(envvar, "/");
                        strcat(envvar, path);
                    }
                    path = strtok(NULL, " /");
                }
                strcpy(cwd, envvar);
            }
                // Expands to parent
            else if (strcmp(path, "..") == 0) {
                chdir("..");
                cwd = getcwd(parent, 100);

                // concatenates parents with input
                if (strcmp(path, "..") != 0) {
                    strcat(cwd, "/");
                    strcat(cwd, path);
                }
                path = strtok(NULL, " /");
                strcpy(envvar, getenv("PWD"));
            }

                // Expands to PWD
            else if (strcmp(path, ".") == 0) {
                strcpy(envvar, getenv("PWD"));

                while (path != NULL) {
                    if (strcmp(path, ".") != 0) {
                        strcat(envvar, "/");
                        strcat(envvar, path);
                    }
                    path = strtok(NULL, " /");
                }

                strcpy(cwd, envvar);
            }
        }
        strcpy(envvar, getenv("PWD"));

        // checks if file or directory (at envvar) exists
        if (chdir(cwd) != 0)
        {
            perror(cwd);
        }
        else
        {
            printf("-Bash: %s is a directory\n", cwd);
        }
        chdir(envvar);
    }
    else
    {
        path = path_name;

        // Expands to root directory
        if (instr_ptr[0] == '/') {
            char *root = (char*)malloc(1000);
            chdir("/");
            cwd = getcwd(root, 100);
            path = strtok(path_name, " /");

            while (path != NULL)
            {
                strcat(cwd, path);
                strcat(cwd, "/");
                path = strtok(NULL, " /");
            }

            // If file/dir doesn't exist, throw error
            if(chdir(cwd) != 0)
            {
                perror(cwd);
            }

            else
            {
                printf("-bash: %s\n", cwd);
            }

        }
    }

    return cwd;
}

void expandEnv(instruction* instr_ptr)
{
    int numTok = instr_ptr->numTokens-1;
    int i;
    for (i = 0; i < numTok; i++)
    {
        if (instr_ptr->tokens[i][0] == '$')
        {
            char* env = (char*) malloc(1000);
            instr_ptr->tokens[i]++;
            memcpy(env, getenv(instr_ptr->tokens[i]), 100);
            instr_ptr->tokens[i] = env;
        }
    }
}

// Check for redirection
void ioRedirection(instruction* instr_ptr)
{
    int numTok = instr_ptr->numTokens -1;
    int i;
    int fd = 0;
    char* path = (char*)malloc(1000);
    char *path_name = (char*)malloc(1000);
    char *path_res = (char*)malloc(1000);
    char *io = (char*)malloc(1000);
    char **new_arr = (char**)malloc(1000);

    for( i = 0; i < numTok; i++)
    {
        if ((strcmp(instr_ptr->tokens[i],  ">") == 0)
        || (strcmp(instr_ptr->tokens[i], "<") == 0)) {

            // Check if first or last character
            if( i == 0 || i == numTok-1 ) {
                printf("Missing name for redirect.\n");
            }

            else {
                if (strcmp(instr_ptr->tokens[i],"<") == 0) {
                    int index = i;

                    strcpy(path, instr_ptr->tokens[i + 1]);

                    io = strtok(path_name, " <");

                    for (int j = 0; j < index; j++) {
                        strcpy(path_name, instr_ptr->tokens[j]);

                        io = strtok(path_name, " <");

                        while (io != NULL) {
                            new_arr[j] = io;
                            io = strtok(NULL, " <");
                        }
                    }

                    fd = open(path, O_RDONLY);

                    if (fd == -1) {
                        perror(path);
                        exit(EXIT_FAILURE);
                    }

                    pid_t child_pid;
                    int stat_loc;
                    child_pid = fork();

                    if (child_pid == 0) {
                        close(STDIN_FILENO);
                        dup(fd);
                        close(fd);

                        path_res = pathResolution(instr_ptr);

                        execv(path_res, new_arr);

                    }
                    else {
                        waitpid(child_pid, &stat_loc, WUNTRACED);
                        close(fd);
                    }
                }

                else if (strcmp(instr_ptr->tokens[i],">") == 0) {
                    int index = i;

                    strcpy(path, instr_ptr->tokens[i+1]);

                    io = strtok(path_name, " >");

                    for (int j = 0; j < index; j++)
                    {
                        strcpy(path_name, instr_ptr->tokens[j]);

                        io = strtok(path_name, " >");

                        while(io != NULL)
                        {
                            new_arr[j] = io;
                            io = strtok(NULL, " >");
                        }
                    }

                    fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR |
                    S_IRGRP | S_IWGRP | S_IWUSR);

                    if(fd == -1)
                    {
                        perror(path);
                        exit(EXIT_FAILURE);
                    }

                    pid_t child_pid;
                    int stat_loc;
                    child_pid = fork();

                    if (child_pid == 0) {
                        path = pathResolution(instr_ptr);

                        close(STDOUT_FILENO);
                        dup(fd);
                        close(fd);

                        path_res = pathResolution(instr_ptr);

                        execv(path_res, new_arr);

                    }

                    else {
                        waitpid(child_pid, &stat_loc, WUNTRACED);
                        close(fd);
                    }

                }
            }
        }
    }
}

void builtIns(instruction* instr_ptr) {
    int numTok = instr_ptr->numTokens - 1;
    int i;

    if (strcmp((instr_ptr->tokens)[0], "echo") == 0) {

        char envVar[300];
        for (i = 1; i < numTok; i++) {
            // Environment vars
            if (instr_ptr->tokens[i][0] == '$') {
                strcpy(envVar, getenv((instr_ptr->tokens)[i]));

                if (envVar != NULL)
                    printf("%s\n", envVar);

                else {
                    printf("%s\n", "Invalid or NULL environment variable");
                    return;
                }
            }

                // Output without modification
            else {
                printf("%s ", (instr_ptr->tokens)[i]);
            }
        }
        printf("%s", "\n");
    } else if (strcmp((instr_ptr->tokens)[0], "exit") == 0) {
        printf("%s \n\t%s %d\n", "Exiting now!", "Commands executed:", numCommands);
        exit(0);
    } else if (strcmp((instr_ptr->tokens)[0], "jobs") == 0) {
        int i;
        for (i = 0; i < 100; i++) {
            if (strcmp(processes[i].cmd, "*") != 0) {
                printf("[%d]+ [%d] [%s]", i, processes[i].pid, processes[i].cmd);
            }
        }
    } else if (strcmp((instr_ptr->tokens)[0], "cd") == 0) {
        //convert path to absolute
        //call chdir on the path and update pwd
        //copy path to pwd using setenv
        //also check that entered directory is valid
        struct stat buf;

        if (instr_ptr->tokens[1][0] != '/') { //path is relative
            //printf("first char is %s", instr_ptr->tokens[1][0]);
            char *temp = (char *) malloc(strlen(getenv("PWD")));
            memcpy(temp, getenv("PWD"), strlen(getenv("PWD")));
            strcat(temp, "/");
            strcat(temp, instr_ptr->tokens[1]);
            //printf("%s", temp);
            if (stat(temp, &buf) == 0 && S_ISDIR(buf.st_mode)) {
                printf("relative directory is %s \n", temp);
                chdir(temp);
                setenv("PWD", temp, 1);
            } else {
                printf("Directory doesn't exist\n");
                return;
            }

        } else {
            if (stat(instr_ptr->tokens[1], &buf) == 0 && S_ISDIR(buf.st_mode)) {
                printf("directory is %s\n", instr_ptr->tokens[1]);
                chdir(instr_ptr->tokens[1]);
                setenv("PWD", instr_ptr->tokens[1], 1);
            } else {
                printf("Directory doesn't exist \n");
                return;
            }
        }
    }
}

void piping(instruction* instr_ptr) {
    int numTok = instr_ptr->numTokens - 1;
    int i;
    int stat_loc;
    char *path_res = (char *) malloc(1000);
    char *path_name = (char *) malloc(1000);
    char *io = (char *) malloc(1000);
    char **new_arr = (char **) malloc(1000);

    char *path = (char *) malloc(1000);

    char *path_res2 = (char *) malloc(1000);
    char *path_name2 = (char *) malloc(1000);
    char *io2 = (char *) malloc(1000);
    char **new_arr2 = (char **) malloc(1000);

    for (i = 0; i < numTok; i++) {
        if (instr_ptr->tokens[i][0] == '|') {
            if (i == 0 || i == numTok - 1) {
                printf("Invalid null command.\n");
                return;
            } else {
                if (strcmp(instr_ptr->tokens[i], "|") == 0) {
                    int index = i;
                    // solve for cmd 1
                    for (int j = 0; j < index; j++) {

                        strcpy(path_name, instr_ptr->tokens[j]);
                        io = strtok(path_name, " |");

                        while (io != NULL) {
                            new_arr[j] = io;
                            io = strtok(NULL, " |");
                        }
                    }

                    // solve for cmd 2
                    int l = 0;
                    for (int k = index + 1; k < numTok; k++) {
                        strcpy(path_name2, instr_ptr->tokens[k]);
                        io2 = strtok(path_name2, " |");

                        while (io2 != NULL) {
                            new_arr2[l] = io2;
                            io2 = strtok(NULL, " |");
                            l++;
                        }
                    }

                    path_res2 = findExec(new_arr2[0]);

                    pid_t child1_pid;
                    pid_t child2_pid;

                    child1_pid = fork();
                    int fd[2];

                    if (child1_pid < 0) {
                        printf("\nCould not fork.");
                        return;
                    }

                    if (child1_pid == 0) {

                        close(STDOUT_FILENO);
                        dup(fd[1]);
                        close(fd[0]);
                        close(fd[1]);
                        path_res = pathResolution(instr_ptr);
                        execv(path_res, new_arr);
                    }
                    else {
                        close(STDIN_FILENO);
                        dup(fd[0]);
                        close(fd[0]);
                        close(fd[1]);

                        printf("PATH: %s\n", path_res2);
                        execv(path_res, new_arr);
                        waitpid(child2_pid, &stat_loc, WUNTRACED);
                        }
                    }
                }
            }
        }
    }




    void printPrompt()
{
    char user[30];
    char machine[80];
    char pwd[100];

    strcpy(user, getenv("USER"));
    strcpy(machine, getenv("MACHINE"));
    strcpy(pwd, getenv("PWD"));

    printf("%s@%s:%s> ", user, machine, pwd);

}

char* findExec(char *cmd)
{
    char* temp = (char*)malloc(1000);
    char* pRes;
    int statReturn;
    struct stat stats;
    char* path = (char*)malloc(1000);

    memcpy(path, getenv("PATH"), strlen(getenv("PATH")));

    pRes = strtok(path,":");
    while( pRes != NULL ) {
        strcpy(temp, pRes);
        strcat(temp, "/");

        char *temp2 = (char *) malloc(strlen(getenv("PATH")));

        strcat(temp2, temp);

        //printf("Before strcat token to temp2\n");
        strcat(temp2, cmd);
        //printf("After strcat token to temp2\n");

        //printf("After strcat\n");
        statReturn = stat(temp2, &stats);
        //printf("temp2 = %s \n", temp2);

        if (statReturn == 0) {
            strcat(temp, cmd);
//            printf("temp: %s\n", temp);
            return temp;
        }

        pRes = strtok(NULL, ":");
    }
}

char* pathResolution(instruction* instr_ptr)
{
	int i;
	int statReturn;
	int numTok = (instr_ptr->numTokens) -  1;
	char *ptr, *pRes, *path, *isAbsolute, *temp = (char*) malloc(strlen(getenv("PATH")));
	struct stat stats;
    int tokIndex = 0;


    path = (char*) malloc(strlen(getenv("PATH")));
	memcpy(path, getenv("PATH"), strlen(getenv("PATH")));

	isAbsolute = strchr((instr_ptr->tokens)[0], '/');

	if(strcmp(instr_ptr->tokens[0], "&") == 0){
	    tokIndex = 1;
	}

	if ( isAbsolute == NULL )
	{
		// Prefix with location in the path and search for file existence
		// The first file in the concatenated path list to exist is the path of the command
		// If none of the files exist, signal an error
		pRes = strtok(path,":");
//		printf("%s\n", pRes);

		while( pRes != NULL )
		{
			strcpy(temp, pRes);
			strcat(temp, "/");

			char* temp2 = (char*) malloc(strlen(getenv("PATH")));
			strcat(temp2, temp);

			//printf("Before strcat token to temp2\n");
			strcat(temp2, instr_ptr->tokens[tokIndex]);
			//printf("After strcat token to temp2\n");

			//printf("After strcat\n");
			statReturn = stat(temp2, &stats);
            //printf("temp2 = %s \n", temp2);
            if ( statReturn  == 0 )
            {
                strcat(temp, instr_ptr->tokens[0]);
//                printf("PATH RES: %s\n", temp);
                return temp;
            }

            pRes = strtok(NULL, ":");
            //free(temp);
            //printf("After strtok %s\n", pRes);
        }
    } else {
        // Handle as Shortcut Resolution
        char* path_res;//= shortcutRes(instr_ptr->tokens[0]);
//        printf("PATH RES: %s\n", path_res);

        return path_res;

    }

	// Invalid command or file
	if ( statReturn == -1 )
	{
		printf("Failure command '%s' not found\n", instr_ptr->tokens[tokIndex]);
		return;
	}

}

void execute(char* path, instruction* instr_ptr) {
    pid_t child_pid;
    int stat_loc;
    int i;
    if(instr_ptr->numTokens > 2){
        //printf("Numtokens = %d", instr_ptr->numTokens);
        for(i = 1; i < instr_ptr->numTokens; i++){

            if(strcmp(instr_ptr->tokens[i], "&") == 0){
                printf("i = %d\n", i);
                executeBg(instr_ptr, i, path);
                //printf("We returnin\n");
                return;
            }
        }
    } else {
        child_pid = fork();
        if (child_pid == 0) {
            /* Never returns if the call is successful */
            execv(path, instr_ptr->tokens);
            printf("This won't be printed if execv is successul\n");
        } else {
            waitpid(child_pid, &stat_loc, WUNTRACED);
        }
    }
    }



int insert(process process1)
{
    int i;
    for(i = 0; i < 100; i++){
        if (strcmp(processes[i].cmd, "*") == 0){
            processes[i] = process1;
            return i;
        }
    }
    printf("No room in queue");
}

void checkProcesses()
{
    int i;
    int stat_loc;
    for(i = 0; i < 100; i++){
        if (processes[i].pid == waitpid(-1, &stat_loc, WUNTRACED)){
            printf("[%d]+ [%s]\n", i, processes[i].cmd);
            int c;
            for (c = i - 1; c < 100 - 1; c++)
            processes[c] = processes[c+1];
        }
    }
}

void initializeProcess(){
    int i;
    for (i = 0; i < 100; i++){
        strcpy(processes[i].cmd, "*");
    }
}

void executeBg(instruction* instr_ptr, int i, char* path){
    pid_t child_pid;
    int stat_loc;
    int size = sizeof(instr_ptr->tokens[0]);

    //printf("Path is: %s", path);
    child_pid = fork();
//    printf("%d", child_pid);
    if (child_pid == 0) {
        /* Never returns if the call is successful */
        int j;
        char** args = (char**)malloc(i+1 * sizeof(char*));
        for (j = 0; j < i; j++){
            args[j] = instr_ptr->tokens[i];
        }
        execv(path, args);
        // printf("This won't be printed if execv is successul\n");
    } else {
        //printf("Forked");
        process process1;

        int tokIndex = 0;
        if(strcmp(instr_ptr->tokens[0], "&") == 0){
            tokIndex = 1;
        }
        process1.cmd = malloc(sizeof(instr_ptr->tokens[tokIndex]));
        memcpy(process1.cmd, instr_ptr->tokens[tokIndex], sizeof(instr_ptr->tokens[tokIndex]));
        //process1.cmd = instr_ptr->tokens[0];
        process1.pid = child_pid;
        int index = insert(process1);
        printf("[%d] [%d] \n", index, process1.pid);
        printf("cmd is: %s\n", process1.cmd);
        waitpid(child_pid, &stat_loc, WNOHANG);
        return;
    }
}
