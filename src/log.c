#include "common.h"

const char *program_name = "";
unsigned long long debug_flags = DEBUG;

int access_log_syslog = 1;
int error_log_syslog = 1;
int output_log_syslog = 1;  // debug log

int stdaccess_fd = -1;
FILE *stdaccess = NULL;

const char *stdaccess_filename = NULL;
const char *stderr_filename = NULL;
const char *stdout_filename = NULL;

void syslog_init(void) {
    static int i = 0;

    if(!i) {
        openlog(program_name, LOG_PID, LOG_DAEMON);
        i = 1;
    }
}

int open_log_file(int fd, FILE **fp, const char *filename, int *enabled_syslog) {
    int f, t;

    if(!filename || !*filename || !strcmp(filename, "none"))
        filename = "/dev/null";

    if(!strcmp(filename, "syslog")) {
        filename = "/dev/null";
        syslog_init();
        if(enabled_syslog) *enabled_syslog = 1;
    }
    else if(enabled_syslog) *enabled_syslog = 0;

    // don't do anything if the user is willing
    // to have the standard one
    if(!strcmp(filename, "system")) {
        if(fd != -1) return fd;
        filename = "stdout";
    }

    if(!strcmp(filename, "stdout"))
        f = STDOUT_FILENO;

    else if(!strcmp(filename, "stderr"))
        f = STDERR_FILENO;

    else {
        f = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0664);
        if(f == -1) {
            error("Cannot open file '%s'. Leaving %d to its default.", filename, fd);
            return fd;
        }
    }

    // if there is a level-2 file pointer
    // flush it before switching the level-1 fds
    if(fp && *fp)
        fflush(*fp);

    if(fd != f && fd != -1) {
        // it automatically closes
        t = dup2(f, fd);
        if (t == -1) {
            error("Cannot dup2() new fd %d to old fd %d for '%s'", f, fd, filename);
            close(f);
            return fd;
        }
        // info("dup2() new fd %d to old fd %d for '%s'", f, fd, filename);
        close(f);
    }
    else fd = f;

    if(fp && !*fp) {
        *fp = fdopen(fd, "a");
        if (!*fp)
            error("Cannot fdopen() fd %d ('%s')", fd, filename);
        else {
            if (setvbuf(*fp, NULL, _IOLBF, 0) != 0)
                error("Cannot set line buffering on fd %d ('%s')", fd, filename);
        }
    }

    return fd;
}

void reopen_all_log_files() {
    if(stdout_filename)
        open_log_file(STDOUT_FILENO, (FILE **)&stdout, stdout_filename, &output_log_syslog);

    if(stderr_filename)
        open_log_file(STDERR_FILENO, (FILE **)&stderr, stderr_filename, &error_log_syslog);

    if(stdaccess_filename)
        stdaccess_fd = open_log_file(stdaccess_fd, (FILE **)&stdaccess, stdaccess_filename, &access_log_syslog);
}

void open_all_log_files() {
    // disable stdin
    open_log_file(STDIN_FILENO, (FILE **)&stdin, "/dev/null", NULL);

    open_log_file(STDOUT_FILENO, (FILE **)&stdout, stdout_filename, &output_log_syslog);
    open_log_file(STDERR_FILENO, (FILE **)&stderr, stderr_filename, &error_log_syslog);
    stdaccess_fd = open_log_file(stdaccess_fd, (FILE **)&stdaccess, stdaccess_filename, &access_log_syslog);
}

// ----------------------------------------------------------------------------
// error log throttling

time_t error_log_throttle_period_backup = 0;
time_t error_log_throttle_period = 1200;
unsigned long error_log_errors_per_period = 200;

int error_log_limit(int reset) {
    static time_t start = 0;
    static unsigned long counter = 0, prevented = 0;

    // do not throttle if the period is 0
    if(error_log_throttle_period == 0)
        return 0;

    // prevent all logs if the errors per period is 0
    if(error_log_errors_per_period == 0)
        return 1;

    time_t now = time(NULL);
    if(!start) start = now;

    if(reset) {
        if(prevented) {
            log_date(stderr);
            fprintf(stderr, "%s: Resetting logging for process '%s' (prevented %lu logs in the last %ld seconds).\n"
                    , program_name
                    , program_name
                    , prevented
                    , now - start
            );
        }

        start = now;
        counter = 0;
        prevented = 0;
    }

    // detect if we log too much
    counter++;

    if(now - start > error_log_throttle_period) {
        if(prevented) {
            log_date(stderr);
            fprintf(stderr, "%s: Resuming logging from process '%s' (prevented %lu logs in the last %ld seconds).\n"
                    , program_name
                    , program_name
                    , prevented
                    , error_log_throttle_period
            );
        }

        // restart the period accounting
        start = now;
        counter = 1;
        prevented = 0;

        // log this error
        return 0;
    }

    if(counter > error_log_errors_per_period) {
        if(!prevented) {
            log_date(stderr);
            fprintf(stderr, "%s: Too many logs (%lu logs in %ld seconds, threshold is set to %lu logs in %ld seconds). Preventing more logs from process '%s' for %ld seconds.\n"
                    , program_name
                    , counter
                    , now - start
                    , error_log_errors_per_period
                    , error_log_throttle_period
                    , program_name
                    , start + error_log_throttle_period - now);
        }

        prevented++;

        // prevent logging this error
        return 1;
    }

    return 0;
}

