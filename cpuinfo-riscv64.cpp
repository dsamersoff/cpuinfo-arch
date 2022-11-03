#include "cpuinfo.hpp"

extern jmp_buf jmp_;
extern int illegal_access_;

// Check for presence of vector ip, get sigill if not present
int vector_ip() {
   CHECK_ILL(0);

   const int csr = 0x20;
   register unsigned long v;
   _ ("csrr %0, %1" : "=r"(v) : "i" (csr));

   return 1;
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
   printf("  VECTOR IP: ");
   res = vector_ip();
   if (res > 0) {
      printf("  yes (no illegal instruction message)\n");
      if (res > 1) {
        printf(" ... but problem in vector ip implementation detected\n");
      }
   }

   return 0;
}
