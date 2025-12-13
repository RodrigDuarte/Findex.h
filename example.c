#define FINDEX_IMPLEMENTATION
#include "findex.h"

int main(void) {
  printf("Hello World\n");

  Findex root = {0};

  findex_scan(&root, "./Root/");

  findex_print(&root, 0);

  findex_free(&root);

  findex_print(&root, 0);

  return 0;
}