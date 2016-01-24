#ifndef CONVERT_H
#define CONVERT_H

int conv_errno;

extern const char* conv_strerror[];

double convert_value (double in_value, const char* src, const char* dest);

#endif
