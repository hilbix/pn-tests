#include "ezpn.h"

int main(int argc, char **argv)
{
  Z	z;

  z	= z_init(NULL);
  z_args(z, argc, argv);
  z_open(z);
  z_sub_open(z, zarg(z, 1, "hello_world"));

  for (;;)
    {
      z_sub_msg(z);
      if (!zok(z))
        break;

      if (!z->msg)
        putchar('.');
      else
        printf("MSG: [%s]\n", z->msg);
      fflush(stdout);
    }

  z_free(z);

  return zret(z);
}

