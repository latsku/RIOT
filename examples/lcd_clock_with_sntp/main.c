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
#include "thread.h"

#include "net/af.h"
#include "net/gnrc.h"
#include "net/conn.h"
#include "net/conn/udp.h"

#include "sntp.h"

#include "hd44780.h"

#define TM_YEAR_OFFSET              (1900)

#define LCD_ROWS                    (4)
#define LCD_COLUMNS                (20)

#define TIMEDIFS                    (1)
#define LCD_ALARM_CB                (0)

/* Private variables ---------------------------------------------------------*/
// const char * defaultServerAddress = "2.pool.ntp.org";
const char * defaultServerIPv6Address = "fe80::219:e3ff:fe43:659c"; //2001:1bc8:101:e601::1"; //
const char * defaultNetworkPort    = "123";
static const int ITERATIONS = 1;    /* One iteration as there is no method to
                                       compensate for network round-trip time or
                                       other sources of jitter. */


#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

char lcd_thread_stack[THREAD_STACKSIZE_MAIN];
#ifdef SNTP_THREAD
char sNTP_stack[THREAD_STACKSIZE_MAIN];
#endif

int sntp_cmd(int argc, char** argv);

static const shell_command_t shell_commands[] = {
    { "sntp", "retrieve time over sNTP", sntp_cmd },
    { NULL, NULL, NULL }
};

#ifdef LCD_ALARM_CB

void lcd_cb(void *arg)
{
    disableIRQ();
    (void)arg;

    puts("Alarm!");

    struct tm curr_time;
    rtc_get_time(&curr_time);
    LCD_home();

    char string[LCD_COLUMNS] = { 0 };
    snprintf(string, LCD_COLUMNS, "%04d-%02d-%02d %02d:%02d:%02d",
                        curr_time.tm_year + TM_YEAR_OFFSET,
                        curr_time.tm_mon + 1,
                        curr_time.tm_mday,
                        curr_time.tm_hour,
                        curr_time.tm_min,
                        curr_time.tm_sec);

    for (int i=0; i<19; i++)
        LCD_write(*(string+i));


    curr_time.tm_sec  += 10;
    rtc_set_alarm(&curr_time, lcd_cb, 0);

    puts("Alarm set");
//    printf("Alarm set to %02d:%02d:%02d.\n",
//                        curr_time.tm_hour,
//                        curr_time.tm_min,
//                        curr_time.tm_sec);
    enableIRQ();
    return;
}
#endif

void *thread_lcd(void *arg)
{
    struct tm curr_time;
    LCD_clear();

#ifdef LCD_ALARM_CB
    rtc_get_time(&curr_time);
    curr_time.tm_sec  += 10;
    rtc_set_alarm(&curr_time, lcd_cb, 0);

    puts("Alarm set");
//    printf("Alarm set to %02d:%02d:%02d.\n",
//                        curr_time.tm_hour,
//                        curr_time.tm_min,
//                        curr_time.tm_sec);

    thread_yield();

#else
    while(1) {
        rtc_get_time(&curr_time);

        LCD_home();

        char string[LCD_COLUMNS] = { 0 };
        snprintf(string, LCD_COLUMNS, "%04d-%02d-%02d %02d:%02d:%02d",
                                curr_time.tm_year + TM_YEAR_OFFSET,
                                curr_time.tm_mon + 1,
                                curr_time.tm_mday,
                                curr_time.tm_hour,
                                curr_time.tm_min,
                                curr_time.tm_sec);
        for (int i=0; i<19; i++)
            LCD_write(*(string+i));

        xtimer_usleep(1000000);
    }
#endif
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
    struct tm *recv_time;
    char result_addr_str[IPV6_ADDR_MAX_STR_LEN];
#ifdef TIMEDIFS
    timex_t start, stop;
#endif
    for (int i = 0; i < ITERATIONS; i++ ) {
#ifdef TIMEDIFS
        xtimer_now_timex(&start);
#endif

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

        ipv6_addr_to_str(result_addr_str, &remote_addr, IPV6_ADDR_MAX_STR_LEN);

#ifdef TIMEDIFS
        xtimer_now_timex(&stop);
        stop = timex_sub(stop, start);
        timex_normalize(&stop);
#endif

        t = sntpReadTimestamp((void *)data);
        recv_time = localtime(&t);
//        recv_time->tm_hour += 2;
        if ( err == 0 ) {
            t += 60*60*2;
            recv_time = localtime(&t);
            rtc_set_time(recv_time);
            t -= 60*60*2;
            recv_time = localtime(&t);
        }
        if ( i < (ITERATIONS-1))
            xtimer_sleep(MINPOLL*MINPOLL);

        //rtc_get_time(recv_time);
        //t = mktime(recv_time);
#ifdef TIMEDIFS
        sntpUpdatePacket((void *)data, t, stop.seconds);
#else
        sntpUpdatePacket((void *)data, t, 1);
#endif

    }

    conn_udp_close(&udp_connection);

    printf("Clock is %04d-%02d-%02d %02d:%02d:%02d received from %s.\n",
                                            recv_time->tm_year + TM_YEAR_OFFSET,
                                            recv_time->tm_mon + 1,
                                            recv_time->tm_mday,
                                            recv_time->tm_hour,
                                            recv_time->tm_min,
                                            recv_time->tm_sec,
                                            result_addr_str);

//    rtc_set_time(recv_time);

    return 0;
}

int sntp_cmd(int argc, char** argv) {

    puts("Start sNTP client... ");

#ifdef SNTP_THREAD
    thread_create(sNTP_stack, sizeof(sNTP_stack),
                  THREAD_PRIORITY_MAIN, 0,
                  sntp_query, NULL, "sNTP");
#else
    sntp_query(NULL);
#endif

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
    puts("LCD Clock with sNTP client !!!");

    LCD_init(GPIO_PIN(PORT_B, 8), 255, GPIO_PIN(PORT_B, 9), GPIO_PIN(PORT_A, 10), GPIO_PIN(PORT_B, 3), GPIO_PIN(PORT_B, 5), GPIO_PIN(PORT_B, 4));
    LCD_begin(LCD_COLUMNS, LCD_ROWS, LCD_5x10DOTS);


    thread_create(lcd_thread_stack, sizeof(lcd_thread_stack),
                    THREAD_PRIORITY_MAIN, 0,
                    thread_lcd, NULL, "lcd");

     sntp_query(NULL);


    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
