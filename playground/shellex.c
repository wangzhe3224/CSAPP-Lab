#include "csapp.h"
#define MAXARGS  128
#define	MAXLINE	 8192  /* max text line length */

/* Functions prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int buildlin_command(char **argv);

int main() 
{
    char cmdline[MAXLINE];

    while(1) {
        printf("tell me: ");
        Fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            exit(0);
        eval(cmdline);
    }
}

void eval(char *cmdline) 
{
    char *argv[MAXARGS];  /* Argument list for execve() */
    char buf[MAXLINE];     
    int bg;
    pid_t pid;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return;  // empty line ignore

    if (!buildlin_command(argv)) {
        if ((pid = Fork()) == 0){
            // child
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

        // parent wait for foreground task to finish
        if (!bg) {
            int status;
            if (waitpid(pid, &status, 0) < 0) 
                unix_error("waitfg: waitpid error.\n");
        }
        else {
            // background task, just return the pid id and commond
            printf("%d %s", pid, cmdline);
        }
    }

    return;
}

int buildlin_command(char **argv)
{
    if (!strcmp(argv[0], "quit"))
        exit(0);
    if (!strcmp(argv[0], "&"))
        return 1;
    return 0;
}

int parseline(char *buf, char **argv) 
{
    char *delim;
    int argc;  // number of arguments
    int bg;

    buf[strlen(buf) - 1] = ' ';  // remove trailing \n with space
    while (*buf && (*buf == ' '))  // ignore leading spaces... 
        buf++;

    argc = 0;
    while ((delim = strchr(buf, ' '))) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' '))  // ignore leading spaces...
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0)   // blank line
        return 1;

    if ((bg = (*argv[argc-1] == '&')) != 0)
        argv[--argc] = NULL;

    return bg;
}
