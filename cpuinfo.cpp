#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <setjmp.h>


// Check for availability of important perf events
#ifdef CHECK_PERF_EVENTS
   #include <linux/perf_event.h>    /* Definition of PERF_* constants */
   #include <linux/hw_breakpoint.h> /* Definition of HW_* constants */
   #include <sys/syscall.h>         /* Definition of SYS_* constants */

   #include <sys/ioctl.h>
   #include <unistd.h>
   #include <string.h>
#endif

// Borrowed from arm-trusted-firmware project
//
#define SCTLR_M_BIT     (1 << 0)
#define SCTLR_A_BIT     (1 << 1)
#define SCTLR_C_BIT     (1 << 2)
#define SCTLR_SA_BIT        (1 << 3)
#define SCTLR_CP15BEN_BIT   (1 << 5)
#define SCTLR_I_BIT     (1 << 12)
#define SCTLR_NTWI_BIT      (1 << 16)
#define SCTLR_NTWE_BIT      (1 << 18)
#define SCTLR_WXN_BIT       (1 << 19)
#define SCTLR_EE_BIT        (1 << 25)

// Borrowed from linux kernel ./arch/arm64/include/asm/cputype.h

#define MIDR_CPU_MODEL(imp, partnum) \
    (((imp)         << 24) | \
    (0xf            << 16) | \
    ((partnum)      << 4))

#define ARM_CPU_IMP_ARM         0x41
#define ARM_CPU_IMP_APM         0x50
#define ARM_CPU_IMP_CAVIUM      0x43
#define ARM_CPU_IMP_BRCM        0x42
#define ARM_CPU_IMP_QCOM        0x51

#define ARM_CPU_PART_AEM_V8     0xD0F
#define ARM_CPU_PART_FOUNDATION     0xD00
#define ARM_CPU_PART_CORTEX_A57     0xD07
#define ARM_CPU_PART_CORTEX_A53     0xD03
#define ARM_CPU_PART_CORTEX_A73     0xD09

#define APM_CPU_PART_POTENZA        0x000

#define CAVIUM_CPU_PART_THUNDERX    0x0A1
#define CAVIUM_CPU_PART_THUNDERX_81XX   0x0A2
#define CAVIUM_CPU_PART_THUNDERX_83XX   0x0A3

#define BRCM_CPU_PART_VULCAN        0x516

#define QCOM_CPU_PART_FALKOR_V1     0x800

#define MIDR_CORTEX_A53 MIDR_CPU_MODEL(ARM_CPU_IMP_ARM, ARM_CPU_PART_CORTEX_A53)
#define MIDR_CORTEX_A57 MIDR_CPU_MODEL(ARM_CPU_IMP_ARM, ARM_CPU_PART_CORTEX_A57)
#define MIDR_CORTEX_A73 MIDR_CPU_MODEL(ARM_CPU_IMP_ARM, ARM_CPU_PART_CORTEX_A73)
#define MIDR_THUNDERX   MIDR_CPU_MODEL(ARM_CPU_IMP_CAVIUM, CAVIUM_CPU_PART_THUNDERX)
#define MIDR_THUNDERX_81XX MIDR_CPU_MODEL(ARM_CPU_IMP_CAVIUM, CAVIUM_CPU_PART_THUNDERX_81XX)
#define MIDR_THUNDERX_83XX MIDR_CPU_MODEL(ARM_CPU_IMP_CAVIUM, CAVIUM_CPU_PART_THUNDERX_83XX)
#define MIDR_QCOM_FALKOR_V1 MIDR_CPU_MODEL(ARM_CPU_IMP_QCOM, QCOM_CPU_PART_FALKOR_V1)

#define L1_CACHE_BYTES (1 << 7) 

jmp_buf jmp_;
int illegal_access_ = 0;

#define _ asm volatile

