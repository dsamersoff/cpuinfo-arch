CXXFLAGS=-march=armv8.1-a -gdwarf-2 -fverbose-asm -fpermissive -I./include 
DEFINES=-DCHECK_PERF_EVENTS

SRC=cpuinfo.cpp

all: cpuinfo.lst cpuinfo 

clean:
	rm -f cpuinfo.lst cpuinfo cpuinfo.o

cpuinfo: $(SRC)
	$(CXX) $(CXXFLAGS) $(DEFINES) -o $@ $< 

cpuinfo.lst: $(SRC)
	$(CXX) $(CXXFLAGS) $(DEFINES) -c -Wa,-adhln $< > $@
