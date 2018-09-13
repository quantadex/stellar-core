OPTI=-O2
DBG=
CXX=g++
CXXFLAGS=-std=c++11 $(OPTI) $(DBG)

###ttest:  Int128.o main.o
###	$(CXX) $(CXXFLAGS) -o ttest $^ 

Int128.o:	Int128.h Int128.cpp

clean:
	rm -f *.o ttest
