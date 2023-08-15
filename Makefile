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
MODOBJS     = library.o

MODLIBS  += -lcjson

CFLAGS += -DUSE_NAVISERVER -I/usr/local/include/cjson

include  $(NAVISERVER)/include/Makefile.module