#define FINDEX_IMPLEMENTATION
#include "findex.h"

int main(void) {
  printf("Hello World\n");

  Findex root = {0};

  findex_scan(&root, "./Root/");

  findex_print(&root, 0);

  Findex *file = &root.children.items[0].children.items[0];
  size_t output_len = 0;
  char *output = findex_get_file_location(file, &output_len);

  printf("\"%s\"\nOutput_len: %zu\n", output, output_len);

  findex_free(&root);

  findex_print(&root, 0);

  return 0;
}