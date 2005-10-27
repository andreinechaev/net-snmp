/******************************************************************
	Copyright 1989, 1991, 1992 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
/*
 * Copyright (c) 1983, 1988, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <net-snmp/net-snmp-config.h>

#ifndef lint
char            copyright[] =
    "@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif                          /* not lint */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <net-snmp/utilities.h>

#include <sys/types.h>
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if TIME_WITH_SYS_TIME
# ifdef WIN32
#  include <sys/timeb.h>
# else
#  include <sys/time.h>
# endif
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if HAVE_WINSOCK_H
#include <winsock.h>
#include "winstub.h"
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <net-snmp/net-snmp-includes.h>
#include "netstat.h"

#define NULLPROTOX	((struct protox *) 0)
struct protox {
    u_char          pr_wanted;  /* 1 if wanted, 0 otherwise */
    void            (*pr_cblocks) (const char *);       /* control blocks printing routine */
    void            (*pr_stats) (void); /* statistics printing routine */
    const char     *pr_name;    /* well-known name */
} protox[] = {
    {
    0, protopr, tcp_stats, "tcp"}, {
    0, protopr, udp_stats, "udp"},
#ifdef INET6
    {
    0, protopr6, tcp_stats, "tcp6"}, {
    0, protopr6, udp_stats, "udp6"},
#endif
    {
    0, 0, ip_stats, "ip"}, {
    0, 0, icmp_stats, "icmp"}, {
    0, 0, 0, 0}
};

int             aflag;
int             iflag;
int             oflag;
int             nflag;
int             rflag;
int             sflag;
int             interval;
char           *intrface;

netsnmp_session *Session;
int             print_errors = 0;

static struct protoent *getprotoent46(void);

void
usage(void)
{
    fprintf(stderr,
            "Usage: snmpnetstat [options...] hostname [interval]\n");
    fprintf(stderr, "NET-SNMP version: %s\n", netsnmp_get_version());
    fprintf(stderr, "  -v [1 | 2c ]   SNMP version\n");
    fprintf(stderr, "  -V             display version number\n");
    fprintf(stderr, "  -c community   specify community name\n");
    fprintf(stderr, "  -t timeout     SNMP packet timeout (seconds)\n");
    fprintf(stderr,
            "  -i             show interfaces with packet counters\n");
    fprintf(stderr,
            "  -o             show interfaces with octet counters\n");
    fprintf(stderr, "  -r             show routing table\n");
    fprintf(stderr, "  -s             show general statistics\n");
    fprintf(stderr, "  -n             show IP addresses, not names\n");
    fprintf(stderr, "  -a             show sockets in LISTEN mode too\n");
    fprintf(stderr,
            "  -P proto       show only details for this protocol\n");
    fprintf(stderr, "  -I interface   show only this interface\n");
    fprintf(stderr, "  -d             dump packets\n");
    fprintf(stderr, "  -Ddebugspec    \n");

}

