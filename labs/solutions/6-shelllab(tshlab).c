/*
 * tsh - A tiny shell program with job control
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdarg.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Parsing states */
#define ST_NORMAL 0x0  /* next token is an argument */
#define ST_INFILE 0x1  /* next token is the input file */
#define ST_OUTFILE 0x2 /* next token is the output file */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t
{                          /* The job struct */
    pid_t pid;             /* job PID */
    int jid;               /* job ID [1, 2, ...] */
    int state;             /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /* command line */
};
struct job_t job_list[MAXJOBS]; /* The job list */

struct cmdline_tokens
{
    int argc;            /* Number of arguments */
    char *argv[MAXARGS]; /* The arguments list */
    char *infile;        /* The input file */
    char *outfile;       /* The output file */
    enum builtins_t
    { /* Indicates if argv[0] is a builtin command */
      BUILTIN_NONE,
      BUILTIN_QUIT,
      BUILTIN_JOBS,
      BUILTIN_BG,
      BUILTIN_FG,
      BUILTIN_KILL,
      BUILTIN_NOHUP
    } builtins;
};

/* End global variables */

/* Function prototypes */
void eval(char *cmdline);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, struct cmdline_tokens *tok);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *job_list);
int maxjid(struct job_t *job_list);
int addjob(struct job_t *job_list, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *job_list, pid_t pid);
pid_t fgpid(struct job_t *job_list);
struct job_t *getjobpid(struct job_t *job_list, pid_t pid);
struct job_t *getjobjid(struct job_t *job_list, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *job_list, int output_fd);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
ssize_t sio_puts(char s[]);
ssize_t sio_putl(long v);
ssize_t sio_put(const char *fmt, ...);
void sio_error(char s[]);

typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/* Wrappers for Unix signal functions */
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void Sigemptyset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigaddset(sigset_t *set, int signum);
int Sigsuspend(const sigset_t *set);

/* Wrappers for Unix process control functions */
pid_t Fork(void);
void Execve(const char *filename, char *const argv[], char *const envp[]);
void Kill(pid_t pid, int signum);
void Setpgid(pid_t pid, pid_t pgid);

/* Wrappers for Unix I/O routines */
int Open(char *pathname, int flags, mode_t mode);
void Close(int fd);
int Dup(int fd);
int Dup2(int fd1, int fd2);

/* Built-in commands */
void buildin_quit();
void buildin_jobs(int output);
void buildin_bg(char *id);
void buildin_fg(char *id);
void buildin_nohup(char *cmdline, int argc);
void buildin_kill(char *id);


