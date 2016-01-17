/**
 * Copyright (C) 2015 Lari Lehtomäki
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @file
 * @brief Tiny SNTP client library.
 *
 * @author Lari Lehtomäki <lari@lehtomaki.fi>
 */


/* Includes ------------------------------------------------------------------*/
#include <stdint.h> /* uint8_t */


/* Exported types ------------------------------------------------------------*/
typedef unsigned long long tstamp;   /* NTP timestamp format */

/* Packet structure */
typedef struct __attribute__ ((__packed__)) {

        uint8_t flags;          /* leap indicator, version number, mode */
        uint8_t stratum;        /* stratum */
        uint8_t poll;           /* poll interval */
        uint8_t precision;      /* precision */
        uint32_t rootdelay;     /* root delay */
        uint32_t rootdisp;      /* root dispersion */
        char refid[4];          /* reference ID */
        tstamp  reftime;        /* reference time */
        tstamp  org;            /* origin timestamp */
        tstamp  rec;            /* receive timestamp */
        tstamp  xmt;            /* transmit timestamp */
} ntp_t;


/* Exported constants --------------------------------------------------------*/
#define FRAC       		4294967296.	 /* 2^32 as a double */
#define JAN_1970        2208988800UL /* 1970 - 1900 in seconds */

#define VERSION         4       /* version number */
#define MINDISP         .01     /* % minimum dispersion (s) */
#define MAXDISP         16      /* maximum dispersion (s) */
#define MAXDIST         1       /* % distance threshold (s) */
#define NOSYNC          0x3     /* leap unsync */
#define MAXSTRAT        16      /* maximum stratum (infinity metric) */
#define MINPOLL         6       /* % minimum poll interval (64 s)*/
#define MAXPOLL         17      /* % maximum poll interval (36.4 h) */

#define PRECISION       -18     /* precision (log2 s)  */

/* Protocol mode definition */
#define M_CLNT          3       /* client */
#define M_SRV           4       /* server */


/* Exported macro ------------------------------------------------------------*/
#define D2LFP(a)   ((tstamp)((a) * FRAC))  /* NTP timestamp */


/* Exported functions ------------------------------------------------------- */
void sntpCreatePacket(void * buffer);
void sntpUpdatePacket(void * buffer, uint32_t timestamp);
int  sntpReadTimestamp(void * buffer);
