#ifndef _CONFIG_H
#error "config_bottom.h should only be included from config.h"
#endif /* _CONFIG_H */

#include "util/util.h"

#if !(defined(HAVE_ASPRINTF) && HAVE_ASPRINTF)
# undef asprintf
# define asprintf vt_asprintf
#endif /* HAVE_ASPRINTF */

#if !(defined(HAVE_SNPRINTF) && HAVE_SNPRINTF)
# undef snprintf
# define snprintf vt_snprintf
#endif /* HAVE_SNPRINTF */

#if !(defined(HAVE_VASPRINTF) && HAVE_VASPRINTF)
# undef vasprintf
# define vasprintf vt_vasprintf
#endif /* HAVE_VASPRINTF */

#if !(defined(HAVE_VSNPRINTF) && HAVE_VSNPRINTF)
# undef vsnprintf
# define vsnprintf vt_vsnprintf
#endif /* HAVE_VSNPRINTF */

#if !(defined(HAVE_STRDUP) && HAVE_STRDUP)
# undef strdup
# define strdup vt_strdup
#endif /* HAVE_STRDUP */

#if defined(HAVE_FNMATCH_H) && HAVE_FNMATCH_H
# include <fnmatch.h>
#else /* HAVE_FNMATCH_H */
# undef fnmatch
# define fnmatch vt_fnmatch
# define FNM_NOESCAPE (1 << 1) /* Backslashes don't quote special chars. */
# define vt_fnmatch(_pattern, _string, __flags) strcmp(_string, _pattern)
#endif /* HAVE_FNMATCH_H */

#include <limits.h>
#if defined(HAVE_SYS_PARAM_H) && HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#if defined(PATH_MAX)
# define VT_PATH_MAX (PATH_MAX + 1)
#elif defined(_POSIX_PATH_MAX)
# define VT_PATH_MAX (_POSIX_PATH_MAX + 1)
#else
# define VT_PATH_MAX 256
#endif