#define CHECK_ILL(val) \
   (void) setjmp(jmp_); \
   if (illegal_access_ == 1) { \
     illegal_access_ = 0; \
     _ ("dmb ish"); \
     return val; \
   }

#ifdef CHECK_PERF_EVENTS
int strex_do() {
   CHECK_ILL(0);
   int res = 1;
   int *res_ptr = &res;
   register long x1 __asm__("x1") = 1;

   _ ("ldaxr x1, %[res]; add x1, x1, 1; stlxr w8, x1, %[res]" : [res] "+Q" (res_ptr) : "r" (x1) );

   return res;
}

long long perf_event_do(uint32_t p_type, uint64_t p_config) {
   unsigned long flags = PERF_FLAG_FD_CLOEXEC; 
   int fd = -1;
   struct perf_event_attr attr;
   long long count;

   memset(&attr, 0, sizeof(attr));
   attr.type = p_type;
   attr.size = sizeof(attr);
   attr.config = p_config;
   attr.disabled = 1;
   attr.exclude_kernel = 1;
   attr.exclude_hv = 1;

   fd = syscall(__NR_perf_event_open, &attr, 0 /*pid*/, -1 /*cpu*/, -1 /*group_fd*/, flags);
   if (fd < 0) {
      return (long long) fd;
   }

   ioctl(fd, PERF_EVENT_IOC_RESET, 0);
   ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
 
   for (int i = 0; i < 10; ++i) {
      strex_do();
   }

   ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
   read(fd, &count, sizeof(count));

   close(fd);   
   return count;
}
#endif

int add(int i, int j) {
    int res = 0;
    _ ("ADD %[res], %[i], %[j]" : [res] "=r" (res) : [i] "r" (i), [j] "r" (j));
    return res;
}

uint32_t cache_levels() {
   CHECK_ILL(0);

   uint32_t n = 0;

  // Neither SCTLR no SCTLR_EL0 is available 
  // on assembly level despite of ARMv8-A spec
  // So It's not possible to enable access 
  // to CLIDR_EL1 from EL0
  //
  // TODO: Try under different OS
   
#ifdef CAN_ACCESS_SCTLR
   uint32_t bit = SCTLR_C_BIT; // U bit to enable cache information access

   _ ("mov x1, %[bit];"::[bit] "r" (bit));
   _ ("mrs x0, sctlr_el1;");
   _ ("orr x0, x0, x1;");
   _ ("msr sctlr_el1, x0;");
   _ ("mrs x0, clidr_el1");
   _ ("mov %[res], x0":[res] "=r" (n));

   printf("Available cache levels: %x\n", n);
   // Get configuration for each level

   if (n >= 1) {
     _ ("mov x0, 1;");
     _ ("msr csselr_el1, x0;");
     _ ("mrs x0, ctr_el0");
     _ ("mov %[res], x0":[res] "=r" (n));
   }

   if (n >= 2) {
     _ ("mov x0, 2;");
     _ ("msr csselr_el1, x0;");
     _ ("mrs x0, ctr_el0");
     _ ("mov %[res], x0":[res] "=r" (n));
   }
#endif

   return n;
}

uint32_t cache_wg() {
   CHECK_ILL(0);
   uint32_t n = 0;

   _ ("mrs x0, ctr_el0;");
   _ ("mov %[res], x0":[res] "=r" (n));

   return (n >> 24) & 0xF;
}

uint32_t cache_line_size() {
   CHECK_ILL(0);
   uint32_t n = 0;

   _ ("mrs x0, ctr_el0;");
   _ ("mov %[res], x0":[res] "=r" (n));

//   return n;
 
   uint32_t cwg = (n >> 24) & 0xF;
   return (cwg) ? (4 << cwg) : L1_CACHE_BYTES; 
}

uint32_t cache_block_size() {
   CHECK_ILL(0);
   uint32_t n = 0;

   _ ("mrs x0, dczid_el0;");
   _ ("mov %[res], x0":[res] "=r" (n));

   return n; 
}

