
#ifndef LELA_H
#define LELA_H 1

/**
 * System dependencies.
 */

#include <stdio.h>
#include <ctype.h>

/**
 * clib dependencies.
 */

#include <sqlite/sqlite3.h>

/**
 * Local dependencies.
 */

#include "user.h"

/**
 * The lela repl history filename.
 */

#ifndef LELA_REPL_HISTORY_FILENAME
#define LELA_REPL_HISTORY_FILENAME ".lela_history"
#endif

/**
 * The default lela database uri file path
 */

#ifndef LELA_DEFAULT_DATABASE_URI
#define LELA_DEFAULT_DATABASE_URI "file:lela.db?cache=shared"
#endif

/**
 * The lela preamble used in the repl
 * and for printing to stdout/stderr.
 */

#ifndef LELA_PREAMBLE
#define LELA_PREAMBLE "lela"
#endif

/**
 * Prints a formatted string to a file descriptor
 * prefixed with a preamble.
 */

#define lela_fprintf(fd, fmt, ...) ({                      \
  fprintf(fd, "%s> ", LELA_PREAMBLE);                      \
  fprintf(fd, fmt, ##__VA_ARGS__);                         \
})

/**
 * Prints a formatted string to stdout.
 */

#define lela_printf(fmt, ...) ({                           \
  lela_fprintf(stdout, fmt, ##__VA_ARGS__);                \
})

/**
 * Prints a string to stdout.
 */

#define lela_puts(string) ({                               \
  lela_printf("%s\n", string);                             \
})

/**
 * Prints a formatted string to stderr.
 */

#define lela_perrorf(fmt, ...) ({                          \
  lela_fprintf(stderr, fmt, ##__VA_ARGS__);                \
})

/**
 * Prints an error to stderr.
 */

#define lela_perror(string) ({                             \
  lela_perrorf("ERROR: %s\n", string);                     \
})

/**
 * Prints info to stdout.
 */

#define lela_info(string) ({                               \
  lela_printf("INFO: %s\n", string);                       \
})

/**
 * Prints formated info to stdout.
 */

#define lela_infof(fmt, ...) ({                            \
  lela_printf("INFO: " fmt, ##__VA_ARGS__);                \
})

/**
 * Prints debug to stdout.
 */

#ifdef DEBUG
#define lela_debug(string) ({                              \
  lela_printf("DEBUG: %s\n", string);                      \
})
#else
#define lela_debug(string)
#endif

/**
 * Prints formated debug to stdout.
 */

#ifdef DEBUG
#define lela_debugf(fmt, ...) ({                           \
  lela_printf("DEBUG: " fmt, ##__VA_ARGS__);               \
})
#else
#define lela_debugf(fmt, ...)
#endif

/**
 * Checks state of lela and bails on failure with
 * an error message and function name.
 */

#define LELA_CHECK_ME(me) ({                               \
  if (NULL == me) {                                        \
    lela_perrorf("%s: Lela is not initialized", __func__); \
    lela_exit(me, 1);                                      \
  }                                                        \
})

/**
 * Converts a string to lower case inline
 */

#define LELA_STRING_TO_LOWER(str) ({                       \
  for (int i = 0; str[i]; ++i) {                           \
    str[i] = tolower(str[i]);                              \
  }                                                        \
})

/**
 * Converse with lela.
 */

#define lela_converse(me, fmt, ...) ({                     \
  LELA_CHECK_ME(me);                                       \
  lela_printf(fmt, ##__VA_ARGS__);                         \
  printf("\n");                                            \
  usleep(80000);                                           \
})

/**
 * lela states.
 */

enum {
  LELA_STATE_NONE = 0,
  LELA_STATE_DDL,
  LELA_STATE_INIT,
  LELA_STATE_IDLE,
  LELA_STATE_BOOT,
  LELA_STATE_READY,
};

/**
 * The lela type structure.
 */

typedef struct lela lela;
struct lela {

  /**
   * Underlying lela database.
   */

  sqlite3 *db;

  /**
   * Current lela user.
   */

  lela_user user;

  /**
   * Program arguments.
   */

  const char **argv;

  /**
   * Program argument length.
   */

  int argc;

  /**
   * Current lela state.
   */

  int state;

  /**
   * Current lela configuration.
   */

  struct {

    /**
     * Database configuration.
     */

    struct {

      /**
       * Database filename uri.
       */

      const char *uri;

      /**
       * Database connection flags.
       */

      int flags;
    } db;

  } config;
};

/**
 * Callback for querying database
 */

typedef int (lela_db_query_callback) (void*, int, char **, char **);

/**
 * Boots and restores lela state.
 */

void
lela_boot (lela **, int, const char **);

/**
 * Initializes a lela structure. If memory
 * has not been allocated then it will be.
 */

void
lela_init (lela **, int, const char **);

/**
 * Exit gracefully with code.
 */

void
lela_exit (lela *, int);

/**
 * Opens lela db if applicable.
 */

void
lela_db_open (lela *);

/**
 * Closes lela db if applicable.
 */

void
lela_db_close (lela *);

/**
 * Free lela.
 */

void
lela_free (lela *);

/**
 * Parses lela input.
 */

void
lela_parse_input (lela *);

/**
 * Gets input from stdin.
 */

char *
lela_get_input (lela *);

/**
 * Query database with arbitrary SQL.
 */

void
lela_db_query (lela *, const char *, lela_db_query_callback *);

/**
 * Performs action based on state.
 */

void
lela_state_tick (lela *);

#endif
