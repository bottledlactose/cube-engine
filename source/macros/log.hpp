#pragma once

#include <cstdio>

#define LOG_INFO(...) printf(__VA_ARGS__)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
