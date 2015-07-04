/*
 * Copyright 2015 Lari Lehtomäki
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_shell_commands
 * @{
 *
 * @file
 * @brief       Shell command implementation for memory access
 *
 * @author      Lari Lehtomäki <lari@lehtomaki.fi>
 *
 * @}
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


static int _mem_read(char **argv)
{
    uint32_t * mem_ptr;
    mem_ptr = (uint32_t *)(strtol(argv[0], NULL, 0) & 0xfffffffc);
    printf("%p: %#010x\n", mem_ptr, *mem_ptr);
    return 0;
}

static int _mem_set(char **argv)
{
    uint32_t * mem_ptr;
    mem_ptr = (uint32_t *)strtol(argv[0], NULL, 0);
    uint32_t value = strtol(argv[1], NULL, 0);

    *mem_ptr = value;
    return 0;
}

static int _mem_usage(void)
{
    puts("usage: mem <command> [arguments]");
    puts("commands:");
    puts("\tread ADDRESS\n\t\t\tprint the memory content");
    puts("\tset ADDERSS VALUE\n\t\t\tset the memory content");
    return 0;
}

int _mem_handler(int argc, char **argv)
{
    if (argc < 2) {
        _mem_usage();
        return 1;
    }
    else if (strncmp(argv[1], "read", 4) == 0) {
        _mem_read(argv + 2);
    }
    else if (strncmp(argv[1], "set", 3) == 0) {
        _mem_set(argv + 2);
    }
    else {
        printf("unknown command: %s\n", argv[1]);
        return 1;
    }
    return 0;
}
