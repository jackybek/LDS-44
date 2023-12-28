HOME=/home/pi/open62541/
CC=gcc
CFLAGS1=-g -std=c99 -v
#CC=g++
#CFLAGS1=-x c++ -std=c++98 -v -fpermissive -Wno-literal-suffix -Wno-write-strings -Wno-long-long -Wno-return-type
CFLAGS2=-I$(HOME) -I$(HOME)include/ -I$(HOME)plugins/ -I$(HOME)src/ -I$(HOME)build/src_generated/ \
-I$(HOME)arch/ -I$(HOME)deps/ -I$(HOME)plugins/include/ -I/usr/local/include/
CFLAGS3=-g -pass-exit-codes
DEPS=
LIBS=-lm -lrt -lpthread  -lcrypto -lssl -lmbedcrypto -lmbedtls -lmbedx509 -lwebsockets
OBJ=LDS_StartServer.o LDS_mainServer.o

ODIR=obj
LDIR1=-L/usr/local/lib/
LDIR2=-L/usr/lib/
LDFLAGS=-L$(HOME)build/bin/ -l:libopen62541.a

all: myNewLDSServer

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS1) $(CFLAGS2) $(CFLAGS3) -c  $< -o $@

myNewLDSServer: $(OBJ)
	$(CC) $(LDIR1) $(LDIR2) -o $@ $^ $(LDFLAGS) $(LIBS)


#CC=gcc
#CFLAGS1=-g -std=c99 -v
#CFLAGS2=-I/home/pi/open62541/ -I/home/pi/open62541/include -I/home/pi/open6251/plugins -I/usr/local/include
#DEPS=open62541.h
#LIBS=-lm -lrt -lpthread  -lcrypto -lssl -lmbedcrypto -lmbedtls -lmbedx509 -lwebsockets
#OBJ=open62541.o LDS_StartServer.o LDS_mainServer.o

#ODIR=obj
#LDIR1=/usr/local/lib
#LDIR2=/usr/lib

#all: myNewLDSServer

#%.o: %.c $(DEPS)
#	$(CC) -c -o $@ $< $(CFLAGS1) $(CFLAGS2)

#myNewLDSServer: $(OBJ)
#	$(CC) -o $@ $^ $(CFLAGS1) $(CFLAGS2) -L $(LDIR1) -L $(LDIR2) $(LIBS)
