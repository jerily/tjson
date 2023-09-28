ifndef NAVISERVER
    NAVISERVER  = /usr/local/ns
endif

#
# Module name
#
MOD      =  tjson.so

#
# Objects to build.
#
MODOBJS     = src/library.o src/cJSON/cJSON.o src/jsonpath/jsonpath.o src/custom_triple_notation/custom_triple_notation.o

#MODLIBS  +=

CFLAGS += -DUSE_NAVISERVER

include  $(NAVISERVER)/include/Makefile.module