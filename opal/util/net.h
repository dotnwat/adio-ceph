/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

/* @file */

#ifndef OPAL_UTIL_NET_H
#define OPAL_UTIL_NET_H

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

BEGIN_C_DECLS


/**
 * Calculate netmask in network byte order from CIDR notation
 *
 * @param prefixlen (IN)  CIDR prefixlen
 * @return                netmask in network byte order
 */
OPAL_DECLSPEC uint32_t opal_net_prefix2netmask(uint32_t prefixlen);


/**
 * Determine if given IP address is in the localhost range
 *
 * Determine if the given IP address is in the localhost range
 * (127.0.0.0/8), meaning that it can't be used to connect to machines
 * outside the current host.
 *
 * @param addr             struct sockaddr_in of IP address
 * @return                 true if \c addr is a localhost address,
 *                         false otherwise.
 */
OPAL_DECLSPEC bool opal_net_islocalhost(struct sockaddr *addr);


/**
 * Are we on the same network?
 *
 * For IPv6, we only need to check for /64, there are no other
 * local netmasks.
 *
 * @param addr1             struct sockaddr of address
 * @param addr2             struct sockaddr of address
 * @param prefixlen         netmask (either CIDR oder IPv6 prefixlen)
 * @return                  true if \c addr1 and \c addr2 are on the
 *                          same net, false otherwise.
 */
OPAL_DECLSPEC bool opal_net_samenetwork(struct sockaddr *addr1,
                                        struct sockaddr *addr2,
                                        uint32_t prefixlen);


/**
 * Is the given address a public IPv4 address?  Returns false for IPv6
 * address.
 *
 * @param addr      address as struct sockaddr
 * @return          true, if \c addr is IPv4 public, false otherwise
 */
OPAL_DECLSPEC bool opal_net_addr_isipv4public(struct sockaddr *addr);


/**
 * Get string version of address
 *
 * Return the un-resolved address in a string format.  The string will
 * be created with malloc and the user must free the string.
 *
 * @param addr              struct sockaddr of address
 * @return                  literal representation of \c addr
 */
OPAL_DECLSPEC char* opal_net_get_hostname(struct sockaddr *addr);

OPAL_DECLSPEC int opal_net_get_port(struct sockaddr *addr);


END_C_DECLS

#endif /* OPAL_UTIL_NET_H */
