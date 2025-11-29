/* util.h - Utility functions interface */

#ifndef UTIL_H
#define UTIL_H

#include "common.h"

/* Memory operations */
void *memset(void *s, int c, uint32_t n);
void *memcpy(void *dest, const void *src, uint32_t n);
int memcmp(const void *s1, const void *s2, uint32_t n);

/* String operations */
uint32_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, uint32_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint32_t n);

/* Number conversion */
int atoi(const char *str);
char *itoa(int value, char *str, int base);

/* Delay function */
void delay(uint32_t count);

#endif /* UTIL_H */
