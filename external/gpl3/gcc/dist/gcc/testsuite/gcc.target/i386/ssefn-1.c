/* Test argument passing with SSE and local functions
   Written by Paolo Bonzini, 25 January 2005 */

/* { dg-do compile } */
/* { dg-require-effective-target ilp32 } */
/* { dg-require-effective-target sse } */
/* { dg-final { scan-assembler "movss" } } */
/* { dg-final { scan-assembler "mulss" } } */
/* { dg-final { scan-assembler-not "movsd" } } */
/* { dg-final { scan-assembler-not "mulsd" } } */
/* { dg-skip-if "" { i?86-*-* x86_64-*-* } { "-march=*" } { "-march=i386" } } */
/* { dg-options "-O2 -march=i386 -msse -mfpmath=sse -fno-inline" } */

static float xs (void)
{
  return 3.14159265;
}

float ys (float a)
{
  return xs () * a;
}

static double xd (void)
{
  return 3.1415926535;
}

double yd (double a)
{
  return xd () * a;
}
