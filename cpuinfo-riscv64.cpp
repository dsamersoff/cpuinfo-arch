#include "cpuinfo.hpp"

extern jmp_buf jmp_;
extern int illegal_access_;

// Check for presence of vector ip, get sigill if not present
int vector_ip() {
   CHECK_ILL(0);

   const register char src_text[] = "Vectorized memcpy test on not so long string";
   const int src_len = sizeof(src_text);
   char dest_buf[src_len];

   register char *dst __asm__ ("a0") = dest_buf; 
   const register char *src __asm__ ("a1") = src_text; 
   register int len __asm__ ("a2") = src_len; 
   register char *dst_copy __asm__ ("a3") = dest_buf; 

   _ (
     "loop:"
     "vsetvli t0, a2, e8, m8, ta, ma;"
     "vle8.v v0, (a1);"
     "add a1, a1, t0;"
     "sub a2, a2, t0;"
     "vse8.v v0, (a3);"
     "add a3, a3, t0;"
     "bnez a2, loop;" 
    : // no output
    : // no input
    : "t0" 
    );

   return (memcmp(dest_buf, src_text, src_len) == 0) ? 1 : 2;
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
