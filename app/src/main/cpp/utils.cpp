//
// Created by eraise on 2017/11/2.
//

#include <cstdio>
#include <cstring>
#include <malloc.h>
#include "utils.h"

/**
 *
 * @return
 */
long utils::getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long)tv.tv_sec * 1000 + (long)tv.tv_usec / 1000;
}
