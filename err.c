#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "err.h"


void fatal(const char *fmt, ...)
{
  va_list fmt_args;
    FILE* f;
  fprintf(stderr, "ERROR: ");
  va_start(fmt_args, fmt);
  vfprintf(stderr, fmt, fmt_args);
  va_end(fmt_args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void syserr(const char *fmt, ...) {
    va_list fmt_args;
    int errno1 = errno;

    fprintf(stderr, "ERROR: ");
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end(fmt_args);
    fprintf(stderr, " (%d; %s)\n", errno1, strerror(errno1));
    exit(EXIT_FAILURE);
}