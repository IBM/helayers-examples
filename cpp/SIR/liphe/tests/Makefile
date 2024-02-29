CCFLAGS = -g --std=c++11 -Wall -fopenmp

#NTLINCDIR = -I../../ntl-11.4.3/include
#NTLLIB = ../../ntl-11.4.3/src/ntl.a

NTLINCDIR = -I../../HElib2/build/dependencies/Source/ntl_fetched/include/
NTLLIB = -L../../HElib2/build/dependencies/Source/ntl_fetched/src/.libs -lntl

HELIB2_INCDIR = -I../../HElib2/src
HELIB2_LIB = -L../../HElib2/build/helib_pack/lib -lhelib

PALISADE_INCDIR = -I../../palisade/include/palisade/pke -I../../palisade/include/palisade/core/ -I../../palisade/include/palisade/
PALISADE_LIB = -L../../palisade/lib -lPALISADEpke -lPALISADEcore

LIPHEINCDIR = -I../include
LIPHELIB = -L../src -lliphe

GMPLIB =  -lgmp

LIBS = $(LIPHELIB)  $(NTLLIB) $(GMPLIB) -lpthread
INCS = $(NTLINCDIR) $(HELIB2_INCDIR) $(LIPHEINCDIR) $(PALISADE_INCDIR)

all: test_zp test_helib2 test_palisade_bfv

#all: test min fast_min min2 min3

benchmark_helib2: benchmark_helib2.o
	g++ $(LDFLAGS) -o $@ $^ -lHelib2Liphe $(HELIB2_LIB) $(LIBS)

test_zp: test_zp.o
	g++ $(LDFLAGS) -o $@ $^ $(LIBS)

test_helib: test_helib.o
	g++  -o test_helib test_helib.o -L../src -lliphe  -lHelibLiphe ../../HElib-multithread/src/fhe.a   -L ../../ntl-10.5.0-multithread/src/ -lntl  -lpthread -lgmp

test_helib.o: test_helib.cc
	g++ $(CCFLAGS) -c -I../../ntl-10.5.0-multithread/include -I../../HElib-multithread/src $(LIPHEINCDIR)  $<

test_helib2: test_helib2.o
	g++ --std=c++11 $(LDFLAGS) -o $@ $^ $(LIBS) -lHelib2Liphe $(HELIB2_LIB)


test_palisade_bfv: test_palisade_bfv.o
	g++ $(LDFLAGS) -o $@ $^ $(LIPHELIB) -lPalisadeBfvLiphe $(PALISADE_LIB)


%.o: %.cc
	g++ $(CCFLAGS) -c  $(INCS) $<

clean:
	rm -f *.o test_zp test_helib

