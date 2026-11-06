/*
 * ft_printf mandatory-part stress tester (500 cases)
 *
 * Build example:
 *   cc -Wall -Wextra -Werror -g -fsanitize=address,undefined \
 *      -fno-omit-frame-pointer main.c libftprintf.a -o app
 *
 * This file tests only the mandatory conversions: c, s, p, d, i, u, x, X, %.
 * Each case runs in a child process so a crash or timeout is reported against
 * its test number instead of terminating the complete test run.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "ft_printf.h"

#define EXPECTED_TESTS 500
#define TEST_TIMEOUT_MS 3000
#define MAX_CAPTURE_SIZE (64u * 1024u * 1024u)

static int      g_test_id;
static int      g_passed;
static int      g_failed;
static int      g_crashed;
static int      g_timed_out;
static char     g_ft_path[160];
static char     g_real_path[160];

typedef struct s_child_report
{
    int ret_ft;
    int ret_real;
    int error;
}   t_child_report;

static int capture_start(const char *path, int *saved_stdout)
{
    int fd;

    if (fflush(stdout) == EOF)
        return (-1);
    *saved_stdout = dup(STDOUT_FILENO);
    if (*saved_stdout < 0)
        return (-1);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
    {
        close(*saved_stdout);
        return (-1);
    }
    if (dup2(fd, STDOUT_FILENO) < 0)
    {
        close(fd);
        close(*saved_stdout);
        return (-1);
    }
    close(fd);
    return (0);
}

static int capture_end(int saved_stdout)
{
    int result;

    result = 0;
    if (fflush(stdout) == EOF)
        result = -1;
    if (dup2(saved_stdout, STDOUT_FILENO) < 0)
        result = -1;
    close(saved_stdout);
    return (result);
}

static void child_finish(int report_fd, t_child_report report)
{
    ssize_t written;

    written = write(report_fd, &report, sizeof(report));
    close(report_fd);
    if (written != (ssize_t)sizeof(report))
        _exit(120);
    exit(0);
}

static int wait_with_timeout(pid_t pid, int *status)
{
    struct timespec pause_time;
    int             elapsed;
    pid_t           result;

    pause_time.tv_sec = 0;
    pause_time.tv_nsec = 1000000L;
    elapsed = 0;
    while (elapsed < TEST_TIMEOUT_MS)
    {
        result = waitpid(pid, status, WNOHANG);
        if (result == pid)
            return (0);
        if (result < 0 && errno != EINTR)
            return (-1);
        nanosleep(&pause_time, NULL);
        elapsed++;
    }
    kill(pid, SIGKILL);
    while (waitpid(pid, status, 0) < 0 && errno == EINTR)
        ;
    return (1);
}

static int read_report(int fd, t_child_report *report)
{
    unsigned char   *dst;
    size_t          received;
    ssize_t         amount;

    dst = (unsigned char *)report;
    received = 0;
    while (received < sizeof(*report))
    {
        amount = read(fd, dst + received, sizeof(*report) - received);
        if (amount == 0)
            return (-1);
        if (amount < 0)
        {
            if (errno == EINTR)
                continue;
            return (-1);
        }
        received += (size_t)amount;
    }
    return (0);
}

static int read_output_file(const char *path, unsigned char **data,
        size_t *length)
{
    int             fd;
    struct stat     info;
    size_t          expected;
    size_t          received;
    ssize_t         amount;

    *data = NULL;
    *length = 0;
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return (-1);
    if (fstat(fd, &info) < 0 || info.st_size < 0
        || (uintmax_t)info.st_size > (uintmax_t)MAX_CAPTURE_SIZE)
    {
        close(fd);
        return (-1);
    }
    expected = (size_t)info.st_size;
    *data = (unsigned char *)malloc(expected == 0 ? 1 : expected);
    if (*data == NULL)
    {
        close(fd);
        return (-1);
    }
    received = 0;
    while (received < expected)
    {
        amount = read(fd, *data + received, expected - received);
        if (amount == 0)
            break;
        if (amount < 0)
        {
            if (errno == EINTR)
                continue;
            free(*data);
            *data = NULL;
            close(fd);
            return (-1);
        }
        received += (size_t)amount;
    }
    close(fd);
    if (received != expected)
    {
        free(*data);
        *data = NULL;
        return (-1);
    }
    *length = received;
    return (0);
}

static void report_difference(const t_child_report *report,
        const unsigned char *ft_data, size_t ft_length,
        const unsigned char *real_data, size_t real_length)
{
    size_t i;
    size_t common;

    if (report->ret_ft != report->ret_real)
        printf("      return value: ft_printf=%d, printf=%d\n",
            report->ret_ft, report->ret_real);
    if (ft_length != real_length)
        printf("      output length: ft_printf=%zu, printf=%zu\n",
            ft_length, real_length);
    if (report->ret_ft < 0 || (size_t)report->ret_ft != ft_length)
        printf("      ft_printf return value does not equal its output length\n");
    if (report->ret_real < 0 || (size_t)report->ret_real != real_length)
        printf("      printf return value does not equal its output length\n");
    common = ft_length < real_length ? ft_length : real_length;
    i = 0;
    while (i < common && ft_data[i] == real_data[i])
        i++;
    if (i < common)
        printf("      first byte difference at %zu: ft=0x%02X, printf=0x%02X\n",
            i, (unsigned int)ft_data[i], (unsigned int)real_data[i]);
}

/*
 * TEST(label, format, arguments...)
 * TEST_FMT(label, dynamic_format_without_conversions, expected_text)
 * The second form is reserved for dynamically generated literal-only formats.
 */
