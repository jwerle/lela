
/**
 * System dependencies.
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * clib dependencies.
 */

#include <linenoise/linenoise.h>
#include <asprintf/asprintf.h>

/**
 * Local dependencies.
 */

#include "lela.h"
#include "user.h"
#include "sql/ddl.h"
#include "sql/boot.h"

/**
 * Linenoise completion callback.
 */

static void
on_lela_linenoise_completion_ (const char *, linenoiseCompletions *);

/**
 * Called when lela is ready to start
 * the main loop.
 */

static int
on_lela_query_ (void *, int, char **, char **);

/**
 * Boot and restore lela state.
 */

void
lela_boot (lela **me, int argc, const char **argv) {
  char *input = NULL;
  lela_user *user = NULL;
  const char *response = NULL;

  srand(time(NULL));

  // init
  lela_init(me, argc, argv);

  lela_debug("Initializing repl..."); {
    // set the completion callback that is called
    // every time the user uses the <tab> key.
    linenoiseSetCompletionCallback(on_lela_linenoise_completion_);

    // load repl history
    linenoiseHistoryLoad(LELA_REPL_HISTORY_FILENAME);
  }

  // kick off state
  (*me)->state = LELA_STATE_INIT;

  // kick off state tick
  lela_state_tick(*me);

#define BOOT_CHECK_INPUT(i) if (NULL == i) { lela_exit(*me, 0); }
  while (1) {
    usleep(5000);

    if ((*me)->state == LELA_STATE_READY) {
      user = &(*me)->user;
      if (NULL != user->name) {
        lela_converse(*me, "Hey %s :]", user->name);
      } else {
        // get name
        lela_converse(*me, "Hey, what's your name?");
        input = lela_get_input(*me);
        BOOT_CHECK_INPUT(input);
        user->name = input;

        if (strlen(user->name) <= 3) {
          if (user->name[0] <= 0x6d) {
            response = "Short and sweet";
          } else {
            response = "Short names are easy!";
          }
        } else if (strlen(user->name) > 6) {
          if (user->name[0] <= 0x6d) {
            response = "I haven't met anyone with that name before";
          } else {
            response = "Well... hello";
          }
        }

        lela_converse(*me, "%s - nice to meet you, %s.", response, user->name);

        // gender
        lela_converse(*me,
            "You have a lovely name. Would you mind telling me your gender?");
        input = lela_get_input(*me);
        BOOT_CHECK_INPUT(input);
        LELA_STRING_TO_LOWER(input);
        user->gender = input;

        if (0 == strcmp("male", user->gender)) {
          lela_converse(*me, "You're a boy!");
        } else if (0 == strcmp("female", user->gender)) {
          lela_converse(*me, "You're a girl!");
        } else {
          lela_converse(*me,
                        "I've never met anyone that identifies as a %s before",
                        user->gender);
        }

        // age
        if (0 == rand() % 3) {
          response = "Anyways! Do you mind telling you age?";
        } else if (0 == rand() % 4) {
          response = "So... How old are you?";
        } else if (0 == rand() % 5) {
          response = "Hmmm... Okay, so how old are you?";
        } else {
          response = "So, if you don't mind me asking. How old are you?";
        }

        lela_converse(*me, "%s", response);
        input = lela_get_input(*me);
        BOOT_CHECK_INPUT(input);
        user->age = atoi(input);

        if (user->age < 15) {
          response = "You'll be driving soon! ;]";
        } else if (user->age < 18) {
          response = "Some of the best life experiences happen around your age";
        } else if (user->age < 21) {
          response = "You're almost at a legal drinking age in the US.";
        } else if (user->age < 28) {
          response = "What an age to be...";
          lela_converse(*me, "How has your 20s been so far?");
          input = lela_get_input(*me);
          BOOT_CHECK_INPUT(input);
          LELA_STRING_TO_LOWER(input);

          if (strstr(input, "good")) {
            lela_converse(*me, "I bet it has been good!");
          } else if (strstr(input, "awesome")) {
            lela_converse(*me, "Awesome you say!? Haha, that's great.");
          } else {
            lela_converse(*me, "Well, okay. I'd love to hear more someday.");
          }
        } else if (user->age < 40) {
          response = "Isn't life precious?";
        }

        lela_converse(*me, "Anyways...");
        lela_converse(*me, "%d! %s", user->age, response);
      }

      (*me)->state = LELA_STATE_IDLE;
      continue;
    }

    if ((*me)->state == LELA_STATE_IDLE) {
      lela_parse_input(*me);
    }
  }
#undef BOOT_CHECK_INPUT
}

void
lela_state_tick (lela *me) {
  LELA_CHECK_ME(me);
  switch (me->state) {
    case LELA_STATE_READY:
      // noop
      return;

    case LELA_STATE_INIT:
      me->state = LELA_STATE_DDL;
      lela_state_tick(me);
      break;

    case LELA_STATE_DDL:
      // insert ddl
      lela_debug("Loading DDL...");
      lela_db_query(me, lela_sql_ddl(), on_lela_query_);
      me->state = LELA_STATE_BOOT;
      lela_state_tick(me);
      break;

    case LELA_STATE_BOOT:
      // load user info
      lela_debug("Loading data...");
      lela_db_query(me, lela_sql_boot(), on_lela_query_);
      me->state = LELA_STATE_READY;
      break;
  }
}

