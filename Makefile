CXXFLAGS=-gdwarf-2 -fverbose-asm -fpermissive -I./include 

SRC=cpuinfo.cpp

all: cpuinfo.lst cputest 

clean:
	rm -f cputest.lst cputest cputest.o

cpuinfo: $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< 

cpuinfo.lst: $(SRC)
	$(CXX) $(CXXFLAGS) -c -Wa,-adhln $< > $@
