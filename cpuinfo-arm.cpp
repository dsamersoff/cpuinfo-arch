#include "cpuinfo.hpp"

extern jmp_buf jmp_;
extern int illegal_access_;

// Check for presence of neon coprocessor, get sigill if no neon
double neon() {
#ifdef __SOFTFP__
  return 0;
#else
   CHECK_ILL(0);
   uint32_t n[4];
   _ ("mov r0, %[n]"::[n] "r" (n));
   _ ("vld1.32 d0, [r0]");
   return 1;
#endif
}

int add(int i, int j) {
    int res = 0;
    _ ("add %[res], %[i], %[j]" : [res] "=r" (res) : [i] "r" (i), [j] "r" (j));
    return res;
}

int main(int argc, char* argv[]) {

   set_signal_handler();

   printf("cpuinfo utility %s.\n", VERSION);
   printf("Smoke test of gcc assembly (expect 5): %d\n", add(2,3));

   int res;
   printf("  NEON: ");
   res = neon();
   if (res > 0) {
      printf("  yes (no illegal instruction message)\n");
   }
   else {
      printf("  no (software fp or illegal instruction)\n");
   }

   return 0;
}
