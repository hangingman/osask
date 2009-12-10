double sin(double);
double cos(double);
double	sqrt (double);
extern __inline__  double sqrt (double x)
{
  double res;
  __asm__ ("fsqrt" : "=t" (res) : "0" (x));
  return res;
}
extern __inline__  double sin (double x)
{
  double res;
  __asm__ ("fsin" : "=t" (res) : "0" (x));
  return res;
}
extern __inline__  double cos (double x)
{
  double res;
  __asm__ ("fcos" : "=t" (res) : "0" (x));
  return res;
}