// ----------------------------------------------------------------------------
// print the date

// FIXME
// this should print the date in a buffer the way it
// is now, logs from multiple threads may be multiplexed

void log_date(FILE *out)
{
        char outstr[24];
        time_t t;
        struct tm *tmp, tmbuf;

        t = time(NULL);
        tmp = localtime_r(&t, &tmbuf);

        if (tmp == NULL) return;
        if (unlikely(strftime(outstr, sizeof(outstr), "%y-%m-%d %H:%M:%S", tmp) == 0)) return;

        fprintf(out, "%s: ", outstr);
}

// ----------------------------------------------------------------------------
// debug log

void debug_int( const char *file, const char *function, const unsigned long line, const char *fmt, ... )
{
    va_list args;

    log_date(stdout);
    va_start( args, fmt );
    printf("DEBUG (%04lu@%-10.10s:%-15.15s): %s: ", line, file, function, program_name);
    vprintf(fmt, args);
    va_end( args );
    putchar('\n');

    if(output_log_syslog) {
        va_start( args, fmt );
        vsyslog(LOG_ERR,  fmt, args );
        va_end( args );
    }

    fflush(stdout);
}

// ----------------------------------------------------------------------------
// info log

void info_int( const char *file, const char *function, const unsigned long line, const char *fmt, ... )
{
    va_list args;

    // prevent logging too much
    if(error_log_limit(0)) return;

    log_date(stderr);

    va_start( args, fmt );
    if(debug_flags) fprintf(stderr, "INFO (%04lu@%-10.10s:%-15.15s): %s: ", line, file, function, program_name);
    else            fprintf(stderr, "INFO: %s: ", program_name);
    vfprintf( stderr, fmt, args );
    va_end( args );

    fputc('\n', stderr);

    if(error_log_syslog) {
        va_start( args, fmt );
        vsyslog(LOG_INFO,  fmt, args );
        va_end( args );
    }
}

// ----------------------------------------------------------------------------
// error log

#if defined(STRERROR_R_CHAR_P)
// GLIBC version of strerror_r
static const char *strerror_result(const char *a, const char *b) { (void)b; return a; }
#elif defined(HAVE_STRERROR_R)
// POSIX version of strerror_r
static const char *strerror_result(int a, const char *b) { (void)a; return b; }
#elif defined(HAVE_C__GENERIC)

// what a trick!
// http://stackoverflow.com/questions/479207/function-overloading-in-c
static const char *strerror_result_int(int a, const char *b) { (void)a; return b; }
static const char *strerror_result_string(const char *a, const char *b) { (void)b; return a; }

#define strerror_result(a, b) _Generic((a), \
    int: strerror_result_int, \
    char *: strerror_result_string \
    )(a, b)

#else
#error "cannot detect the format of function strerror_r()"
#endif

void error_int( const char *prefix, const char *file, const char *function, const unsigned long line, const char *fmt, ... )
{
    va_list args;

    // prevent logging too much
    if(error_log_limit(0)) return;

    log_date(stderr);

    va_start( args, fmt );
    if(debug_flags) fprintf(stderr, "%s (%04lu@%-10.10s:%-15.15s): %s: ", prefix, line, file, function, program_name);
    else            fprintf(stderr, "%s: %s: ", prefix, program_name);
    vfprintf( stderr, fmt, args );
    va_end( args );

    if(errno) {
        char buf[1024];
        fprintf(stderr, " (errno %d, %s)\n", errno, strerror_result(strerror_r(errno, buf, 1023), buf));
        errno = 0;
    }
    else
        fputc('\n', stderr);

    if(error_log_syslog) {
        va_start( args, fmt );
        vsyslog(LOG_ERR,  fmt, args );
        va_end( args );
    }
}

void fatal_int( const char *file, const char *function, const unsigned long line, const char *fmt, ... )
{
    va_list args;

    log_date(stderr);

    va_start( args, fmt );
    if(debug_flags) fprintf(stderr, "FATAL (%04lu@%-10.10s:%-15.15s): %s: ", line, file, function, program_name);
    else            fprintf(stderr, "FATAL: %s: ", program_name);
    vfprintf( stderr, fmt, args );
    va_end( args );

    perror(" # ");
    fputc('\n', stderr);

    if(error_log_syslog) {
        va_start( args, fmt );
        vsyslog(LOG_CRIT,  fmt, args );
        va_end( args );
    }

    netdata_cleanup_and_exit(1);
}

// ----------------------------------------------------------------------------
// access log

void log_access( const char *fmt, ... )
{
    va_list args;

    if(stdaccess) {
        log_date(stdaccess);

        va_start( args, fmt );
        vfprintf( stdaccess, fmt, args );
        va_end( args );
        fputc('\n', stdaccess);
    }

    if(access_log_syslog) {
        va_start( args, fmt );
        vsyslog(LOG_INFO,  fmt, args );
        va_end( args );
    }
}

