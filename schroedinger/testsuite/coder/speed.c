
#include <stdio.h>
#include <schroedinger/schro.h>
#include <schroedinger/schroarith.h>
#include <liboil/liboil.h>
#include <liboil/liboilrandom.h>
#include <math.h>

#include "arith.h"

#define N 1000


double speed_arith_dirac (int x, unsigned char *data, int n);
double speed_arith_qm (int x, unsigned char *data, int n);
double speed_arith_dirac_byte (int x, unsigned char *data, int n);
double speed_arith_bit (int x, unsigned char *data, int n);
double speed_arith_exp (int x, unsigned char *data, int n);

unsigned char data[N];

int
main (int argc, char *argv[])
{
  int x;
  double a, b, c, d, e;

  schro_init();

  for(x = 0; x <= 256; x += 1) {
    a = speed_arith_dirac (x, data, N);
    b = speed_arith_qm (x, data, N);
    c = speed_arith_dirac_byte (x, data, N);
    d = speed_arith_bit (x, data, N);
    e = speed_arith_exp (x, data, N);

    printf("%g %g %g %g %g %g\n", x/256.0, a, b, c, d, e);
  }

  return 0;
}