void
lela_init (lela **me, int argc, const char **argv) {
  lela_debug("Initializing...");

  lela_debug("Allocating memory for lela..."); {
    // alloc
    if (NULL == *me) {
      *me = (lela *) malloc(sizeof(lela));

      // set ->db to NULL
      (*me)->db = NULL;

      // reset user state
      (*me)->user.name = NULL;
      (*me)->user.gender = NULL;
      (*me)->user.age = 0;
    }

    // fail on alloc error
    if (NULL == *me) {
      lela_perror("boot: Failed to allocate memory for lela.");
      lela_exit(*me, 1);
    }

    // close db in case it is already open
    lela_db_close(*me);

    // init configuration
    (*me)->config.db.uri = NULL;
    (*me)->config.db.flags = (SQLITE_OPEN_URI |
                              SQLITE_OPEN_CREATE |
                              SQLITE_OPEN_READWRITE);
  }

  // parse options from command line
  lela_debug("Parsing command line options..."); {
    int i = 0;
    const char *arg;
    const char *next;
    while (i < argc) {
      arg = argv[i++];
      next = argv[i];
      if (0 == strcmp(arg, "-d") || 0 == strcmp(arg, "--db")) {
        if (NULL == next) {
          lela_perror("Missing value for `-d/--db' option.");
          lela_exit(*me, 1);
        }

        (*me)->config.db.uri = next;
        lela_debugf("Database URI set to %s\n", next);
      }
    }
  }

  // init db
  lela_debug("Initializing database..."); {
    if (NULL == (*me)->config.db.uri) {
      (*me)->config.db.uri = LELA_DEFAULT_DATABASE_URI;
    }

    lela_db_open(*me);
  }
}

/**
 * Exit gracefully with code.
 */

void
lela_exit (lela *me, int code) {
  lela_db_close(me);
  lela_free(me);
  exit(code);
}

/**
 * Opens lela db if applicable.
 */

void
lela_db_open (lela *me) {
  LELA_CHECK_ME(me);
  const char *uri = NULL;
  int flags = 0;
  int rc = 0;
  if (NULL == me->db) {
    uri = me->config.db.uri;
    flags = me->config.db.flags;

    if (NULL == uri) {
      lela_perror("Database uri not set.");
      lela_exit(me, 1);
    }

    lela_debugf("Opening database `%s'\n", uri);
    rc = sqlite3_open_v2(uri, &me->db, flags, NULL);
    if (rc) {
      lela_perrorf("Failed to open lela database: %s\n",
                   sqlite3_errmsg(me->db));
      lela_exit(me, 1);
    }
  }
}

/**
 * Closes lela db if applicable.
 */

void
lela_db_close (lela *me) {
  LELA_CHECK_ME(me);
  if (me->db) {
    sqlite3_close(me->db);
    me->db = NULL;
  }
}

/**
 * Free lela.
 */

void
lela_free (lela *me) {
  if (NULL != me) {
    lela_db_close(me);
    free(me);
  }

  me = NULL;
}

/**
 * Parses lela input.
 */

void
lela_parse_input (lela *me) {
  LELA_CHECK_ME(me);
  char *input = lela_get_input(me);
  if (NULL == input) {
    lela_exit(me, 0);
  }
}

/**
 * Gets input from stdin.
 */

char *
lela_get_input (lela *me) {
  LELA_CHECK_ME(me);
  return linenoise(LELA_PREAMBLE "[+] ");
}

/**
 * Query database with arbitrary SQL.
 */

void
lela_db_query (lela *me, const char *query, lela_db_query_callback *cb) {
  char *err = NULL;
  int rc = 0;
  LELA_CHECK_ME(me);
  lela_db_open(me);
  rc = sqlite3_exec(me->db, query, on_lela_query_, me, &err);
  if (SQLITE_OK != rc) {
    lela_perrorf("SQL error `%s'\n", err);
    lela_exit(me, 1);
  }
}

static void
on_lela_linenoise_completion_ (const char *buf, linenoiseCompletions *lc) {
  // @TODO(werle) - complete me
}

static int
on_lela_query_ (void *data, int argc, char **argv, char **columns) {
  lela *me = (lela *) data;
  int i = 0;
  LELA_CHECK_ME(me);

  printf("foo\n");
  switch (me->state) {
    case LELA_STATE_DDL:
      me->state = LELA_STATE_BOOT;
      printf("dll\n");
      break;

    case LELA_STATE_BOOT:
      for (; i < argc; ++i) {
        printf("%s = %s\n", columns[i], argv[i] ? argv[i] : "null");
      }
      printf("boot\n");
      me->state = LELA_STATE_READY;
      break;

    case LELA_STATE_READY:
      break;
  }

  lela_state_tick(me);
  return 0;
}