double cpu_freq() {
   CHECK_ILL(0);
   uint32_t n = 0;

   _ ("mrs x0, cntfrq_el0;");
   _ ("mov %[res], x0":[res] "=r" (n));


   return (double)(n*2)/1000/1000; 
}

// Check for presence of neon coprocessor,
// get sigill if no neon
double neon() {
   CHECK_ILL(0);
   uint32_t n[4];

   _ ("mov x0, %[n]"::[n] "r" (n));
   _ ("ldr q0, [x0];");
   return 1; 
}

uint32_t modern_atomics() {
   CHECK_ILL(0);
   int res = 1;
   int *res_ptr = &res;
   register long x1 __asm__("x1") = 1;

   _ ("stadd x1, %[res]" : [res] "+Q" (res_ptr) : "r" (x1));

   return res;
}

const char *cpu_rev() {
   CHECK_ILL(0);
   uint32_t n = 0;
   static char cpuid[64];

   _ ("mrs x0, midr_el1;");
   _ ("mov %[res], x0":[res] "=r" (n));

   // Decode:

   // Strip revision revision
   int rev = n & 0xF;
   const char *s = NULL;

   switch (n & (-1 << 4)) {
      case MIDR_CORTEX_A53:
            s = "CORTEX_A53"; break; 
      case MIDR_CORTEX_A57:
            s = "CORTEX_A57"; break; 
      case MIDR_CORTEX_A73:
            s = "CORTEX_A73"; break; 
      case MIDR_THUNDERX:  
            s = "THUNDERX"; break; 
      case MIDR_THUNDERX_81XX:  
            s = "THUNDERX_81XX"; break; 
      case MIDR_THUNDERX_83XX:  
            s = "THUNDERX_83XX"; break; 
      case MIDR_QCOM_FALKOR_V1: 
            s = "QCOM_FALKOR_V1"; break; 
      default:
           s = NULL;
   } 

   if (s == NULL) {
     sprintf(cpuid,"id. %Xi rev. %X", n, rev);
   }
   else {
     sprintf(cpuid,"%s rev. %X", s, rev);
   }
   return cpuid; 
}

void handler(int signo) {
   illegal_access_ = 1;
   _ ("dmb ish");

   ucontext_t uc;
   getcontext(&uc);

   void *pc = (char *)(uc.uc_mcontext.pc);
   printf("Illegal instruction at %p %x\n", pc, *(uint32_t *)pc);

   longjmp(jmp_,0);

   //   exit(-1);
}


int main(int argc, char* argv[]) {

   struct sigaction sa;
   sa.sa_handler = (sighandler_t) handler;
   sa.sa_flags = SA_SIGINFO;
   sigaction(SIGILL, &sa, NULL);

   printf("Start testing\n");
   printf("Smoke test of gcc assembly: %d\n", add(2,3));
   printf("Cpu info:\n");
   printf("  CPU: %s\n", cpu_rev());
   printf("  Cache write granularity: %x\n", cache_wg());
   printf("  Cache line size: %x\n", cache_line_size());
   printf("  Cache block size: %x\n", cache_block_size());
   printf("  CPU freq: %.2fGhz\n", cpu_freq());

   printf("  NEON: ");
   if (neon() > 0) {
      printf("  yes (no illegal instruction message)\n");
   }

   printf("  STADD: ");
   if ( modern_atomics() > 0) {
      printf("  yes (no illegal instruction message)\n");
   }

#ifdef CHECK_PERF_EVENTS
   long long count;

   printf("Measuring performance counters:\n");
   count = perf_event_do(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
   printf("PERF: hw_instr_cnt is %lld %s\n", count, (count > 0) ? "Ok" : "not available");

   count = perf_event_do(PERF_TYPE_RAW, 0x6d /*strex_pass_spec */);
   printf("PERF: strex_pass_spec is %lld %s\n", count, (count > 0) ? "Ok" : "not available");
#endif

   return 0;
}
