Copyright (c) 2010 Cisco Systems, Inc.  All rights reserved.

Jeff Squyres
19 April 2010

This extension provides a single new function, OMPI_Affinity_str(),
that provides 3 prettyprint strings as output:

ompi_bound: describes what sockets/cores Open MPI bound this process
to (or indicates that Open MPI did not bind this process).

currently_bound: describes what sockets/cores this process is
currently bound to (or indicates that it is unbound).

exists: describes what processors are available in the current host.

See OMPI_Affinity_str(3) for more details.