/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE]; /* cmdline for fgets */
    int emit_prompt = 1;   /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF)
    {
        switch (c)
        {
        case 'h': /* print help message */
            usage();
            break;
        case 'v': /* emit additional diagnostic info */
            verbose = 1;
            break;
        case 'p':            /* don't print a prompt */
            emit_prompt = 0; /* handy for automatic testing */
            break;
        default:
            usage();
        }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */
    Signal(SIGTTIN, SIG_IGN);
    Signal(SIGTTOU, SIG_IGN);

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(job_list);

    /* Execute the shell's read/eval loop */
    while (1)
    {

        if (emit_prompt)
        {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin))
        {
            /* End of file (ctrl-d) */
            printf("\n");
            fflush(stdout);
            fflush(stderr);
            exit(0);
        }

        /* Remove the trailing newline */
        cmdline[strlen(cmdline) - 1] = '\0';

        /* Evaluate the command line */
        eval(cmdline);

        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(char *cmdline)
{
    int bg; /* should the job run in bg or fg? */
    struct cmdline_tokens tok;

    /* Parse command line */
    bg = parseline(cmdline, &tok);
    int state = bg ? BG : FG;

    if (bg == -1) /* parsing error */
        return;
    if (tok.argv[0] == NULL) /* ignore empty lines */
        return;


    /* I/O setting */
    int infile_id = STDIN_FILENO;
    int outfile_id = STDOUT_FILENO;
    int tmpstdin = Dup(STDIN_FILENO);
    int tmpstdout = Dup(STDOUT_FILENO);
    if (tok.infile != NULL)
    {
        infile_id = Open(tok.infile, O_RDONLY, 0);
        if (infile_id < 0)
            return;
        Dup2(infile_id, STDIN_FILENO);
    }
    if (tok.outfile != NULL)
    {
        outfile_id = Open(tok.outfile, O_WRONLY, 0);
        Dup2(outfile_id, STDOUT_FILENO);
    }


    /* solve build-in commands */
    switch (tok.builtins)
    {
    case BUILTIN_QUIT:
        buildin_quit();
        break;
    case BUILTIN_JOBS:
        buildin_jobs(outfile_id);
        break;
    case BUILTIN_BG:
        buildin_bg(tok.argv[1]);
        break;
    case BUILTIN_FG:
        buildin_fg(tok.argv[1]);
        break;
    case BUILTIN_NOHUP:
        buildin_nohup(cmdline, tok.argc);
        break;
    case BUILTIN_KILL:
        buildin_kill(tok.argv[1]);
        break;
    default:
        break;
    }
    if (tok.builtins != BUILTIN_NONE)
    {
        /* I/O setting for built-in commands */
        if (tok.infile != NULL)
        {
            Dup2(tmpstdin, STDIN_FILENO);
            Close(infile_id);
        }
        if (tok.outfile != NULL)
        {
            Dup2(tmpstdout, STDOUT_FILENO);
            Close(outfile_id);
        }
        Close(tmpstdin);
        Close(tmpstdout);
        return;
    }


    /*
     * mask_all: block all signals
     * mask_empty: block no signal
     * mask_one: block SIGCHLD signal
     * mask_now: block SIGCHLD, SIGINT, SIGTSTP signals
     * mask_prev: store the previous signals been blocked
     */
    sigset_t mask_all, mask_empty, mask_one, mask_now, mask_prev;
    Sigemptyset(&mask_empty);
    Sigemptyset(&mask_one);
    Sigemptyset(&mask_now);
    Sigfillset(&mask_all);
    Sigfillset(&mask_prev);
    Sigaddset(&mask_one, SIGCHLD);
    Sigaddset(&mask_now, SIGCHLD);
    Sigaddset(&mask_now, SIGINT);
    Sigaddset(&mask_now, SIGTSTP);


    pid_t pid;
    Sigprocmask(SIG_BLOCK, &mask_now, &mask_prev);


    /* child process */
    if ((pid = Fork()) == 0)
    {
        Setpgid(0, 0);
        Sigprocmask(SIG_SETMASK, &mask_prev, NULL);
        Execve(tok.argv[0], tok.argv, environ);

        /* close files when an error occurs in Execve */
        if (tok.infile != NULL)
            Close(infile_id);
        if (tok.outfile != NULL)
            Close(outfile_id);
        _exit(0);
    }


    /* parent process */

    /* I/O settings and close files*/
    if (tok.infile != NULL)
    {
        Dup2(tmpstdin, STDIN_FILENO);
        Close(infile_id);
    }
    if (tok.outfile != NULL)
    {
        Dup2(tmpstdout, STDOUT_FILENO);
        Close(outfile_id);
    }
    Close(tmpstdin);
    Close(tmpstdout);

    if (state == FG) /* frontground */
    {
        Sigprocmask(SIG_SETMASK, &mask_all, NULL);

        addjob(job_list, pid, state, cmdline);

        /* Waiting for the foreground job to terminate */
        while (fgpid(job_list) == pid)
            Sigsuspend(&mask_empty);

        Sigprocmask(SIG_SETMASK, &mask_prev, NULL);
    }
    else /* background */
    {
        Sigprocmask(SIG_BLOCK, &mask_all, NULL);
        addjob(job_list, pid, state, cmdline);
        Sigprocmask(SIG_SETMASK, &mask_prev, NULL);

        sio_put("[%d] (%d) ", pid2jid(pid), pid);
        sio_puts(cmdline);
        sio_puts("\n");
    }
    return;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Parameters:
 *   cmdline:  The command line, in the form:
 *
 *                command [arguments...] [< infile] [> oufile] [&]
 *
 *   tok:      Pointer to a cmdline_tokens structure. The elements of this
 *             structure will be populated with the parsed tokens. Characters
 *             enclosed in single or double quotes are treated as a single
 *             argument.
 * Returns:
 *   1:        if the user has requested a BG job
 *   0:        if the user has requested a FG job
 *  -1:        if cmdline is incorrectly formatted
 *
 * Note:       The string elements of tok (e.g., argv[], infile, outfile)
 *             are statically allocated inside parseline() and will be
 *             overwritten the next time this function is invoked.
 */
int parseline(const char *cmdline, struct cmdline_tokens *tok)
{

    static char array[MAXLINE];        /* holds local copy of command line */
    const char delims[10] = " \t\r\n"; /* argument delimiters (white-space) */
    char *buf = array;                 /* ptr that traverses command line */
    char *next;                        /* ptr to the end of the current arg */
    char *endbuf;                      /* ptr to end of cmdline string */
    int is_bg;                         /* background job? */

    int parsing_state; /* indicates if the next token is the
                          input or output file */

    if (cmdline == NULL)
    {
        (void)fprintf(stderr, "Error: command line is NULL\n");
        return -1;
    }

    (void)strncpy(buf, cmdline, MAXLINE);
    endbuf = buf + strlen(buf);

    tok->infile = NULL;
    tok->outfile = NULL;

    /* Build the argv list */
    parsing_state = ST_NORMAL;
    tok->argc = 0;

    while (buf < endbuf)
    {
        /* Skip the white-spaces */
        buf += strspn(buf, delims);
        if (buf >= endbuf)
            break;

        /* Check for I/O redirection specifiers */
        if (*buf == '<')
        {
            if (tok->infile)
            {
                (void)fprintf(stderr, "Error: Ambiguous I/O redirection\n");
                return -1;
            }
            parsing_state |= ST_INFILE;
            buf++;
            continue;
        }
        if (*buf == '>')
        {
            if (tok->outfile)
            {
                (void)fprintf(stderr, "Error: Ambiguous I/O redirection\n");
                return -1;
            }
            parsing_state |= ST_OUTFILE;
            buf++;
            continue;
        }

        if (*buf == '\'' || *buf == '\"')
        {
            /* Detect quoted tokens */
            buf++;
            next = strchr(buf, *(buf - 1));
        }
        else
        {
            /* Find next delimiter */
            next = buf + strcspn(buf, delims);
        }

        if (next == NULL)
        {
            /* Returned by strchr(); this means that the closing
               quote was not found. */
            (void)fprintf(stderr, "Error: unmatched %c.\n", *(buf - 1));
            return -1;
        }

        /* Terminate the token */
        *next = '\0';

        /* Record the token as either the next argument or the i/o file */
        switch (parsing_state)
        {
        case ST_NORMAL:
            tok->argv[tok->argc++] = buf;
            break;
        case ST_INFILE:
            tok->infile = buf;
            break;
        case ST_OUTFILE:
            tok->outfile = buf;
            break;
        default:
            (void)fprintf(stderr, "Error: Ambiguous I/O redirection\n");
            return -1;
        }
        parsing_state = ST_NORMAL;

        /* Check if argv is full */
        if (tok->argc >= MAXARGS - 1)
            break;

        buf = next + 1;
    }

    if (parsing_state != ST_NORMAL)
    {
        (void)fprintf(stderr,
                      "Error: must provide file name for redirection\n");
        return -1;
    }

    /* The argument list must end with a NULL pointer */
    tok->argv[tok->argc] = NULL;

    if (tok->argc == 0) /* ignore blank line */
        return 1;

    if (!strcmp(tok->argv[0], "quit"))
    { /* quit command */
        tok->builtins = BUILTIN_QUIT;
    }
    else if (!strcmp(tok->argv[0], "jobs"))
    { /* jobs command */
        tok->builtins = BUILTIN_JOBS;
    }
    else if (!strcmp(tok->argv[0], "bg"))
    { /* bg command */
        tok->builtins = BUILTIN_BG;
    }
    else if (!strcmp(tok->argv[0], "fg"))
    { /* fg command */
        tok->builtins = BUILTIN_FG;
    }
    else if (!strcmp(tok->argv[0], "kill"))
    { /* kill command */
        tok->builtins = BUILTIN_KILL;
    }
    else if (!strcmp(tok->argv[0], "nohup"))
    { /* kill command */
        tok->builtins = BUILTIN_NOHUP;
    }
    else
    {
        tok->builtins = BUILTIN_NONE;
    }

    /* Should the job run in the background? */
    if ((is_bg = (*tok->argv[tok->argc - 1] == '&')) != 0)
        tok->argv[--tok->argc] = NULL;

    return is_bg;
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP, SIGTSTP, SIGTTIN or SIGTTOU signal. The
 *     handler reaps all available zombie children, but doesn't wait
 *     for any other currently running children to terminate.
 */
void sigchld_handler(int sig)
{
    int olderrno = errno;

    sigset_t mask_all, prev;
    Sigfillset(&mask_all);
    Sigfillset(&prev);

    pid_t pid;
    int ret_state; /* the status of the return process */

    while ((pid = 
        waitpid(-1, &ret_state, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        Sigprocmask(SIG_BLOCK, &mask_all, &prev);
        if (WIFEXITED(ret_state)) /* process terminated by exit() or return */
            deletejob(job_list, pid);
        else if (WIFSTOPPED(ret_state)) /* process stopped now */
        {
            struct job_t *nowjob = getjobpid(job_list, pid);
            nowjob->state = ST;
            sio_put("Job [%d] (%d) stopped by signal %d\n", 
                pid2jid(pid), pid, WSTOPSIG(ret_state));
        }
        else if (WIFSIGNALED(ret_state)) /* process terminated by a signal */
        {
            sio_put("Job [%d] (%d) terminated by signal %d\n", 
                pid2jid(pid), pid, WTERMSIG(ret_state));
            deletejob(job_list, pid);
        }
        else if (WIFCONTINUED(ret_state)) /* process continued by SIGCONT */
        {
            struct job_t *nowjob = getjobpid(job_list, pid);
            nowjob->state = BG;
        }
        Sigprocmask(SIG_SETMASK, &prev, NULL);
    }

    errno = olderrno;

    return;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig)
{
    int olderrno = errno;

    sigset_t mask_all, mask_prev;
    Sigfillset(&mask_all);
    Sigfillset(&mask_prev);

    Sigprocmask(SIG_BLOCK, &mask_all, &mask_prev);
    pid_t pid = fgpid(job_list);
    Sigprocmask(SIG_BLOCK, &mask_prev, NULL);

    if (pid != 0)
        Kill(-pid, SIGINT);

    errno = olderrno;

    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig)
{
    int olderrno = errno;

    sigset_t mask_all, mask_prev;
    Sigfillset(&mask_all);
    Sigfillset(&mask_prev);

    Sigprocmask(SIG_BLOCK, &mask_all, &mask_prev);
    pid_t pid = fgpid(job_list);
    Sigprocmask(SIG_BLOCK, &mask_prev, NULL);

    if (pid != 0)
        Kill(-pid, SIGTSTP);

    errno = olderrno;

    return;
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    sio_error("Terminating after receipt of SIGQUIT signal\n");
}

/*********************
 * End signal handlers
 *********************/




/*********************
 * Built-in commands *
 *********************/

/* buildin_quit - built-in command "quit" */
void buildin_quit()
{
    _exit(0);
}

/*
 * buildin_jobs - built-in command "jobs"
 * Parameters:
 *      output: the I/O descriptor for output
 */
void buildin_jobs(int output)
{
    sigset_t mask_all, prev;
    Sigfillset(&mask_all);
    Sigfillset(&prev);

    Sigprocmask(SIG_BLOCK, &mask_all, &prev);
    listjobs(job_list, output);
    Sigprocmask(SIG_SETMASK, &prev, NULL);
}

/*
 * buildin_bg - built-in command "bg job"
 * Parameters:
 *      id: the argument denoted PID or %jobid
 */
void buildin_bg(char *id)
{
    if (id == NULL)
    {
        sio_puts("bg command requires PID or %jobid argument\n");
        return;
    }

    sigset_t mask_all, prev;
    Sigfillset(&mask_all);
    Sigfillset(&prev);

    Sigprocmask(SIG_BLOCK, &mask_all, &prev);

    struct job_t *nowjob = NULL;

    if (id[0] == '%')
    {
        nowjob = getjobjid(job_list, atoi(id + 1));
        if (nowjob == NULL)
        {
            sio_puts(id);
            sio_puts(": No such job\n");
            return;
        }
    }
    else if (id[0] >= '0' && id[0] <= '9')
    {
        nowjob = getjobpid(job_list, atoi(id));
        if (nowjob == NULL)
        {
            sio_put("(%d): No such process\n", atoi(id));
            return;
        }
    }
    else
    {
        sio_puts("bg command requires PID or %jobid argument\n");
        return;
    }

    Kill(-(nowjob->pid), SIGCONT);

    sio_put("[%d] (%d) ", nowjob->jid, nowjob->pid);
    sio_puts(nowjob->cmdline);
    sio_puts("\n");
    nowjob->state = BG;

    Sigprocmask(SIG_SETMASK, &prev, NULL);

    return;
}

/*
 * buildin_fg - built-in command "fg job"
 * Parameters:
 *      id: the argument denoted PID or %jobid
 */
void buildin_fg(char *id)
{
    if (id == NULL)
    {
        sio_puts("fg command requires PID or %jobid argument\n");
        return;
    }

    sigset_t mask_all, mask_empty, prev;
    Sigfillset(&mask_all);
    Sigfillset(&prev);
    Sigemptyset(&mask_empty);

    Sigprocmask(SIG_BLOCK, &mask_all, &prev);

    struct job_t *nowjob = NULL;

    if (id[0] == '%')
    {
        nowjob = getjobjid(job_list, atoi(id + 1));
        if (nowjob == NULL)
        {
            sio_puts(id);
            sio_puts(": No such job\n");
            return;
        }
    }
    else if (id[0] >= '0' && id[0] <= '9')
    {
        nowjob = getjobpid(job_list, atoi(id));
        if (nowjob == NULL)
        {
            sio_put("(%d): No such process\n", atoi(id));
            return;
        }
    }
    else
    {
        sio_puts("fg command requires PID or %jobid argument\n");
        return;
    }

    Kill(-(nowjob->pid), SIGCONT);

    /* Waiting for the foreground job to terminate */
    while (fgpid(job_list) == nowjob->pid)
        Sigsuspend(&mask_empty);

    Sigprocmask(SIG_SETMASK, &prev, NULL);

    return;
}

/*
 * buildin_nohup - built-in command "nohup [command]"
 * Parameters:
 *      cmdline: the command line user typed in
 *      argc: the total number of arguments user typed in
 */
void buildin_nohup(char *cmdline, int argc)
{
    if (argc == 1)
    {
        sio_puts("(null): Command not found\n");
    }

    /* remove the "nohup" argument */
    char newline[MAXLINE];
    strcpy(newline, strstr(cmdline, "nohup") + 5);

    sigset_t mask_one, prev;
    Sigemptyset(&mask_one);
    Sigfillset(&prev);
    Sigaddset(&mask_one, SIGHUP);

    Sigprocmask(SIG_BLOCK, &mask_one, &prev);

    eval(newline);

    Sigprocmask(SIG_SETMASK, &prev, NULL);

    return;
}

/*
 * buildin_kill - built-in command "kill job"
 * Parameters:
 *      id: the argument denoted PID or %jobid
 */
void buildin_kill(char *id)
{
    if (id == NULL)
    {
        sio_puts("kill command requires PID or %jobid argument");
        return;
    }

    sigset_t mask_all, prev;
    Sigfillset(&mask_all);
    Sigfillset(&prev);

    Sigprocmask(SIG_BLOCK, &mask_all, &prev);

    struct job_t *nowjob = NULL;

    int isgroup = 0;

    if (id[0] == '%') /* jid */
    {
        int jid = atoi(id + 1);
        if (jid < 0)
        {
            isgroup = 1;
            jid = -jid;
        }

        nowjob = getjobjid(job_list, jid);

        if (nowjob == NULL)
        {
            if (jid < 0)
            {
                sio_puts(id);
                sio_puts(": No such process group\n");
            }
            else
            {
                sio_puts(id);
                sio_puts(": No such job\n");
            }
            return;
        }
    }
    else /* pid */
    {
        pid_t pid = atoi(id);
        if (pid < 0)
        {
            isgroup = 1;
            pid = -pid;
        }

        nowjob = getjobpid(job_list, pid);

        if (nowjob == NULL)
        {
            sio_puts("(");
            sio_puts(id);
            sio_puts("): No such process");
            if (pid < 0)
                sio_puts(" group\n");
            else
                sio_puts("\n");
            return;
        }
    }

    Sigprocmask(SIG_SETMASK, &prev, NULL);

    if (isgroup)
        Kill(-(nowjob->pid), SIGTERM);
    else
        Kill(nowjob->pid, SIGTERM);

    return;
}




/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *job_list)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&job_list[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *job_list)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].jid > max)
            max = job_list[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *job_list, pid_t pid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (job_list[i].pid == 0)
        {
            job_list[i].pid = pid;
            job_list[i].state = state;
            job_list[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(job_list[i].cmdline, cmdline);
            if (verbose)
            {
                printf("Added job [%d] %d %s\n",
                       job_list[i].jid,
                       job_list[i].pid,
                       job_list[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *job_list, pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (job_list[i].pid == pid)
        {
            clearjob(&job_list[i]);
            nextjid = maxjid(job_list) + 1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *job_list)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].state == FG)
            return job_list[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t
    *
    getjobpid(struct job_t *job_list, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].pid == pid)
            return &job_list[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *job_list, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].jid == jid)
            return &job_list[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].pid == pid)
        {
            return job_list[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *job_list, int output_fd)
{
    int i;
    char buf[MAXLINE << 2];

    for (i = 0; i < MAXJOBS; i++)
    {
        memset(buf, '\0', MAXLINE);
        if (job_list[i].pid != 0)
        {
            sprintf(buf, "[%d] (%d) ", job_list[i].jid, job_list[i].pid);
            if (write(output_fd, buf, strlen(buf)) < 0)
            {
                fprintf(stderr, "Error writing to output file\n");
                exit(1);
            }
            memset(buf, '\0', MAXLINE);
            switch (job_list[i].state)
            {
            case BG:
                sprintf(buf, "Running    ");
                break;
            case FG:
                sprintf(buf, "Foreground ");
                break;
            case ST:
                sprintf(buf, "Stopped    ");
                break;
            default:
                sprintf(buf, "listjobs: Internal error: job[%d].state=%d ",
                        i, job_list[i].state);
            }
            if (write(output_fd, buf, strlen(buf)) < 0)
            {
                fprintf(stderr, "Error writing to output file\n");
                exit(1);
            }
            memset(buf, '\0', MAXLINE);
            sprintf(buf, "%s\n", job_list[i].cmdline);
            if (write(output_fd, buf, strlen(buf)) < 0)
            {
                fprintf(stderr, "Error writing to output file\n");
                exit(1);
            }
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/




/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/* Private sio_functions */
/* sio_reverse - Reverse a string (from K&R) */
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - Convert long to base b string (from K&R) */
static void sio_ltoa(long v, char s[], int b)
{
    int c, i = 0;

    do
    {
        s[i++] = ((c = (v % b)) < 10) ? c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);
    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - Return length of string (from K&R) */
static size_t sio_strlen(const char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}

/* sio_copy - Copy len chars from fmt to s (by Ding Rui) */
void sio_copy(char *s, const char *fmt, size_t len)
{
    if (!len)
        return;

    for (size_t i = 0; i < len; i++)
        s[i] = fmt[i];
}

/* Public Sio functions */
ssize_t sio_puts(char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s));
}

ssize_t sio_putl(long v) /* Put long */
{
    char s[128];

    sio_ltoa(v, s, 10); /* Based on K&R itoa() */
    return sio_puts(s);
}

ssize_t sio_put(const char *fmt, ...) // Put to the console. only understands %d
{
    va_list ap;
    char str[MAXLINE]; // formatted string
    char arg[128];
    const char *mess = "sio_put: Line too long!\n";
    int i = 0, j = 0;
    int sp = 0;
    int v;

    if (fmt == 0)
        return -1;

    va_start(ap, fmt);
    while (fmt[j])
    {
        if (fmt[j] != '%')
        {
            j++;
            continue;
        }

        sio_copy(str + sp, fmt + i, j - i);
        sp += j - i;

        switch (fmt[j + 1])
        {
        case 0:
            va_end(ap);
            if (sp >= MAXLINE)
            {
                write(STDOUT_FILENO, mess, sio_strlen(mess));
                return -1;
            }

            str[sp] = 0;
            return write(STDOUT_FILENO, str, sp);

        case 'd':
            v = va_arg(ap, int);
            sio_ltoa(v, arg, 10);
            sio_copy(str + sp, arg, sio_strlen(arg));
            sp += sio_strlen(arg);
            i = j + 2;
            j = i;
            break;

        case '%':
            sio_copy(str + sp, "%", 1);
            sp += 1;
            i = j + 2;
            j = i;
            break;

        default:
            sio_copy(str + sp, fmt + j, 2);
            sp += 2;
            i = j + 2;
            j = i;
            break;
        }
    } // end while

    sio_copy(str + sp, fmt + i, j - i);
    sp += j - i;

    va_end(ap);
    if (sp >= MAXLINE)
    {
        write(STDOUT_FILENO, mess, sio_strlen(mess));
        return -1;
    }

    str[sp] = 0;
    return write(STDOUT_FILENO, str, sp);
}

void sio_error(char s[]) /* Put error message and exit */
{
    sio_puts(s);
    _exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t
    *
    Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}




/******************************************
 * Wrappers for Unix signal functions
 ******************************************/

/* Sigprocmask - Wrapper for Unix signal function sigprocmask */
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
        unix_error("Sigprocmask error");
    return;
}

/* Sigemptyset - Wrapper for Unix signal function sigemptyset */
void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
        unix_error("Sigemptyset error");
    return;
}

/* Sigfillset - Wrapper for Unix signal function sigfillset */
void Sigfillset(sigset_t *set)
{
    if (sigfillset(set) < 0)
        unix_error("Sigfillset error");
    return;
}

/* Sigaddset - Wrapper for Unix signal function sigaddset */
void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
        unix_error("Sigaddset error");
    return;
}

/* Sigsuspend - Wrapper for Unix signal function sigsuspend */
int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* always returns -1 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}




/*********************************************
 * Wrappers for Unix process control functions
 ********************************************/

/* Fork - Wrapper for Unix process control fork */
pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}

/* Execve - Wrapper for Unix process control execve */
void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if (execve(filename, argv, envp) < 0)
        unix_error("Execve error");
    return;
}

/* Kill - Wrapper for Unix process control kill */
void Kill(pid_t pid, int signum)
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
        unix_error("Kill error");
}

/* Setpgid - Wrapper for Unix process control setpgid */
void Setpgid(pid_t pid, pid_t pgid)
{
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
    {
        unix_error("Setpgid error");
        _exit(0);
    }
    return;
}




/********************************
 * Wrappers for Unix I/O routines
 ********************************/

/* Open - Wrapper for Unix I/O open */
int Open(char *pathname, int flags, mode_t mode)
{
    int rc;

    if ((rc = open(pathname, flags, mode)) < 0)
    {
        sio_puts("Error: ");
        sio_puts(pathname);
        sio_puts(" No such file or directory\n");
    }
    return rc;
}

/* Close - Wrapper for Unix I/O close */
void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
        unix_error("Close error");
}

/* Dup - Wrapper for Unix I/O dup */
int Dup(int fd)
{
    int rc;
    if ((rc = dup(fd)) < 0)
        unix_error("Dup error");
    return rc;
}

/* Dup2 - Wrapper for Unix I/O dup2 */
int Dup2(int fd1, int fd2)
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
        unix_error("Dup2 error");
    return rc;
}