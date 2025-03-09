HOME=/root/open62541/
CC=gcc
CFLAGS1=-g -std=c99 -Wall -Werror -Wno-implicit -Wfatal-errors
#CC=g++
#CFLAGS1=-x c++ -std=c++98 -v -fpermissive -Wno-literal-suffix -Wno-write-strings -Wno-long-long -Wno-return-type
CFLAGS2=-I$(HOME) -I$(HOME)include/ -I$(HOME)plugins/ -I$(HOME)src/ -I$(HOME)build/src_generated/ \
-I$(HOME)arch/ -I$(HOME)deps/ -I$(HOME)plugins/include/ -I/usr/local/include/ -I$(HOME)plugins/ -I$(HOME)include/ \
-I$(HOME)plugins/include/
CFLAGS3=-g -pass-exit-codes
DEPS=
LIBS=-lm -lrt -lpthread  -lcrypto -lssl -lmbedcrypto -lmbedtls -lmbedx509 -lwebsockets
#OBJ=LDS_mainServer.o LDS_StartServer.o
OBJ= LDS_Misc.o LDS_configureServer.o LDS_encryptServer.o LDS_main.o 

ODIR=obj
LDIR1=-L/usr/local/lib/
LDIR2=-L/usr/lib/
LDFLAGS=-L$(HOME)build/bin/ -l:libopen62541.a -l:liblibmdnsd.a

all: myNewLDSServer

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS1) $(CFLAGS2) $(CFLAGS3) -c  $< -o $@

myNewLDSServer: $(OBJ)
	$(CC) $(LDIR1) $(LDIR2) -o $@ $^ $(LDFLAGS) $(LIBS)

clean: 
	rm -f *.o

# Important notes:
# $(HOME)/build/src_generated => open62541/config.h is located here
# $(HOME)/plugins/include/ => open62541/client_config_default.h is located here
