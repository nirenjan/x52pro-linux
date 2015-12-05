/*
 * Saitek X52 Pro MFD & LED driver
 *
 * Copyright (C) 2012-2015 Nirenjan Krishnan (nirenjan@nirenjan.org)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, version 2.
 *
 */

#ifndef X52_TEST_COMMON_H
#define X52_TEST_COMMON_H

#include <stdio.h>
#include "libx52.h"

extern libx52_device *dev;
extern int test_exit;

#define TEST(tst, ...) do { \
    int rc; \
    rc = ( libx52_set_ ## tst (dev, __VA_ARGS__) ); \
    if (rc) { \
        fprintf(stderr, "\n%s(%s) failed with %d\n", #tst, #__VA_ARGS__, rc); \
        return rc; \
    } \
    if (test_exit) return test_exit; \
    rc  = libx52_update(dev); \
    if (rc) { \
        fprintf(stderr, "\nupdate failed with %d\n", rc); \
        return rc; \
    } \
    if (test_exit) return test_exit; \
} while (0)

void test_cleanup(void);

void print_banner(const char *message);

int test_brightness(void);
int test_mfd_display(void);
int test_mfd_text(void);
int test_leds(void);
int test_blink_n_shift(void);
int test_clock(void);

#endif /* X52_TEST_COMMON_H */
