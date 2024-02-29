CCFLAGS = -O3 --std=c++17 -Wall -fopenmp

##NTLINCDIR = -I../../ntl-11.4.3/include
#NTLINCDIR = -I../../HElib2/build/dependencies/Source/ntl_fetched/include/
##NTLLIBDIR = -L../../ntl-11.4.3/src
#
#HELIBINCDIR = -I../../HElib-multithread/src
#HELIBLIBDIR = -L../../HElib-multithread/src

#NTLINCDIR = -I../../HElib2/build/dependencies/Source/ntl_fetched/include/
#HELIB2_INCDIR = -I../../helib_install/helib_pack/include/
#HELIB2_LIBDIR = -L../../helib_install/helib_pack/lib/

NTLINCDIR = -I/dccstor/helayers/tools/installed_ntl/include/
HELIB2_INCDIR = -I../../../../tools/installed_helib/helib_pack/include  -I../../HElib_v2.2.0/helib_pack/include -I/dccstor/helayers/tools/installed_helib/include
HELIB2_LIBDIR = -L../../../../tools/installed_helib/helib_pack/lib      -L../../HElib_v2.2.0/helib_pack/lib  -L/dccstor/helayers/tools/installed_helib/lib

#PALISADE_INCDIR = -I/usr/local/include/palisade -I/usr/local/include/palisade/pke -I/usr/local/include/palisade/core
#PALISADE_LIB = -L/usr/local/lib

SEAL_INCDIR = -I../installed_seal_3.7/include/SEAL-3.7  -I/usr/local/include/SEAL-3.6/
SEAL_LIB = -L../installed_seal_3.7/lib  -lseal-4.0

#HEAANINCDIR = -I../../HEAAN_v2.1/HEAAN/src
#HEAANLIBDIR = -L../../HEAAN_v2.1/HEAAN/src

LIPHEINCDIR = -I../include
INCS = $(NTLINCDIR) $(HELIB2_INCDIR) $(LIPHEINCDIR) $(PALISADE_INCDIR) $(SEAL_INCDIR)

OBJ = zp.o primes.o thread_pool.o # heaan_number.o

#all: libliphe.a   libPalisadeCkksLiphe.a
all: libliphe.a libHelib2Liphe.a libSealBfvLiphe.a

libliphe.a: $(OBJ)
	ar rUv $@ $^

libSealBfvLiphe.a: seal_bfv_keys.o
	ar rUv $@ $^

libPalisadeBfvLiphe.a: palisade_bfv_keys.o palisade_bfv_number.o
	ar rUv $@ $^

libPalisadeCkksLiphe.a: palisade_ckks_keys.o palisade_ckks_number.o palisade_ckks_exact_number.o palisade_ckks_exact_keys.o
	ar rUv $@ $^

libHelib2Liphe.a: helib2_keys.o
	ar rUv $@ $^

libHelibLiphe.a: helib_keys.o helib_number.o
	ar rUv $@ $^

libHeaanLiphe.a: heaan_number.o
	ar rUv $@ $^

helib_keys.o: helib_keys.cc
	g++ $(CCFLAGS) -c -I../../ntl-10.5.0/include $(HELIBINCDIR) $(LIPHEINCDIR) $<

helib_number.o: helib_number.cc
	g++ $(CCFLAGS) -c -I../../ntl-10.5.0/include $(HELIBINCDIR) $(LIPHEINCDIR) $<

%.o: %.cc
	g++ $(CCFLAGS) -c  $(INCS) $<

clean:
	rm -f *.o *.a
