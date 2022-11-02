#ifndef __CPUINFO_HPP__
#define __CPUINFO_HPP__

#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
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

   typedef int (*do_method_t)();
   long long perf_event_do(uint32_t p_type, uint64_t p_config, do_method_t func);
#endif

#define _ asm volatile

#define CHECK_ILL(val) \
   (void) setjmp(jmp_); \
   if (illegal_access_ == 1) { \
     illegal_access_ = 0; \
     return val; \
   }

// Use:
// $CXX -dM -E - < /dev/null
// to check plafrom defines

#ifdef __aarch64__
   inline void fence() {
     asm volatile ("dmb ish");
   }

   inline void *get_ucontext_pc(ucontext_t *uc) {
      return (void *)(uc->uc_mcontext.pc);
   }
#endif

#ifdef __arm__
   inline void fence() {
     asm volatile ("dmb ish");
   }

   inline void *get_ucontext_pc(ucontext_t *uc) {
      return (void *) (uc->uc_mcontext.arm_pc);
   }
#endif

#ifdef __riscv
   inline void fence() {
     asm volatile ("fence");
   }

   inline void *get_ucontext_pc(ucontext_t *uc) {
     return (void *) uc->uc_mcontext.__gregs[REG_PC];
   }
#endif

void set_signal_handler();

#endif // __CPUINFO_HPP__
