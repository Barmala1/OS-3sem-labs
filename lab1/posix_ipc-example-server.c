#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
//#include <stdio.h>
#include <ctype.h>

static int frmt_int(char *buf, int v) {
    char tmp[12];
    int i = 0;
    int j = 0;
    int u;

    if (v < 0) {
        buf[j] = '-';
        j = j + 1;
        u = -v;
    } else {
        u = v;
    }

    if (u == 0) {
        tmp[i] = '0';
        i = i + 1;
    }

    while (u > 0) {
        tmp[i] = '0' + (u % 10);
        i = i + 1;
        u = u / 10;
    }

    while (i > 0) {
        i = i - 1;
        buf[j] = tmp[i];
        j = j + 1;
    }

    return j;
}

static int frmt_msg(char *buf, float sum, int count) {
    int n = 0;
    const char *p;

    p = "sum = ";
    while (*p != '\0') {
        buf[n] = *p;
        n = n + 1;
        p = p + 1;
    }

    if (sum < 0) {
        buf[n] = '-';
        n = n + 1;
        sum = -sum;
    }

    long sc = (long)(sum * 100.0f + 0.5f);

    n = n + frmt_int(buf + n, (int)(sc / 100));

    buf[n] = '.';
    n = n + 1;

    buf[n] = '0' + (int)((sc / 10) % 10);
    n = n + 1;
    buf[n] = '0' + (int)(sc % 10);
    n = n + 1;

    p = " (numbers: ";
    while (*p != '\0') {
        buf[n] = *p;
        n = n + 1;
        p = p + 1;
    }

    n = n + frmt_int(buf + n, count);

    buf[n] = ')';
    n = n + 1;
    buf[n] = '\n';
    n = n + 1;
    buf[n] = '\0';

    return n;
}

static void process_line(const char *line) {
    float sum = 0.0f;
    int count = 0;
    const char *p = line;

    while (*p != '\0') {
        while (*p && (isspace((unsigned char)*p) || *p == ','))
            ++p;
        if (*p == '\0') break;

        char *endptr = NULL;
        float value = strtof(p, &endptr);
        if (endptr == p) { ++p; continue; }

        sum += value;
        ++count;
        p = endptr;
    }

    char msg[128];
    int32_t len = frmt_msg(msg, sum, count);
    write(STDOUT_FILENO, msg, len);
}

int main(void) {
    char buf[4096];
    ssize_t bytes;

    char line[4096];
    size_t linelen = 0;

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < bytes; i++) {
            char c = buf[i];

            if (c == '\n') {
                line[linelen] = '\0';
                process_line(line);
                linelen = 0;
            } else {
                if (linelen < sizeof(line) - 1) {
                    line[linelen++] = c;
                }
            }
        }
    }

    if (bytes < 0) {
        const char msg[] = "error: failed to read from stdin\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    if (linelen > 0) {
        line[linelen] = '\0';
        process_line(line);
    }

    return EXIT_SUCCESS;
}