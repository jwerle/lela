
#include "lela.h"

/**
 * Our lela instance.
 */

static lela *me = NULL;

/**
 * Main function.
 */

int
main (int argc, const char **argv) {
  lela_boot(&me, argc, argv);
  return 0;
}
