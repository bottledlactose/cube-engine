#pragma once

#include <cstdio>

// Some simple logging macros
#define LOG_INFO(...) printf(__VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
