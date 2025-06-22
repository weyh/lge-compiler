/******************************
    LGE Runtime Library
    Provides implementations for built-in functions
********************************/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define BUFFER_SIZE UCHAR_MAX

static char glob_buffer[BUFFER_SIZE]; // TODO have heap alloc mem

int str_print(const char *str) {
  fputs(str, stdout);
  return 0;
}

char *str_read(int n) {
  // n must be less then BUFFER_SIZE
  n = MIN(BUFFER_SIZE, n + 1);
  glob_buffer[0] = '\0';

  if (fgets(glob_buffer, n, stdin)) {
    // Remove newline if present
    const size_t len = strlen(glob_buffer);
    if (len > 0 && glob_buffer[len - 1] == '\n') {
      glob_buffer[len - 1] = '\0';
    }
  }

  return glob_buffer;
}

int str_len(const char *str) { return (int)strlen(str); }

char str_at(const char *str, int index) {
  if (!str || index < 0 || index >= strlen(str)) {
    return '\0';
  }
  return str[index];
}

char *str_sub(const char *str, int start, int end) {
  glob_buffer[0] = '\0';
  if (!str)
    return glob_buffer;

  int len = strlen(str);
  if (start < 0 || end < start || start >= len) {
    return glob_buffer; // Return empty string
  }

  if (end > len)
    end = len;

  int sublen = end - start;

  strncpy(glob_buffer, str + start, sublen);
  glob_buffer[sublen] = '\0';

  return glob_buffer;
}

int str_find(const char *haystack, const char *needle) {
  if (!haystack || !needle)
    return -1;

  char *found = strstr(haystack, needle);
  if (found) {
    return found - haystack;
  }

  return -1;
}

char *int_to_str(int value) {
  sprintf(glob_buffer, "%d", value);
  return glob_buffer;
}

int str_to_int(const char *str) {
  if (!str)
    return 0;
  return atoi(str);
}

char *float_to_str(float value) {
  sprintf(glob_buffer, "%f", value);
  return glob_buffer;
}

float str_to_float(const char *str) {
  if (!str)
    return 0.0f;
  return atof(str);
}

int str_cmp(const char *a, const char *b) {
  return strcmp(a, b) == 0;
}
