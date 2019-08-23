#include "ezpn.h"

int
main(int argc, char **argv)
{
  Z z;

  z	= z_init(NULL);
  z_args(z, argc, argv);
  z_open(z);
  z_pub_str(z, zarg(z, 1, "hello_world"), zarg(z, 2, "test1pub test"));
  z_pub_await(z);
  z_free(z);
  return zret(z);
}