#define RUN_CASE(label, ft_expression, real_expression) do { \
    int             pipefd[2]; \
    int             wait_status; \
    int             wait_result; \
    pid_t           child_pid; \
    t_child_report  child_report; \
    unsigned char   *ft_data; \
    unsigned char   *real_data; \
    size_t          ft_length; \
    size_t          real_length; \
    int             equal; \
    int             this_id; \
    this_id = ++g_test_id; \
    ft_data = NULL; \
    real_data = NULL; \
    ft_length = 0; \
    real_length = 0; \
    memset(&child_report, 0, sizeof(child_report)); \
    unlink(g_ft_path); \
    unlink(g_real_path); \
    fflush(stdout); \
    if (pipe(pipefd) < 0) { \
        printf("T%03d: ERROR - pipe() failed\n", this_id); \
        g_failed++; \
    } else { \
        child_pid = fork(); \
        if (child_pid == 0) { \
            int saved_fd; \
            close(pipefd[0]); \
            child_report.error = 0; \
            if (capture_start(g_ft_path, &saved_fd) < 0) { \
                child_report.error = 1; \
                child_finish(pipefd[1], child_report); \
            } \
            child_report.ret_ft = (ft_expression); \
            if (capture_end(saved_fd) < 0) { \
                child_report.error = 2; \
                child_finish(pipefd[1], child_report); \
            } \
            if (capture_start(g_real_path, &saved_fd) < 0) { \
                child_report.error = 3; \
                child_finish(pipefd[1], child_report); \
            } \
            child_report.ret_real = (real_expression); \
            if (capture_end(saved_fd) < 0) { \
                child_report.error = 4; \
                child_finish(pipefd[1], child_report); \
            } \
            child_finish(pipefd[1], child_report); \
        } else if (child_pid < 0) { \
            close(pipefd[0]); \
            close(pipefd[1]); \
            printf("T%03d: ERROR - fork() failed\n", this_id); \
            g_failed++; \
        } else { \
            close(pipefd[1]); \
            wait_result = wait_with_timeout(child_pid, &wait_status); \
            if (wait_result == 1) { \
                printf("T%03d: TIMEOUT - %s\n", this_id, (label)); \
                g_failed++; \
                g_timed_out++; \
            } else if (wait_result < 0) { \
                printf("T%03d: ERROR - waitpid() failed - %s\n", this_id, (label)); \
                g_failed++; \
            } else if (WIFSIGNALED(wait_status)) { \
                printf("T%03d: CRASH - %s (signal %d)\n", this_id, (label), \
                    WTERMSIG(wait_status)); \
                g_failed++; \
                g_crashed++; \
            } else if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0) { \
                printf("T%03d: RUNTIME ERROR - %s (exit %d)\n", this_id, \
                    (label), WIFEXITED(wait_status) ? WEXITSTATUS(wait_status) : -1); \
                g_failed++; \
            } else if (read_report(pipefd[0], &child_report) < 0) { \
                printf("T%03d: ERROR - child report missing - %s\n", this_id, (label)); \
                g_failed++; \
            } else if (child_report.error != 0) { \
                printf("T%03d: ERROR - capture setup/restoration (%d) - %s\n", \
                    this_id, child_report.error, (label)); \
                g_failed++; \
            } else if (read_output_file(g_ft_path, &ft_data, &ft_length) < 0 \
                || read_output_file(g_real_path, &real_data, &real_length) < 0) { \
                printf("T%03d: ERROR - could not read captured output - %s\n", \
                    this_id, (label)); \
                g_failed++; \
            } else { \
                equal = (child_report.ret_ft == child_report.ret_real \
                    && child_report.ret_ft >= 0 \
                    && child_report.ret_real >= 0 \
                    && (size_t)child_report.ret_ft == ft_length \
                    && (size_t)child_report.ret_real == real_length \
                    && ft_length == real_length \
                    && memcmp(ft_data, real_data, ft_length) == 0); \
                if (equal) { \
                    printf("T%03d: PASS - %s\n", this_id, (label)); \
                    g_passed++; \
                } else { \
                    printf("T%03d: FAIL - %s\n", this_id, (label)); \
                    report_difference(&child_report, ft_data, ft_length, \
                        real_data, real_length); \
                    g_failed++; \
                } \
            } \
            free(ft_data); \
            free(real_data); \
            close(pipefd[0]); \
        } \
    } \
    fflush(stdout); \
} while (0)

#define TEST(label, ...) \
    RUN_CASE((label), (ft_printf(__VA_ARGS__)), (printf(__VA_ARGS__)))

#define TEST_FMT(label, dynamic_format, expected_text) \
    RUN_CASE((label), (ft_printf(dynamic_format)), \
        (printf("%s", (expected_text))))

static char *make_text(size_t length, unsigned int seed)
{
    char            *text;
    size_t          i;
    unsigned int    value;

    text = (char *)malloc(length + 1);
    if (text == NULL)
        return (NULL);
    i = 0;
    while (i < length)
    {
        value = (unsigned int)((i * 37u + seed * 13u) % 94u);
        text[i] = (char)(33u + value);
        i++;
    }
    text[length] = '\0';
    return (text);
}

static char *make_plain_format(size_t length, unsigned int seed)
{
    char            *text;
    size_t          i;

    text = make_text(length, seed);
    if (text == NULL)
        return (NULL);
    i = 0;
    while (i < length)
    {
        if (text[i] == '%')
            text[i] = '#';
        i++;
    }
    return (text);
}

static void print_summary(void)
{
    printf("\n============================================================\n");
    printf("                 FT_PRINTF 500-TEST REPORT                  \n");
    printf("============================================================\n");
    printf("TOTAL:       %d\n", g_test_id);
    printf("PASSED:      %d\n", g_passed);
    printf("FAILED:      %d\n", g_failed);
    printf("CRASHED:     %d\n", g_crashed);
    printf("TIMED OUT:   %d\n", g_timed_out);
    printf("============================================================\n");
}

