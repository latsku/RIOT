/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Clock application with LCD display and sNTP client
 *
 * @author      Lari Lehtomäki <lari@lehtomaki.fi>
 *
 * @}
 */

#include <stdio.h>

#include "shell.h"
#include "msg.h"

#include <time.h>

#include "periph_conf.h"
#include "periph/rtc.h"
#include "xtimer.h"
#include "timex.h"

#include "net/af.h"
#include "net/gnrc.h"
#include "net/conn.h"
#include "net/conn/udp.h"

#include "sntp.h"

#include "hd44780.h"

#define TM_YEAR_OFFSET              (1900)



/* Private variables ---------------------------------------------------------*/
const char * defaultServerAddress = "2.pool.ntp.org";
const char * defaultServerIPv6Address = "fe80::219:e3ff:fe43:659c"; //2001:1bc8:101:e601::1"; //
const char * defaultNetworkPort    = "123";
static const int ITERATIONS = 3;


#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

char lcd_thread_stack[THREAD_STACKSIZE_MAIN];
char sNTP_stack[THREAD_STACKSIZE_MAIN];

int sntp_cmd(int argc, char** argv);

static const shell_command_t shell_commands[] = {
    { "sntp", "retrieve time over sNTP", sntp_cmd },
    { NULL, NULL, NULL }
};


void *update_lcd_cb(void *arg)
{
    struct tm time;
    LCD_clear();

    while(1) {
        rtc_get_time(&time);

        LCD_home();

        char string[8];
        sprintf(string, "%04d", time.tm_year + TM_YEAR_OFFSET);
        for (int i=0; i<4; i++) {
            LCD_write(*(string+i));
        }

        LCD_write('-');
        sprintf(string, "%02d", time.tm_mon + 1);
        for (int i=0; i<2; i++)
            LCD_write(*(string+i));

        LCD_write('-');
        sprintf(string, "%02d", time.tm_mday);
        for (int i=0; i<2; i++)
            LCD_write(*(string+i));


        LCD_write(' ');
        sprintf(string, "%02d", time.tm_hour);
        for (int i=0; i<2; i++) {
            LCD_write(*(string+i));
        }

        LCD_write(':');
        sprintf(string, "%02d", time.tm_min);
        for (int i=0; i<2; i++)
            LCD_write(*(string+i));

        LCD_write(':');
        sprintf(string, "%02d", time.tm_sec);
        for (int i=0; i<2; i++)
            LCD_write(*(string+i));


//      time.tm_sec  += 1;
//      rtc_set_alarm(&time, cb, 0);
        xtimer_usleep(1000000);
    }
    return NULL;
}

void *sntp_query(void *arg) {

    char data[48];
    size_t datalen = 48;

    sntpCreatePacket((void *)data);

    conn_udp_t udp_connection;
    ipv6_addr_t remote_addr;
    ipv6_addr_t local_addr;
    ipv6_addr_from_str(&remote_addr, defaultServerIPv6Address);
    memcpy(&local_addr, conn_find_best_source(&remote_addr), sizeof(ipv6_addr_t));
    uint16_t remote_port = 123;
    uint16_t local_port = 123;
    uint16_t err = 0;
//    ipv6_addr_t unspec = IPV6_ADDR_UNSPECIFIED;
    int rc = conn_udp_create(&udp_connection, &local_addr, sizeof(ipv6_addr_t),
                    AF_INET6, remote_port);
    if (rc < 0) {
        printf("Error %s:%d %d\n", __FILE__,__LINE__, rc);
        err += 1;
    }

    rc = conn_udp_getlocaladdr(&udp_connection, &local_addr, &local_port);
    if (rc < 0) {
        printf("Error %s:%d %d\n", __FILE__,__LINE__, rc);
        err += 1;
    }

    time_t t;
    struct tm *timestamp;
//    timex_t start, stop;
    for (int i = 0; i < ITERATIONS; i++ ) {
//        xtimer_now_timex(&start);

        rc = conn_udp_sendto((void *)data, datalen,
                        &local_addr, sizeof(ipv6_addr_t),
                        &remote_addr, sizeof(ipv6_addr_t),
                        AF_INET6, local_port, remote_port);
        if (rc < 0) {
            printf("Error %s:%d %d\n", __FILE__,__LINE__, rc);
            err += 1;
        }

        size_t addr_len = 0;
        rc = conn_udp_recvfrom(&udp_connection,
                            data, sizeof(data),
                            &remote_addr, &addr_len,
                            &remote_port);
        if (rc < 0) {
            printf("Error %s:%d %d\n", __FILE__,__LINE__, rc);
            err += 1;
        }

//        char result_addr_str[IPV6_ADDR_MAX_STR_LEN];
//        ipv6_addr_to_str(result_addr_str, &remote_addr, IPV6_ADDR_MAX_STR_LEN);


//        xtimer_now_timex(&stop);
//        stop = timex_sub(stop, start);

        t = sntpReadTimestamp((void *)data);
        timestamp = localtime(&t);
        timestamp->tm_hour += 2;
        if ( err == 0 )
            rtc_set_time(timestamp);
        xtimer_sleep(2);

        //rtc_get_time(timestamp);
        //t = mktime(timestamp);
        sntpUpdatePacket((void *)data, t);

    }

    conn_udp_close(&udp_connection);

    printf("Clock is %04d-%02d-%02d %02d:%02d:%02d\n",
                                            timestamp->tm_year + TM_YEAR_OFFSET,
                                            timestamp->tm_mon + 1,
                                            timestamp->tm_mday,
                                            timestamp->tm_hour,
                                            timestamp->tm_min,
                                            timestamp->tm_sec);

//    rtc_set_time(timestamp);

    return 0;
}

int sntp_cmd(int argc, char** argv) {

    puts("Start sNTP client... ");

    thread_create(sNTP_stack, sizeof(sNTP_stack),
                  THREAD_PRIORITY_MAIN, 0,
                  sntp_query, NULL, "sNTP");

    puts("done.");

    return 0;
}

int main(void)
{

#ifdef FEATURE_PERIPH_RTC
    rtc_init();
#endif

    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT network stack example application");

    LCD_init(GPIO_PIN(PORT_B, 8), 255, GPIO_PIN(PORT_B, 9), GPIO_PIN(PORT_A, 10), GPIO_PIN(PORT_B, 3), GPIO_PIN(PORT_B, 5), GPIO_PIN(PORT_B, 4));
    LCD_begin(20, 4, LCD_5x10DOTS);


    thread_create(lcd_thread_stack, sizeof(lcd_thread_stack), THREAD_PRIORITY_MAIN,
                  0, update_lcd_cb, NULL, "lcd");

    sntp_query(NULL);


    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
