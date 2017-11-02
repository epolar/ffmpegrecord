//
// Created by eraise on 2017/11/2.
//

#include <cstdio>
#include <cstring>
#include <malloc.h>
#include "utils.h"

long utils::getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
