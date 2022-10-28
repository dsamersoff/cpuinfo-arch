#include "cpuinfo.hpp"

extern jmp_buf jmp_;
extern int illegal_access_;

// Check for presence of neon coprocessor, get sigill if no neon
double vector_ip() {
   CHECK_ILL(0);
   register unsigned v =0;
   register unsigned csr = 0x20;

   _ ("csrr %0, 0x20" : "=r" (v): "r" (csr)  );

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
   }

   return 0;
}
