#include "minunit.h"
#include "../src/btree/btree.h"

MU_TEST(testBtree) {
  btnode *root;

  root = btree_new(1, btree_make_record(1337));
  root = btree_insert(root, 2, 1338);

  btree_print(root);
}

MU_TEST_SUITE(test_btree) {
  // MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(testBtree);
}

int main(int argc, char **argv) {
  // return testIndex();
  MU_RUN_SUITE(test_btree);
  // MU_REPORT();
  return minunit_status;
}
