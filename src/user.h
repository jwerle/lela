
#ifndef LELA_USER_H
#define LELA_USER_H 1

/**
 * The lela user type structure.
 */

typedef struct lela_user lela_user;
struct lela_user {
  const char *name;
  const char *gender;
  unsigned int age;
};

#endif
