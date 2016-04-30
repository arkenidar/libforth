/** @file     unit.c
 *  @brief    unit tests for liblisp interpreter public interface
 *  @author   Richard Howe (2015)
 *  @license  LGPL v2.1 or Later 
 *            <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email    howe.r.j.89@gmail.com **/

/*** module to test ***/
#include "libforth.h"
/**********************/

#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*** very minimal test framework ***/

static unsigned passed, failed;
static double timer;
static clock_t start_time, end_time;
static time_t rawtime;

int color_on = 0, jmpbuf_active = 0;
jmp_buf current_test;
unsigned current_line = 0;
const char *current_expr;

static const char *reset(void)  { return color_on ? "\x1b[0m"  : ""; }
static const char *red(void)    { return color_on ? "\x1b[31m" : ""; }
static const char *green(void)  { return color_on ? "\x1b[32m" : ""; }
static const char *yellow(void) { return color_on ? "\x1b[33m" : ""; }
static const char *blue(void)   { return color_on ? "\x1b[34m" : ""; }

static void unit_tester(int test, const char *msg, unsigned line) {
        if(test) passed++, printf("      %sok%s:\t%s\n", green(), reset(), msg); 
        else     failed++, printf("  %sFAILED%s:\t%s (line %d)\n", red(), reset(), msg, line);
}

static void print_statement(char *stmt) {
        printf("   %sstate%s:\t%s\n", blue(), reset(), stmt);
}

static void print_note(char *name) { printf("%s%s%s\n", yellow(), name, reset()); }

#define MAX_SIGNALS (256)
static char *(sig_lookup[]) = { /*List of C89 signals and their names*/
        [SIGABRT]       = "SIGABRT",
        [SIGFPE]        = "SIGFPE",
        [SIGILL]        = "SIGILL",
        [SIGINT]        = "SIGINT",
        [SIGSEGV]       = "SIGSEGV",
        [SIGTERM]       = "SIGTERM",
        [MAX_SIGNALS]   = NULL
};

static int caught_signal;
static void print_caught_signal_name(void) {
        char *sig_name = "UNKNOWN SIGNAL";
        if((caught_signal > 0) && (caught_signal < MAX_SIGNALS) && sig_lookup[caught_signal])
                sig_name = sig_lookup[caught_signal];
        printf("Caught %s (signal number %d)\n", sig_name, caught_signal);\
}

static void sig_abrt_handler(int sig) {
        caught_signal = sig;
        if(jmpbuf_active) {
                jmpbuf_active = 0;
                longjmp(current_test, 1);
        }
}

/**@brief Advance the test suite by testing and executing an expression. This
 *        framework can catch assertions that have failed within the expression
 *        being tested.
 * @param EXPR The expression should yield non zero on success **/
#define test(EXPR)\
        do {\
                current_line = __LINE__;\
                current_expr = #EXPR;\
                signal(SIGABRT, sig_abrt_handler);\
                if(!setjmp(current_test)) {\
                        jmpbuf_active = 1;\
                        unit_tester( ((EXPR) != 0), current_expr, current_line);\
                } else {\
                        print_caught_signal_name();\
                        unit_tester(0, current_expr, current_line);\
                        signal(SIGABRT, sig_abrt_handler);\
                }\
                signal(SIGABRT, SIG_DFL);\
                jmpbuf_active = 0;\
        } while(0);

/**@brief print out and execute a statement that is needed to further a test
 * @param STMT A statement to print out (stringify first) and then execute**/
#define state(STMT) do{ print_statement( #STMT ); STMT; } while(0);

/**@brief As signals are caught (such as those generated by abort()), we exit
 *        the unit test function by returning from it instead. */
#define return_if(EXPR) if((EXPR)) { printf("unit test framework failed on line '%d'\n", __LINE__); return -1;}

static int unit_test_start(const char *unit_name) {
        time(&rawtime);
        if(signal(SIGABRT, sig_abrt_handler) == SIG_ERR) {
                printf("signal handler installation failed");
                return -1;
        }
        start_time = clock();
        printf("%s unit tests\n%sbegin:\n\n", unit_name, asctime(localtime(&rawtime)));
        return 0;
}

static unsigned unit_test_end(const char *unit_name) {
        end_time = clock();
        timer = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
        printf("\n\n%s unit tests\npassed  %u/%u\ntime    %fs\n", unit_name, passed, passed+failed, timer);
        return failed;
}

/*** end minimal test framework ***/

int main(int argc, char **argv) {
        if(argc > 1)
                while(++argv, --argc) {
                        if(!strcmp("-c", argv[0]))
                                color_on = 1;
                        else if (!strcmp("-h", argv[0]))
                                printf("liblisp unit tests\n\tusage ./%s (-c)? (-h)?\n", argv[0]);
                        else
                                printf("unknown argument '%s'\n", argv[0]);
                }

        unit_test_start("libforth");
        {
                forth_t *f;
                print_note("libforth.c");
                test(f = forth_init(MINIMUM_CORE_SIZE, stdin, stdout));

                test(forth_eval(f, "here . cr")  >= 0);
                test(forth_eval(f, "2 2 + . cr") >= 0);
		state(forth_push(f, 99));
		state(forth_push(f, 98));
                test(forth_eval(f, "+") >= 0);
		test(forth_pop(f) == 197);

                state(forth_free(f));
        }
        return unit_test_end("libforth"); /*should be zero!*/

}

