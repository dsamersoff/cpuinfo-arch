CXXFLAGS=-march=armv8.1-a -gdwarf-2 -fverbose-asm -fpermissive -I./include 

SRC=cpuinfo.cpp

all: cpuinfo.lst cpuinfo 

clean:
	rm -f cpuinfo.lst cpuinfo cpuinfo.o

cpuinfo: $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< 

cpuinfo.lst: $(SRC)
	$(CXX) $(CXXFLAGS) -c -Wa,-adhln $< > $@