int main(void)
{
    static const char *plain_texts[25] = {
        "", "Hello 1337", "A single space: ", " leading spaces",
        "trailing spaces   ", "spaces   between   words",
        "tabs\tbetween\twords", "line one\nline two", "first\r\nsecond",
        "control:\a\b\f\v", "0123456789", "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "abcdefghijklmnopqrstuvwxyz", "!@#$^&*()-_=+[]",
        "{}<>:;,.?/\\|~`", "quotes: 'single' and \"double\"",
        "slashes: / / backslash: \\", "C path: C:\\Users\\tester\\file.c",
        "Unix path: /home/tester/project/file.c",
        "JSON {\"key\":\"value\",\"n\":42}", "UTF-8: المغرب",
        "Repeated AAAAAAAAAA", "Mixed 1337 ABC xyz 42",
        "A longer literal sentence with punctuation, spaces, and digits 2026.",
        "ends with newline\n"
    };
    static const int char_values[60] = {
        INT_MIN, INT_MIN + 1, -1025, -512, -257, -256, -255, -129, -128, -127,
        -2, -1, 0, 1, 2, 7, 8, 9, 10, 13, 27, 31, 32, 33, 47, 48, 57, 64,
        65, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104,
        105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 126, 127, 128,
        255, 256
    };
    static const size_t short_string_sizes[25] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 15, 16, 31, 32, 63, 64, 127, 128, 255,
        256, 511, 512, 1023, 1024, 2048, 4096
    };
    static const int signed_boundaries[25] = {
        INT_MIN, INT_MIN + 1, -2000000000, -1000000000, -65536, -32769,
        -32768, -1024, -256, -129, -128, -1, 0, 1, 2, 9, 10, 99, 100,
        127, 255, 256, 32767, 1000000000, INT_MAX
    };
    static const unsigned int unsigned_boundaries[15] = {
        0u, 1u, 2u, 9u, 10u, 15u, 16u, 255u, 256u, 65535u, 65536u,
        UINT_MAX / 2u, UINT_MAX / 2u + 1u, UINT_MAX - 1u, UINT_MAX
    };
    static const unsigned int hex_patterns[15] = {
        0x01010101u, 0x10101010u, 0x11111111u, 0x12345678u, 0x87654321u,
        0xdeadbeefu, 0xabcdef01u, 0x00ff00ffu, 0xff00ff00u, 0x55555555u,
        0xaaaaaaaau, 0x0f0f0f0fu, 0xf0f0f0f0u, 0x7f7f7f7fu, 0x80808080u
    };
    static const char *strings[20] = {
        "", "A", "z", "1337", "Hello world", " leading",
        "trailing ", "two words", "spaces   inside", "line1\nline2",
        "tab\tinside", "quotes \'single\' and double", "slashes / and \\",
        "punctuation !@#$^&*()-_=+", "brackets [] {} () <>",
        "UTF-8: المغرب", "digits 0123456789", "mixed ABCxyz09",
        "string with percent % sign", "A different string for va_arg testing"
    };
    int             pointer_ints[32];
    char            pointer_chars[128];
    long            pointer_longs[4];
    int             i;
    int             value;
    unsigned int    uvalue;
    unsigned int    state;
    uint32_t        random_state;
    char            label[96];
    char            *dynamic_text;
    char            *large_a;
    char            *large_b;
    char            *large_c;
    char            *dynamic_format;
    char            *expected_format;
    size_t          length;
    size_t          j;
    char            c1;
    const char      *s1;
    void            *p1;
    int             d1;
    int             d2;
    unsigned int    u1;
    unsigned int    u2;
    unsigned int    u3;

    snprintf(g_ft_path, sizeof(g_ft_path), "/tmp/ft_printf_500_%ld_ft.bin", (long)getpid());
    snprintf(g_real_path, sizeof(g_real_path), "/tmp/ft_printf_500_%ld_real.bin", (long)getpid());
    i = 0;
    while (i < 32)
    {
        pointer_ints[i] = i * 7919;
        i++;
    }
    memset(pointer_chars, 'P', sizeof(pointer_chars));
    i = 0;
    while (i < 4)
    {
        pointer_longs[i] = (long)i * 123456789L;
        i++;
    }

    printf("============================================================\n");
    printf("       STARTING 500 DISTINCT MANDATORY-PART TESTS          \n");
    printf("  Each case is isolated; output bytes and return values are compared.\n");
    printf("============================================================\n\n");
    fflush(stdout);

    /* 001-025: Plain format strings and literal output paths. */
    i = 0;
    while (i < 25)
    {
        snprintf(label, sizeof(label), "plain text case %02d", i + 1);
        TEST(label, "%s", plain_texts[i]);
        i++;
    }

    /* 026-085: %c values, including negative, NUL, control, and byte limits. */
    i = 0;
    while (i < 60)
    {
        snprintf(label, sizeof(label), "char argument value %d", char_values[i]);
        if (i % 4 == 0)
            TEST(label, "%c", char_values[i]);
        else if (i % 4 == 1)
            TEST(label, "[%c]", char_values[i]);
        else if (i % 4 == 2)
            TEST(label, "left:%c:right", char_values[i]);
        else
            TEST(label, "%c%c", char_values[i], char_values[(i + 7) % 60]);
        i++;
    }

    /* 086-130: %s fixed cases plus lengths around common buffer boundaries. */
    i = 0;
    while (i < 20)
    {
        snprintf(label, sizeof(label), "string pattern %02d", i + 1);
        TEST(label, "<%s>", strings[i]);
        i++;
    }
    i = 0;
    while (i < 25)
    {
        dynamic_text = make_text(short_string_sizes[i], (unsigned int)(i + 3));
        if (dynamic_text == NULL)
            return (2);
        snprintf(label, sizeof(label), "string length %zu", short_string_sizes[i]);
        TEST(label, "begin|%s|end", dynamic_text);
        free(dynamic_text);
        i++;
    }

    /* 131-165: %p with real object addresses, NULL, and one-past pointers. */
    TEST("null object pointer", "%p", (void *)0);
    i = 0;
    while (i < 32)
    {
        snprintf(label, sizeof(label), "address of int array element %d", i);
        TEST(label, "%p", (void *)&pointer_ints[i]);
        i++;
    }
    TEST("address of character array", "%p", (void *)pointer_chars);
    TEST("one-past character array address", "%p", (void *)(pointer_chars + sizeof(pointer_chars)));

    /* 166-230: %d and %i signed integer boundaries and deterministic values. */
    i = 0;
    while (i < 25)
    {
        snprintf(label, sizeof(label), "signed boundary %d", signed_boundaries[i]);
        if (i % 2 == 0)
            TEST(label, "value=%d", signed_boundaries[i]);
        else
            TEST(label, "value=%i", signed_boundaries[i]);
        i++;
    }
    random_state = 0x13579BDFu;
    i = 0;
    while (i < 40)
    {
        random_state = random_state * 1664525u + 1013904223u;
        value = (int)(random_state % 2000000001u) - 1000000000;
        snprintf(label, sizeof(label), "signed generated value %02d (%d)", i + 1, value);
        if (i % 2 == 0)
            TEST(label, "%d", value);
        else
            TEST(label, "%i", value);
        i++;
    }

    /* 231-265: %u, including UINT_MAX and high-bit values. */
    i = 0;
    while (i < 15)
    {
        snprintf(label, sizeof(label), "unsigned boundary %u", unsigned_boundaries[i]);
        TEST(label, "unsigned=%u", unsigned_boundaries[i]);
        i++;
    }
    state = 0x2468ACE1u;
    i = 0;
    while (i < 20)
    {
        state = state * 1103515245u + 12345u;
        uvalue = state ^ (unsigned int)(i * 0x10203u);
        snprintf(label, sizeof(label), "unsigned generated value %02d", i + 1);
        TEST(label, "u=%u", uvalue);
        i++;
    }

    /* 266-310: %x and %X with nibble, byte, word, and bit-pattern coverage. */
    {
        static const unsigned int hex_boundaries[15] = {
            0u, 1u, 0x0fu, 0x10u, 0xffu, 0x100u, 0xfffu, 0x1000u,
            0xffffu, 0x10000u, UINT_MAX / 2u, UINT_MAX / 2u + 1u,
            UINT_MAX - 2u, UINT_MAX - 1u, UINT_MAX
        };
        i = 0;
        while (i < 15)
        {
            snprintf(label, sizeof(label), "hex boundary case %02d", i + 1);
            if (i % 2 == 0)
                TEST(label, "lower=%x", hex_boundaries[i]);
            else
                TEST(label, "upper=%X", hex_boundaries[i]);
            i++;
        }
    }
    i = 0;
    while (i < 15)
    {
        snprintf(label, sizeof(label), "hex bit pattern %02d", i + 1);
        if (i % 2 == 0)
            TEST(label, "pattern=%x", hex_patterns[i]);
        else
            TEST(label, "pattern=%X", hex_patterns[i]);
        i++;
    }
    state = 0xA5A5F00Du;
    i = 0;
    while (i < 15)
    {
        state = state * 1664525u + 1013904223u;
        uvalue = state ^ (unsigned int)(0x9E3779B9u + (unsigned int)i);
        snprintf(label, sizeof(label), "hex generated value %02d", i + 1);
        if (i % 2 == 0)
            TEST(label, "%x", uvalue);
        else
            TEST(label, "%X", uvalue);
        i++;
    }

    /* 311-325: Legal %% sequences; no malformed or undefined format strings. */
    TEST("escaped percent pattern 01", "%%");
    TEST("escaped percent pattern 02", "%%%%");
    TEST("escaped percent pattern 03", "prefix %%");
    TEST("escaped percent pattern 04", "%% suffix");
    TEST("escaped percent pattern 05", "a%%b");
    TEST("escaped percent pattern 06", "%% %%");
    TEST("escaped percent pattern 07", "progress: 10%%");
    TEST("escaped percent pattern 08", "%%100");
    TEST("escaped percent pattern 09", "%% %% %%");
    TEST("escaped percent pattern 10", "left%%%%right");
    TEST("escaped percent pattern 11", "%%-%%-%%");
    TEST("escaped percent pattern 12", "%%\n%%");
    TEST("escaped percent pattern 13", "%%\t%%");
    TEST("escaped percent pattern 14", "%%%%%%");
    TEST("escaped percent pattern 15", "start %% middle %% end");

    /* 326-425: 100 unique mixed formats, each with every conversion in a new order. */
    i = 0;
    while (i < 100)
    {
        d1 = (int)((unsigned int)(i * 7919 + 12345) % 2000000001u) - 1000000000;
        d2 = (int)((unsigned int)(i * 104729 + 54321) % 2000000001u) - 1000000000;
        u1 = (unsigned int)(i * 2654435761u) ^ 0xA5A5A5A5u;
        u2 = (unsigned int)(i * 2246822519u) ^ 0x5A5A5A5Au;
        u3 = (unsigned int)(i * 3266489917u) ^ UINT_MAX;
        c1 = (char)('A' + (i % 26));
        s1 = strings[i % 20];
        p1 = (void *)&pointer_ints[i % 32];
        snprintf(label, sizeof(label), "mixed conversion permutation %03d", i + 1);
        switch (i)
        {
            case 0: TEST(label, "MIX001:%c|%d|%X|%p|%s|%x|%i|%u|%%", c1, d1, u3, p1, s1, u2, d2, u1); break;
            case 1: TEST(label, "MIX002:%x|%X|%c|%p|%s|%u|%d|%i|%%", u2, u3, c1, p1, s1, u1, d1, d2); break;
            case 2: TEST(label, "MIX003:%X|%u|%s|%d|%p|%i|%c|%x|%%", u3, u1, s1, d1, p1, d2, c1, u2); break;
            case 3: TEST(label, "MIX004:%u|%d|%s|%X|%i|%p|%x|%c|%%", u1, d1, s1, u3, d2, p1, u2, c1); break;
            case 4: TEST(label, "MIX005:%c|%s|%u|%d|%i|%X|%x|%p|%%", c1, s1, u1, d1, d2, u3, u2, p1); break;
            case 5: TEST(label, "MIX006:%p|%s|%X|%u|%x|%c|%i|%d|%%", p1, s1, u3, u1, u2, c1, d2, d1); break;
            case 6: TEST(label, "MIX007:%p|%d|%x|%i|%s|%X|%u|%c|%%", p1, d1, u2, d2, s1, u3, u1, c1); break;
            case 7: TEST(label, "MIX008:%x|%s|%c|%p|%i|%u|%X|%d|%%", u2, s1, c1, p1, d2, u1, u3, d1); break;
            case 8: TEST(label, "MIX009:%d|%u|%p|%c|%X|%s|%i|%x|%%", d1, u1, p1, c1, u3, s1, d2, u2); break;
            case 9: TEST(label, "MIX010:%X|%c|%p|%u|%x|%s|%i|%d|%%", u3, c1, p1, u1, u2, s1, d2, d1); break;
            case 10: TEST(label, "MIX011:%p|%s|%c|%d|%i|%x|%u|%X|%%", p1, s1, c1, d1, d2, u2, u1, u3); break;
            case 11: TEST(label, "MIX012:%d|%c|%s|%p|%x|%i|%X|%u|%%", d1, c1, s1, p1, u2, d2, u3, u1); break;
            case 12: TEST(label, "MIX013:%i|%u|%d|%x|%X|%s|%c|%p|%%", d2, u1, d1, u2, u3, s1, c1, p1); break;
            case 13: TEST(label, "MIX014:%p|%x|%u|%d|%s|%X|%c|%i|%%", p1, u2, u1, d1, s1, u3, c1, d2); break;
            case 14: TEST(label, "MIX015:%d|%i|%X|%c|%s|%u|%p|%x|%%", d1, d2, u3, c1, s1, u1, p1, u2); break;
            case 15: TEST(label, "MIX016:%X|%i|%x|%p|%s|%c|%d|%u|%%", u3, d2, u2, p1, s1, c1, d1, u1); break;
            case 16: TEST(label, "MIX017:%c|%u|%p|%i|%X|%x|%s|%d|%%", c1, u1, p1, d2, u3, u2, s1, d1); break;
            case 17: TEST(label, "MIX018:%u|%c|%X|%d|%p|%x|%s|%i|%%", u1, c1, u3, d1, p1, u2, s1, d2); break;
            case 18: TEST(label, "MIX019:%X|%i|%c|%p|%d|%u|%s|%x|%%", u3, d2, c1, p1, d1, u1, s1, u2); break;
            case 19: TEST(label, "MIX020:%c|%x|%s|%i|%p|%X|%u|%d|%%", c1, u2, s1, d2, p1, u3, u1, d1); break;
            case 20: TEST(label, "MIX021:%c|%s|%x|%u|%i|%X|%p|%d|%%", c1, s1, u2, u1, d2, u3, p1, d1); break;
            case 21: TEST(label, "MIX022:%x|%c|%u|%d|%s|%X|%i|%p|%%", u2, c1, u1, d1, s1, u3, d2, p1); break;
            case 22: TEST(label, "MIX023:%d|%i|%s|%x|%p|%c|%u|%X|%%", d1, d2, s1, u2, p1, c1, u1, u3); break;
            case 23: TEST(label, "MIX024:%p|%X|%u|%d|%i|%c|%s|%x|%%", p1, u3, u1, d1, d2, c1, s1, u2); break;
            case 24: TEST(label, "MIX025:%i|%d|%x|%c|%p|%X|%u|%s|%%", d2, d1, u2, c1, p1, u3, u1, s1); break;
            case 25: TEST(label, "MIX026:%s|%d|%x|%c|%X|%p|%i|%u|%%", s1, d1, u2, c1, u3, p1, d2, u1); break;
            case 26: TEST(label, "MIX027:%s|%x|%X|%p|%d|%i|%u|%c|%%", s1, u2, u3, p1, d1, d2, u1, c1); break;
            case 27: TEST(label, "MIX028:%X|%u|%s|%x|%i|%c|%d|%p|%%", u3, u1, s1, u2, d2, c1, d1, p1); break;
            case 28: TEST(label, "MIX029:%d|%X|%c|%i|%u|%s|%x|%p|%%", d1, u3, c1, d2, u1, s1, u2, p1); break;
            case 29: TEST(label, "MIX030:%i|%c|%s|%p|%x|%u|%d|%X|%%", d2, c1, s1, p1, u2, u1, d1, u3); break;
            case 30: TEST(label, "MIX031:%d|%X|%i|%s|%c|%u|%p|%x|%%", d1, u3, d2, s1, c1, u1, p1, u2); break;
            case 31: TEST(label, "MIX032:%d|%c|%p|%X|%s|%x|%u|%i|%%", d1, c1, p1, u3, s1, u2, u1, d2); break;
            case 32: TEST(label, "MIX033:%u|%x|%p|%d|%i|%X|%c|%s|%%", u1, u2, p1, d1, d2, u3, c1, s1); break;
            case 33: TEST(label, "MIX034:%X|%u|%i|%p|%x|%s|%d|%c|%%", u3, u1, d2, p1, u2, s1, d1, c1); break;
            case 34: TEST(label, "MIX035:%x|%p|%d|%i|%c|%s|%u|%X|%%", u2, p1, d1, d2, c1, s1, u1, u3); break;
            case 35: TEST(label, "MIX036:%s|%x|%X|%c|%d|%i|%p|%u|%%", s1, u2, u3, c1, d1, d2, p1, u1); break;
            case 36: TEST(label, "MIX037:%u|%p|%s|%d|%X|%x|%i|%c|%%", u1, p1, s1, d1, u3, u2, d2, c1); break;
            case 37: TEST(label, "MIX038:%x|%i|%s|%X|%d|%c|%u|%p|%%", u2, d2, s1, u3, d1, c1, u1, p1); break;
            case 38: TEST(label, "MIX039:%s|%d|%i|%X|%p|%u|%x|%c|%%", s1, d1, d2, u3, p1, u1, u2, c1); break;
            case 39: TEST(label, "MIX040:%u|%X|%i|%d|%p|%c|%x|%s|%%", u1, u3, d2, d1, p1, c1, u2, s1); break;
            case 40: TEST(label, "MIX041:%u|%i|%p|%X|%c|%s|%d|%x|%%", u1, d2, p1, u3, c1, s1, d1, u2); break;
            case 41: TEST(label, "MIX042:%i|%u|%p|%x|%s|%X|%c|%d|%%", d2, u1, p1, u2, s1, u3, c1, d1); break;
            case 42: TEST(label, "MIX043:%d|%u|%p|%X|%c|%i|%s|%x|%%", d1, u1, p1, u3, c1, d2, s1, u2); break;
            case 43: TEST(label, "MIX044:%u|%s|%X|%p|%i|%d|%c|%x|%%", u1, s1, u3, p1, d2, d1, c1, u2); break;
            case 44: TEST(label, "MIX045:%X|%x|%s|%p|%d|%u|%c|%i|%%", u3, u2, s1, p1, d1, u1, c1, d2); break;
            case 45: TEST(label, "MIX046:%d|%X|%c|%s|%p|%x|%i|%u|%%", d1, u3, c1, s1, p1, u2, d2, u1); break;
            case 46: TEST(label, "MIX047:%p|%s|%X|%x|%i|%u|%d|%c|%%", p1, s1, u3, u2, d2, u1, d1, c1); break;
            case 47: TEST(label, "MIX048:%x|%s|%p|%d|%i|%X|%u|%c|%%", u2, s1, p1, d1, d2, u3, u1, c1); break;
            case 48: TEST(label, "MIX049:%X|%c|%x|%d|%u|%i|%p|%s|%%", u3, c1, u2, d1, u1, d2, p1, s1); break;
            case 49: TEST(label, "MIX050:%X|%x|%c|%u|%p|%i|%d|%s|%%", u3, u2, c1, u1, p1, d2, d1, s1); break;
            case 50: TEST(label, "MIX051:%u|%X|%x|%d|%i|%s|%c|%p|%%", u1, u3, u2, d1, d2, s1, c1, p1); break;
            case 51: TEST(label, "MIX052:%x|%c|%s|%i|%X|%p|%u|%d|%%", u2, c1, s1, d2, u3, p1, u1, d1); break;
            case 52: TEST(label, "MIX053:%p|%c|%x|%d|%X|%s|%i|%u|%%", p1, c1, u2, d1, u3, s1, d2, u1); break;
            case 53: TEST(label, "MIX054:%X|%u|%x|%c|%s|%p|%d|%i|%%", u3, u1, u2, c1, s1, p1, d1, d2); break;
            case 54: TEST(label, "MIX055:%X|%x|%d|%i|%u|%c|%s|%p|%%", u3, u2, d1, d2, u1, c1, s1, p1); break;
            case 55: TEST(label, "MIX056:%i|%u|%x|%d|%c|%X|%p|%s|%%", d2, u1, u2, d1, c1, u3, p1, s1); break;
            case 56: TEST(label, "MIX057:%i|%X|%d|%p|%u|%s|%c|%x|%%", d2, u3, d1, p1, u1, s1, c1, u2); break;
            case 57: TEST(label, "MIX058:%i|%x|%d|%p|%X|%s|%u|%c|%%", d2, u2, d1, p1, u3, s1, u1, c1); break;
            case 58: TEST(label, "MIX059:%X|%d|%s|%u|%c|%x|%p|%i|%%", u3, d1, s1, u1, c1, u2, p1, d2); break;
            case 59: TEST(label, "MIX060:%x|%s|%d|%p|%i|%c|%X|%u|%%", u2, s1, d1, p1, d2, c1, u3, u1); break;
            case 60: TEST(label, "MIX061:%u|%x|%i|%d|%s|%X|%p|%c|%%", u1, u2, d2, d1, s1, u3, p1, c1); break;
            case 61: TEST(label, "MIX062:%p|%X|%x|%c|%d|%i|%s|%u|%%", p1, u3, u2, c1, d1, d2, s1, u1); break;
            case 62: TEST(label, "MIX063:%x|%s|%c|%p|%X|%i|%u|%d|%%", u2, s1, c1, p1, u3, d2, u1, d1); break;
            case 63: TEST(label, "MIX064:%c|%p|%u|%s|%X|%d|%i|%x|%%", c1, p1, u1, s1, u3, d1, d2, u2); break;
            case 64: TEST(label, "MIX065:%d|%X|%u|%i|%p|%c|%s|%x|%%", d1, u3, u1, d2, p1, c1, s1, u2); break;
            case 65: TEST(label, "MIX066:%s|%x|%d|%c|%p|%X|%u|%i|%%", s1, u2, d1, c1, p1, u3, u1, d2); break;
            case 66: TEST(label, "MIX067:%i|%c|%x|%s|%d|%u|%X|%p|%%", d2, c1, u2, s1, d1, u1, u3, p1); break;
            case 67: TEST(label, "MIX068:%s|%c|%x|%u|%X|%d|%i|%p|%%", s1, c1, u2, u1, u3, d1, d2, p1); break;
            case 68: TEST(label, "MIX069:%s|%p|%x|%X|%u|%d|%c|%i|%%", s1, p1, u2, u3, u1, d1, c1, d2); break;
            case 69: TEST(label, "MIX070:%d|%X|%p|%x|%u|%s|%i|%c|%%", d1, u3, p1, u2, u1, s1, d2, c1); break;
            case 70: TEST(label, "MIX071:%s|%d|%p|%x|%i|%X|%u|%c|%%", s1, d1, p1, u2, d2, u3, u1, c1); break;
            case 71: TEST(label, "MIX072:%d|%X|%c|%p|%i|%s|%x|%u|%%", d1, u3, c1, p1, d2, s1, u2, u1); break;
            case 72: TEST(label, "MIX073:%u|%c|%s|%d|%X|%x|%p|%i|%%", u1, c1, s1, d1, u3, u2, p1, d2); break;
            case 73: TEST(label, "MIX074:%u|%p|%s|%X|%d|%x|%i|%c|%%", u1, p1, s1, u3, d1, u2, d2, c1); break;
            case 74: TEST(label, "MIX075:%u|%p|%i|%X|%c|%d|%s|%x|%%", u1, p1, d2, u3, c1, d1, s1, u2); break;
            case 75: TEST(label, "MIX076:%c|%d|%s|%X|%u|%i|%p|%x|%%", c1, d1, s1, u3, u1, d2, p1, u2); break;
            case 76: TEST(label, "MIX077:%u|%d|%s|%X|%i|%x|%c|%p|%%", u1, d1, s1, u3, d2, u2, c1, p1); break;
            case 77: TEST(label, "MIX078:%X|%d|%x|%s|%i|%p|%u|%c|%%", u3, d1, u2, s1, d2, p1, u1, c1); break;
            case 78: TEST(label, "MIX079:%u|%s|%d|%i|%X|%p|%c|%x|%%", u1, s1, d1, d2, u3, p1, c1, u2); break;
            case 79: TEST(label, "MIX080:%u|%i|%x|%c|%s|%d|%p|%X|%%", u1, d2, u2, c1, s1, d1, p1, u3); break;
            case 80: TEST(label, "MIX081:%p|%s|%x|%c|%u|%i|%d|%X|%%", p1, s1, u2, c1, u1, d2, d1, u3); break;
            case 81: TEST(label, "MIX082:%s|%c|%x|%i|%X|%u|%d|%p|%%", s1, c1, u2, d2, u3, u1, d1, p1); break;
            case 82: TEST(label, "MIX083:%c|%s|%u|%x|%d|%p|%i|%X|%%", c1, s1, u1, u2, d1, p1, d2, u3); break;
            case 83: TEST(label, "MIX084:%d|%u|%i|%X|%p|%s|%c|%x|%%", d1, u1, d2, u3, p1, s1, c1, u2); break;
            case 84: TEST(label, "MIX085:%x|%p|%d|%X|%s|%i|%c|%u|%%", u2, p1, d1, u3, s1, d2, c1, u1); break;
            case 85: TEST(label, "MIX086:%x|%i|%c|%X|%s|%d|%u|%p|%%", u2, d2, c1, u3, s1, d1, u1, p1); break;
            case 86: TEST(label, "MIX087:%s|%p|%d|%X|%x|%u|%c|%i|%%", s1, p1, d1, u3, u2, u1, c1, d2); break;
            case 87: TEST(label, "MIX088:%s|%c|%p|%u|%x|%X|%i|%d|%%", s1, c1, p1, u1, u2, u3, d2, d1); break;
            case 88: TEST(label, "MIX089:%d|%u|%p|%i|%s|%X|%x|%c|%%", d1, u1, p1, d2, s1, u3, u2, c1); break;
            case 89: TEST(label, "MIX090:%s|%p|%X|%c|%x|%d|%i|%u|%%", s1, p1, u3, c1, u2, d1, d2, u1); break;
            case 90: TEST(label, "MIX091:%c|%X|%d|%p|%s|%i|%x|%u|%%", c1, u3, d1, p1, s1, d2, u2, u1); break;
            case 91: TEST(label, "MIX092:%X|%x|%s|%c|%p|%d|%i|%u|%%", u3, u2, s1, c1, p1, d1, d2, u1); break;
            case 92: TEST(label, "MIX093:%u|%i|%d|%p|%X|%x|%c|%s|%%", u1, d2, d1, p1, u3, u2, c1, s1); break;
            case 93: TEST(label, "MIX094:%d|%c|%p|%i|%x|%X|%u|%s|%%", d1, c1, p1, d2, u2, u3, u1, s1); break;
            case 94: TEST(label, "MIX095:%u|%d|%c|%x|%p|%X|%i|%s|%%", u1, d1, c1, u2, p1, u3, d2, s1); break;
            case 95: TEST(label, "MIX096:%x|%s|%c|%u|%p|%d|%X|%i|%%", u2, s1, c1, u1, p1, d1, u3, d2); break;
            case 96: TEST(label, "MIX097:%c|%s|%i|%u|%X|%p|%x|%d|%%", c1, s1, d2, u1, u3, p1, u2, d1); break;
            case 97: TEST(label, "MIX098:%d|%s|%X|%p|%u|%i|%c|%x|%%", d1, s1, u3, p1, u1, d2, c1, u2); break;
            case 98: TEST(label, "MIX099:%c|%i|%p|%X|%s|%x|%d|%u|%%", c1, d2, p1, u3, s1, u2, d1, u1); break;
            case 99: TEST(label, "MIX100:%c|%d|%p|%s|%i|%x|%u|%X|%%", c1, d1, p1, s1, d2, u2, u1, u3); break;
        }
        i++;
    }

    /* 426-480: 55 progressively larger strings; stresses %s length/copy loops. */
    i = 0;
    while (i < 55)
    {
        length = 8192u + (size_t)i * 3071u;
        dynamic_text = make_text(length, (unsigned int)(i + 101));
        if (dynamic_text == NULL)
            return (2);
        snprintf(label, sizeof(label), "long string stress length %zu", length);
        TEST(label, "<%s>", dynamic_text);
        free(dynamic_text);
        i++;
    }

    /* 481-500: 20 deliberate stress cases using valid mandatory inputs. */
    i = 0;
    while (i < 10)
    {
        length = 131072u + (size_t)i * 65537u;
        dynamic_text = make_text(length, (unsigned int)(i + 211));
        if (dynamic_text == NULL)
            return (2);
        snprintf(label, sizeof(label), "KILLER %02d: very large %%s length %zu", i + 1, length);
        TEST(label, "%s", dynamic_text);
        free(dynamic_text);
        i++;
    }

    large_a = make_text(262144u, 17u);
    large_b = make_text(393217u, 29u);
    large_c = make_text(131071u, 41u);
    if (large_a == NULL || large_b == NULL || large_c == NULL)
        return (2);
    TEST("KILLER 11: two large strings in one call", "[%s][%s]", large_a, large_b);
    TEST("KILLER 12: three long strings around numeric conversions",
        "%s|%d|%s|%u|%s", large_a, INT_MIN, large_b, UINT_MAX, large_c);

    TEST("KILLER 13: 40 signed conversions in one call",
        "%d %i %d %i %d %i %d %i %d %i %d %i %d %i %d %i %d %i %d %i "
        "%d %i %d %i %d %i %d %i %d %i %d %i %d %i %d %i %d %i %d %i",
        INT_MIN, INT_MAX, -1, 0, 1, -42, 42, -1337, 1337, -999999,
        999999, -1000000000, 1000000000, -2147483647, 2147483646,
        -32768, 32767, -255, 255, -128, 127, -64, 64, -9, 9,
        -7, 7, -2, 2, -100, 100, -50000, 50000, -123456789, 123456789,
        -987654321, 987654321, INT_MIN + 1, INT_MAX - 1, 0);

    TEST("KILLER 14: 40 unsigned decimal conversions in one call",
        "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u "
        "%u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u",
        0u, 1u, 2u, 9u, 10u, 15u, 16u, 255u, 256u, 65535u,
        65536u, 100000u, 1000000u, 10000000u, 100000000u,
        UINT_MAX / 2u, UINT_MAX / 2u + 1u, UINT_MAX - 1u, UINT_MAX,
        (unsigned int)-1, (unsigned int)-42, 42u, 1337u, 777777u,
        0x01010101u, 0x10101010u, 0x11111111u, 0x12345678u, 0x87654321u,
        0xdeadbeefu, 0xabcdef01u, 0x00ff00ffu, 0xff00ff00u,
        0x55555555u, 0xaaaaaaaau, 0x0f0f0f0fu, 0xf0f0f0f0u,
        0x7f7f7f7fu, 0x80808080u, UINT_MAX - 12345u);

    TEST("KILLER 15: 40 alternating lower/upper hexadecimal conversions",
        "%x %X %x %X %x %X %x %X %x %X %x %X %x %X %x %X %x %X %x %X "
        "%x %X %x %X %x %X %x %X %x %X %x %X %x %X %x %X %x %X %x %X",
        0u, 1u, 0x0fu, 0x10u, 0xffu, 0x100u, 0xfffu, 0x1000u,
        0xffffu, 0x10000u, UINT_MAX / 2u, UINT_MAX / 2u + 1u,
        UINT_MAX - 1u, UINT_MAX, 0x12345678u, 0x87654321u,
        0xdeadbeefu, 0xabcdef01u, 0x00ff00ffu, 0xff00ff00u,
        0x55555555u, 0xaaaaaaaau, 0x0f0f0f0fu, 0xf0f0f0f0u,
        0x7f7f7f7fu, 0x80808080u, 0x01010101u, 0x10101010u,
        0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u,
        0x66666666u, 0x77777777u, 0x88888888u, 0x99999999u,
        0xbbbbbbbbu, 0xccccccccu, 0xddddddddu, 0xeeeeeeeeu);

    TEST("KILLER 16: 32 characters including NUL and control bytes",
        "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
        0, 1, 2, 7, 8, 9, 10, 13, 27, 31, 32, 33, 47, 48, 57, 64,
        65, 90, 97, 122, 126, 127, 128, 129, 254, 255, -1, -42,
        256, 257, INT_MIN, INT_MAX);

    TEST("KILLER 17: 32 valid object pointers in one call",
        "%p %p %p %p %p %p %p %p %p %p %p %p %p %p %p %p "
        "%p %p %p %p %p %p %p %p %p %p %p %p %p %p %p %p",
        (void *)&pointer_ints[0], (void *)&pointer_ints[1],
        (void *)&pointer_ints[2], (void *)&pointer_ints[3],
        (void *)&pointer_ints[4], (void *)&pointer_ints[5],
        (void *)&pointer_ints[6], (void *)&pointer_ints[7],
        (void *)&pointer_ints[8], (void *)&pointer_ints[9],
        (void *)&pointer_ints[10], (void *)&pointer_ints[11],
        (void *)&pointer_ints[12], (void *)&pointer_ints[13],
        (void *)&pointer_ints[14], (void *)&pointer_ints[15],
        (void *)&pointer_ints[16], (void *)&pointer_ints[17],
        (void *)&pointer_ints[18], (void *)&pointer_ints[19],
        (void *)&pointer_ints[20], (void *)&pointer_ints[21],
        (void *)&pointer_ints[22], (void *)&pointer_ints[23],
        (void *)&pointer_ints[24], (void *)&pointer_ints[25],
        (void *)&pointer_ints[26], (void *)&pointer_ints[27],
        (void *)&pointer_ints[28], (void *)&pointer_ints[29],
        (void *)&pointer_ints[30], (void *)&pointer_ints[31]);

    TEST("KILLER 18: every mandatory conversion in a long mixed call",
        "%c|%s|%p|%d|%i|%u|%x|%X|%%|%c|%s|%p|%d|%i|%u|%x|%X",
        'Z', large_c, (void *)&pointer_longs[0], INT_MIN, INT_MAX,
        UINT_MAX, UINT_MAX, UINT_MAX, '!', large_a,
        (void *)&pointer_longs[1], -1, 0, 0u, 0u, 0u);

    dynamic_format = (char *)malloc(30001u);
    expected_format = (char *)malloc(20001u);
    if (dynamic_format == NULL || expected_format == NULL)
        return (2);
    j = 0;
    length = 0;
    while (j < 10000u)
    {
        dynamic_format[length++] = 'A';
        dynamic_format[length++] = '%';
        dynamic_format[length++] = '%';
        expected_format[j * 2u] = 'A';
        expected_format[j * 2u + 1u] = '%';
        j++;
    }
    dynamic_format[length] = '\0';
    expected_format[20000u] = '\0';
    TEST_FMT("KILLER 19: 10000 escaped percent pairs in a dynamic format",
        dynamic_format, expected_format);
    free(dynamic_format);
    free(expected_format);

    dynamic_format = make_plain_format(500000u, 73u);
    if (dynamic_format == NULL)
        return (2);
    TEST_FMT("KILLER 20: half-million-character dynamic format scan",
        dynamic_format, dynamic_format);
    free(dynamic_format);

    free(large_a);
    free(large_b);
    free(large_c);
    unlink(g_ft_path);
    unlink(g_real_path);
    print_summary();
    if (g_test_id != EXPECTED_TESTS)
    {
        printf("ERROR: expected %d tests but executed %d\n", EXPECTED_TESTS, g_test_id);
        return (2);
    }
    return (g_failed == 0 ? 0 : 1);
}
