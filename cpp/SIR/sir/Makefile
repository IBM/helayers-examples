#CCFLAGS = -g --std=c++17 -Wall -D__DEBUG -fopenmp
#CCFLAGS = -O3 --std=c++17 -Wall -fopenmp -D__DEBUG -g
CCFLAGS = -O3 --std=c++17 -Wall -fopenmp -D__DEBUGW  -g
LDFLAGS = -fopenmp

NTLINCDIR = -I../HElib2/build/dependencies/Source/ntl_fetched/include/ -I/dccstor/helayers/tools/installed_ntl/include/
NTLLIB = -L../HElib2/build/dependencies/Source/ntl_fetched/src/.libs -L/dccstor/helayers/tools/installed_ntl/lib/ -lntl

HELIB2_INCDIR = -I../../../tools/installed_helib/helib_pack/include -I../HElib2/src -I/dccstor/helayers/tools/installed_helib/include
HELIB2_LIBDIR = -L../../../tools/installed_helib/helib_pack/lib -L../HElib2/build/helib_pack/lib -L/dccstor/helayers/tools/installed_helib/lib64 -lhelib

SEAL_INCDIR = -I../installed_seal_3.7/include/SEAL-3.7  -I/usr/local/include/SEAL-3.6/
SEAL_LIB = -L../installed_seal_3.7/lib  -L/usr/local/lib -lseal-3.6

LIPHEINCDIR = -I../liphe/include
LIPHELIB = -L../liphe/src -lliphe -lHelib2Liphe -lSealBfvLiphe

JSONDIR =  -I../json/src

LIBS = $(LIPHELIB) $(HELIB2_LIB) $(NTLLIB)  -lgmp  -lpthread
INCS = $(JSONDIR) $(NTLINCDIR) $(HELIB2_INCDIR) $(LIPHEINCDIR) $(SEAL_INCDIR)

# all: test_zp
all: test_zp sir_fhe

benchmark: benchmark.o
	g++ $(LDFLAGS) -o $@ $^ $(LIBS)

test_adj: test_adj.o times.o
	g++ $(LDFLAGS) -o $@ $^ $(LIBS)

test_zp: test_zp.o times.o
	g++ $(LDFLAGS) -o $@ $^ $(LIBS)

test_seal: test_seal.o times.o
	g++ $(LDFLAGS) -o $@ $^ $(LIBS)

sir_fhe: sir_fhe.o times.o
	g++ $(LDFLAGS) -o $@ $^ $(LIBS) $(SEAL_LIB) $(HELIB2_LIBDIR)

test: BGV_bootstrap.cc
	g++ -O3  --std=c++17 -Wall -D__DEBUG -fopenmp -g  -fopenmp -o test BGV_bootstrap.cc  -L../../../tools/installed_helib/helib_pack/lib  -lhelib -lntl   -lgmp  -lpthread
	#g++ -g  --std=c++17 -Wall -D__DEBUG -fopenmp -g  -fopenmp -o test BGV_bootstrap.cc  -L../../../tools/installed_helib/helib_pack/lib -L../HElib_v2.2.0/helib_pack/lib -lhelib -lntl   -lgmp  -lpthread

%.o: %.cc
	g++ $(CCFLAGS) -c  $(INCS) $<

clean:
	rm -f *.o test_zp test_helib sir_fhe

