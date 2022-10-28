#include "cpuinfo.hpp"

jmp_buf jmp_;
int illegal_access_ = 0;

#ifdef CHECK_PERF_EVENTS
long long perf_event_do(uint32_t p_type, uint64_t p_config, do_method_t func) {
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
      func();
   }

   ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
   read(fd, &count, sizeof(count));

   close(fd);
   return count;
}
#endif

void handler(int signo) {
   illegal_access_ = 1;
   // _ ("dmb ish");
   fence();

   ucontext_t uc;
   getcontext(&uc);

   // void *pc = (char *)(uc.uc_mcontext.pc);
   void *pc = get_ucontext_pc(&uc);
   printf("Illegal instruction at %p inst: 0x%x\n", pc, *(uint32_t *)pc);

   longjmp(jmp_,0);

   //   exit(-1);
}

void set_signal_handler() {
   struct sigaction sa;
   sa.sa_handler = (sighandler_t) handler;
   sa.sa_flags = SA_SIGINFO;
   sigaction(SIGILL, &sa, NULL);
}

