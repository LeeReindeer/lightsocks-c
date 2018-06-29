/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdio.h>

#define LOG_VERSION "0.1.0"

typedef void (*log_LockFn)(void *udata, int lock);

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define log_t(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_d(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_i(...) log_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_w(...) log_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_e(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_f(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define check(A, ...)                                                          \
  if (!(A)) {                                                                  \
    log_e(__VA_ARGS__);                                                        \
    goto error;                                                                \
  }

void log_set_udata(void *udata);
void log_set_lock(log_LockFn fn);
void log_set_fp(FILE *fp);
void log_set_level(int level);
void log_set_quiet(int enable);

void log_log(int level, const char *file, int line, const char *fmt, ...);

#endif
