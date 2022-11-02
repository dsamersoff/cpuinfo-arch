PROJ:=cpuinfo
VERSION:=$(shell git log -1 --date=format:"%Y-%m-%d" --format="%ad")
SUPPORTED_TARGETS:=aarch64 arm riscv64

# for cross compilation, define CXX
TARGET:=$(shell $(CXX) -dumpmachine | sed -n -e "s/-.*//p")

ifeq ($(filter $(TARGET),$(SUPPORTED_TARGETS)),)
  $(error Unsupported TARGET: $(TARGET))
endif

$(info Building for TARGET: $(TARGET))

CXXFLAGS+=--std=c++11 -gdwarf-2 -fverbose-asm -fpermissive
DEFINES=-DVERSION="\"$(VERSION)\"" -D$(TARGET)

ifneq ($(SYSROOT),)
  CXXFLAGS+= "--sysroot=$(SYSROOT)"
endif

SRCS=cpuinfo.cpp cpuinfo-$(TARGET).cpp

ifeq ($(TARGET),aarch64)
  CXXFLAGS+=-march=armv8.1-a
  DEFINES+=-DCHECK_PERF_EVENTS
endif

ifeq ($(TARGET),riscv64)
  CXXFLAGS+=-march=rv64gv
endif

OBJS:=$(patsubst %.cpp,%.o,$(SRCS))

all: $(PROJ)

lst: cpuinfo.lst

clean:
	rm -f cpuinfo.lst cpuinfo $(OBJS)

cpuinfo: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

cpuinfo.o: cpuinfo.cpp
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -o $@ $<

cpuinfo-$(TARGET).o: cpuinfo-$(TARGET).cpp
	$(CXX) -c $(CXXFLAGS) $(DEFINES) -o $@ $<

cpuinfo.lst: cpuinfo-${TARGET}.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) -c -Wa,-adhln $< > $@
