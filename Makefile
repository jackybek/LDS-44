HOME=/root/open62541/
CC=gcc
CFLAGS1=-g -std=c99 -v -Wall
#CC=g++
#CFLAGS1=-x c++ -std=c++98 -v -fpermissive -Wno-literal-suffix -Wno-write-strings -Wno-long-long -Wno-return-type
CFLAGS2=-I$(HOME) -I$(HOME)include/ -I$(HOME)plugins/ -I$(HOME)src/ -I$(HOME)build/src_generated/ \
-I$(HOME)arch/ -I$(HOME)deps/ -I$(HOME)plugins/include/ -I/usr/local/include/
CFLAGS3=-g -pass-exit-codes
DEPS=
LIBS=-lm -lrt -lpthread  -lcrypto -lssl -lmbedcrypto -lmbedtls -lmbedx509 -lwebsockets
OBJ=LDS_Misc.o LDS_encryptServer.o LDS_configureServer.o LDS_main.o

ODIR=obj
LDIR1=-L/usr/local/lib/
LDIR2=-L/usr/lib/
LDFLAGS=-L$(HOME)build/bin/ -l:libopen62541.a

all: myNewLDSServer

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS1) $(CFLAGS2) $(CFLAGS3) -c  $< -o $@

myNewLDSServer: $(OBJ)
	$(CC) $(LDIR1) $(LDIR2) -o $@ $^ $(LDFLAGS) $(LIBS)