int
main(int argc, char *argv[])
{
    char           *hostname = NULL;
    struct protoent *p;
    struct protox  *tp = NULL;  /* for printing cblocks & stats */
    int             allprotos = 1;
    char           *community = NULL;
    char           *argp;
    netsnmp_session session;
    int             timeout = SNMP_DEFAULT_TIMEOUT;
    int             version = SNMP_DEFAULT_VERSION;
    int             arg;

#ifndef DISABLE_MIB_LOADING
    init_mib();
#endif /* DISABLE_MIB_LOADING */
    /*
     * Usage: snmpnetstatwalk -v 1 [-q] hostname community ...      or:
     * Usage: snmpnetstat [-v 2 ] [-q] hostname noAuth     ...
     */
    while ((arg = getopt(argc, argv, "VhdqD:t:c:v:aionrsP:I:")) != EOF) {
        switch (arg) {
        case 'V':
            fprintf(stderr, "NET-SNMP version: %s\n",
                    netsnmp_get_version());
            exit(0);
            break;

        case 'h':
            usage();
            exit(0);

        case 'd':
            snmp_set_dump_packet(1);
            break;

        case 'q':
            snmp_set_quick_print(1);
            break;

        case 'D':
            debug_register_tokens(optarg);
            snmp_set_do_debugging(1);
            break;

        case 't':
            timeout = atoi(optarg);
            timeout *= 1000000;
            break;

        case 'c':
            community = optarg;
            break;

        case 'v':
            argp = optarg;
            if (!strcasecmp(argp, "1"))
                version = SNMP_VERSION_1;
            else if (!strcasecmp(argp, "2c"))
                version = SNMP_VERSION_2c;
            else {
                fprintf(stderr, "Invalid version: %s\n", argp);
                usage();
                exit(1);
            }
            break;

        case 'a':
            aflag++;
            break;

        case 'i':
            iflag++;
            break;

        case 'o':
            oflag++;
            break;

        case 'n':
            nflag++;
            break;

        case 'r':
            rflag++;
            break;

        case 's':
            sflag++;
            break;

        case 'P':
            if ((tp = name2protox(optarg)) == NULLPROTOX) {
                fprintf(stderr, "%s: unknown or uninstrumented protocol\n",
                        optarg);
                exit(1);
            }
            allprotos = 0;
            tp->pr_wanted = 1;
            break;

        case 'I':
            iflag++;
            intrface = optarg;
            break;

        default:
            exit(1);
            break;
        }
        continue;
    }

    init_snmp("snmpapp");
    snmp_enable_stderrlog();
    if (version == SNMP_DEFAULT_VERSION) {
        version = netsnmp_ds_get_int(NETSNMP_DS_LIBRARY_ID,
		                     NETSNMP_DS_LIB_SNMPVERSION);
        if (!version) {
            switch (DEFAULT_SNMP_VERSION) {
            case 1:
                version = SNMP_VERSION_1;
                break;
            case 2:
                version = SNMP_VERSION_2c;
                break;
            case 3:
                version = SNMP_VERSION_3;
                break;
            }
        } else if (version == NETSNMP_DS_SNMP_VERSION_1) {
                          /* Bogus value. version1 = 0 */
            version = SNMP_VERSION_1;
        }
    }
    if (optind < argc) {
        hostname = argv[optind++];
    }
    else {
        fprintf(stderr, "Missing host name.\n");
        exit(1);
    }
    if (community == NULL) {
	community = netsnmp_ds_get_string(NETSNMP_DS_LIBRARY_ID,
		                          NETSNMP_DS_LIB_COMMUNITY);
    }
    if (optind < argc && isdigit(argv[optind][0])) {
        interval = atoi(argv[optind++]);
        if (interval <= 0) {
            usage();
            exit(1);
        }
        iflag++;
    }
    if (optind < argc) {
        usage();
        exit(1);
    }


    snmp_sess_init(&session);
    session.peername = hostname;
    session.timeout = timeout;
    if (version == SNMP_VERSION_1 || version == SNMP_VERSION_2c) {
        if (!community) {
            fprintf(stderr, "Missing community name.\n");
            exit(1);
        }
        session.version = version;
        session.community = (u_char *) community;
        session.community_len = strlen(community);
    }

    SOCK_STARTUP;

    /*
     * open an SNMP session 
     */
    Session = snmp_open(&session);
    if (Session == NULL) {
        /*
         * diagnose snmp_open errors with the input netsnmp_session pointer 
         */
        snmp_sess_perror("snmpnetstat", &session);
        SOCK_CLEANUP;
        exit(1);
    }

    /*
     * Keep file descriptors open to avoid overhead
     * of open/close on each call to get* routines.
     */
    sethostent(1);
    setnetent(1);
    setprotoent(1);
    setservent(1);

    if (iflag) {
        intpr(interval);
    }
    if (oflag) {
        intpro(interval);
    }
    if (rflag) {
        if (sflag)
            rt_stats();
        else
            routepr();
    }

    if (!(iflag || rflag || oflag)) {
        while ((p = getprotoent46())) {
            for (tp = protox; tp->pr_name; tp++) {
                if (strcmp(tp->pr_name, p->p_name) == 0)
                    break;
            }
            if (tp->pr_name == 0 || (tp->pr_wanted == 0 && allprotos == 0))
                continue;
            if (sflag) {
                if (tp->pr_stats)
                    (*tp->pr_stats) ();
            } else if (tp->pr_cblocks)
                (*tp->pr_cblocks) (tp->pr_name);
        }
    }                           /* ! iflag, rflag, oflag */

    endprotoent();
    endservent();
    endnetent();
    endhostent();

    snmp_close(Session);

    SOCK_CLEANUP;
    return 0;
}

const char     *
plural(int n)
{
    return (n != 1 ? "s" : "");
}

/*
 * Find the protox for the given "well-known" name.
 */
struct protox  *
knownname(const char *name)
{
    struct protox  *tp;

    for (tp = protox; tp->pr_name; tp++)
        if (strcmp(tp->pr_name, name) == 0)
            return (tp);
    return (NULLPROTOX);
}

/*
 * Find the protox corresponding to name.
 */
struct protox  *
name2protox(const char *name)
{
    struct protox  *tp;
    char          **alias;      /* alias from p->aliases */
    struct protoent *p;

    /*
     * Try to find the name in the list of "well-known" names. If that
     * fails, check if name is an alias for an Internet protocol.
     */
    if ((tp = knownname(name)))
        return (tp);

    setprotoent(1);             /* make protocol lookup cheaper */
    while ((p = getprotoent46())) {
        /*
         * assert: name not same as p->name 
         */
        for (alias = p->p_aliases; *alias; alias++)
            if (strcasecmp(name, *alias) == 0) {
                endprotoent();
                return (knownname(p->p_name));
            }
    }
    endprotoent();
    return (NULLPROTOX);
}

static struct protoent *
getprotoent46(void)
{
#ifdef INET6
    static enum { NONE, V4, V6 } state = NONE;
    static struct protoent v;
    struct protoent *p;
    int             l;

    switch (state) {
    case NONE:
        p = getprotoent();
        if (!p)
            return p;
        memcpy(&v, p, sizeof(v));
        state = V4;
        break;
    case V4:
        strcat(v.p_name, "4");
        state = V6;
        break;
    case V6:
        l = strlen(v.p_name);
        v.p_name[l - 1] = '6';
        state = NONE;
        break;
    }
    return &v;
#else
    return getprotoent();
#endif
}
