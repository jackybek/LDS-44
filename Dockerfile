###########################################################################################################
#  Dockerfile : creates a docker image that runs a open62541 instance called Local Directory Server (LDS) #
###########################################################################################################

#########################
# -- get the OS 
#########################
FROM debian:bookworm
LABEL maintainer="jacky81100@yahoo.com"
SHELL ["/bin/bash", "-c"]

########################################
# -- update and upgrade OS patches
########################################
RUN apt-get update && apt-get upgrade -y && apt-get dist-upgrade -y 

RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y apt-utils
# -- prepare the build environment for OPC62541
RUN DEBIAN_FRONTEND="noninteractive" apt-get install build-essential pkg-config python3 net-tools iputils-ping cmake-curses-gui check libsubunit-dev libmbedtls-dev wget -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install lib32readline8 lib32readline-dev -y

###############################
# -- pre-requisites for GCC
###############################
RUN DEBIAN_FRONTEND="noninteractive" apt-get install flex zlib1g-dev g++ git -y
# -------DO NOT remove GCC otherwise git-clone will be very slow
#RUN DEBIAN_FRONTEND="noninteractive" apt-get remove gcc -y

#############################################################################
# -- build GCC from source to get the latest version
# -- https://iq.opengenus.org/build-gcc-from-source/
# -- get gcc from source using git-clone : https://gcc.gnu.org/gcc-14/
#############################################################################
RUN git clone git://gcc.gnu.org/git/gcc.git
WORKDIR /root/gcc
RUN git checkout releases/gcc-14.2.0
RUN ./contrib/download_prerequisites
WORKDIR /root
RUN mkdir objdir
WORKDIR /root/objdir
RUN ../gcc/configure --prefix=/usr/local/gcc14.2.0 --disable-multilib --with-system-zlib --enable-languages=c,c++ --program-suffix=14.2.0
RUN ulimit -m unlimited
RUN ulimit -v unlimited
RUN make -j4                     
RUN make install
WORKDIR /etc
RUN echo "PATH=/usr/local/gcc14.2.0/bin:"$PATH > environment 
# original environment : PATH=/root/cmake-3.31.6/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
RUN source /etc/environment
RUN echo $PATH
WORKDIR /usr/local/gcc14.2.0/bin
RUN ln -s gcc14.2.0 gcc
RUN gcc --version -a

WORKDIR /etc/profile.d
#binaries are installed in:/usr/local/gcc14.2.0/bin
#Libraries have been installed in: /usr/local/gcc14.2.0/lib
#If you ever happen to want to link against installed libraries
#in a given directory, LIBDIR, you must either use libtool, and
#specify the full pathname of the library, or use the `-LLIBDIR'
#flag during linking and do at least one of the following:
#   - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
#     during execution
#   - add LIBDIR to the `LD_RUN_PATH' environment variable
#     during linking
#   - use the `-Wl,-rpath -Wl,LIBDIR' linker flag
#   - have your system administrator add LIBDIR to `/etc/ld.so.conf'


####################################
# -- remove & reinstall openssl libraries
####################################
WORKDIR /root
RUN DEBIAN_FRONTEND="noninteractive" apt-get remove openssl -y
RUN wget https://www.openssl.org/source/openssl-3.4.1.tar.gz
RUN tar -xf openssl-3.4.1.tar.gz
RUN pwd
WORKDIR /root/openssl-3.4.1
RUN ./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl/ shared zlib
RUN make -j4
# RUN make test
RUN make install_sw
WORKDIR /etc/ld.so.conf.d/
RUN touch openssl-open62541.conf 
RUN echo "/usr/local/ssl/lib64/" | tee -a /etc/ld.so.conf.d/openssl-open62541.conf
RUN export LD_LIBRARY_PATH=/usr/local/ssl/lib64/
WORKDIR /etc/profile.d
RUN echo "export LD_LIBRARY_PATH=/usr/local/ssl/lib64; ldconfig" | tee -a ssl_export_ld_library_path.sh
RUN ldconfig -v
WORKDIR /etc/
#RUN echo ":/usr/local/ssl/bin" | tee -a environment 
#RUN source /etc/environment
RUN echo $PATH
RUN /usr/local/ssl/bin/openssl version -a

############################################################################
# -- build CMAKE from source to get the latest version : https://cmake.org
# -- https://markusthill.github.io/blog/2024/installing-cmake/
############################################################################
WORKDIR /root
RUN DEBIAN_FRONTEND="noninteractive" apt-get remove --purge --autoremove cmake -y
RUN wget https://cmake.org/files/v3.31/cmake-3.31.6.tar.gz
RUN tar -xvf cmake-3.31.6.tar.gz
WORKDIR cmake-3.31.6/
RUN ./configure
RUN gmake
RUN make install
WORKDIR /etc/profile.d
RUN echo "export CMAKE_ROOT=/root/cmake-3.31.6/bin/; sudo ldconfig" | tee -a cmake_export_CMAKE_root_path.sh
RUN ldconfig -v
WORKDIR /etc
#RUN echo "export PATH=export PATH=/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X11/bin:/root/cmake-3.31.6/bin:$PATH" | tee -a environment
RUN echo "PATH=/root/cmake-3.31.6/bin:"$PATH > environment
RUN source /etc/environment
RUN echo $PATH
RUN cmake --version -a
# -- alternative - use apt-get
# DEBIAN_FRONTEND="noninteractive" apt-get install cmake

######################################
# -- add websockets capability
######################################
# -- library is installed  to /usr/local/include
WORKDIR /root
RUN DEBIAN_FRONTEND="noninteractive" apt-get install git -y
RUN git clone https://libwebsockets.org/repo/libwebsockets
WORKDIR /root/libwebsockets
RUN mkdir build
WORKDIR /root/libwebsockets/build
RUN cmake ..
RUN make -j4
RUN make install
RUN ldconfig
RUN pkg-config --modversion libwebsockets
#RUN DEBIAN_FRONTEND="noninteractive" apt-get remove git -y
# -- alternative - use apt-get
#RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
#RUN DEBIAN_FRONTEND="noninteractive" apt-get install libwebsockets-dev -y

#######################################################################################
# -- install other libraries needed for user-defined application e.g. open62541lds
#######################################################################################
WORKDIR /root
RUN DEBIAN_FRONTEND="noninteractive" apt-get install libjson-c-dev -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install libxml2-dev -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install mariadb-client -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install libmariadb3 libmariadb-dev -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install mosquitto-clients -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install net-tools proftpd nano -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install python3-sphinx graphviz -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install python3-sphinx-rtd-theme -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install texlive-latex-recommended -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install libavahi-client-dev libavahi-common-dev -y

################################################
# -- get the open62541 source from github
################################################
WORKDIR /root 
#RUN DEBIAN_FRONTEND="noninteractive" apt-get install git -y
RUN git clone https://github.com/open62541/open62541.git --branch v1.4.9 -c advice.detachedHead=FALSE
WORKDIR /root/open62541
RUN git submodule update --init --recursive

##################################
# -- install options for cmake
##################################
RUN DEBIAN_FRONTEND="noninteractive" apt-get install biber -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install clang-format -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install clang-tidy -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install latex2html -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install texlive-xetex -y
RUN DEBIAN_FRONTEND="noninteractive" apt-get install xindy -y

###########################################
# -- build the base open62541 libraries
###########################################
WORKDIR /root/open62541
RUN mkdir build
WORKDIR /root/open62541/build
# -- clears cache
RUN rm CMakeCache.txt
RUN rm *.cmake
RUN rm -r CMakeFiles
RUN rm -r doc

# once gcc is updated to v14.2.0, need to modify the following
# -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 
# -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0
# -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0
RUN cmake -S .. -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0 -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib64/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib64/libssl.so -DUA_ARCHITECTURE=posix -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=100 -DUA_NAMESPACE_ZERO=FULL

WORKDIR /root/open62541/build/
RUN make -j4
RUN make doc
RUN make doc_pdf
RUN make latexpdf
RUN export open62541_NODESET_DIR='/root/open62541/deps/ua-nodeset/Schema/'

##########################################
# -- creates the volume in container
# 1. ldscerts44.pem are kept in /usr/local/ssl/certs/ softlink to /etc/ssl/certs/
# 2. ldsprivate-key.pem are kept in /usr/local/ssl/private/ softlink to /etc/ssl/private/
# 3. trustlist.crl is not used
##########################################
VOLUME /usr/local/ssl/certs
VOLUME /usr/local/ssl/private
# VOLUME /usr/local/ssl/trustlist/trustlist.crl

#########################################################################################################
# -- set the default environment variables for docker container, can be changed in docker-compose.yml
#########################################################################################################
ENV PRODUCT_URI="http://lds.virtualskies.com.sg"
ENV MANUFACTURER_NAME="Virtual Skies"
ENV PRODUCT_NAME="Virtual Skies OPC UA LDS Server"
ENV APPLICATION_URI_SERVER="lds.virtualskies.com.sg"
ENV APPLICATION_NAME="OPCUA LDS Server based on open62541"
# ENV APPLICATION_URI - not used
ENV PRIVATEKEYLOC="/usr/local/ssl/private/ldsprivate-key.pem"
ENV SSLCERTIFICATELOC="/usr/local/ssl/certs/ldscert44.pem"

#############################
# -- set the listening port
#############################
EXPOSE 4840

##########################################
# -- build the user-defined application
##########################################
RUN export PRODUCT_URI="http://lds.virtualskies.com.sg"
RUN export MANUFACTURER_NAME="Virtual Skies"
RUN export PRODUCT_NAME="Virtual Skies OPC UA LDS Server"
RUN export APPLICATION_URI_SERVER="lds.virtualskies.com.sg"
RUN export APPLICATION_NAME="OPCUA LDS Server based on open62541"
RUN export PRIVATEKEYLOC="/usr/local/ssl/private/ldsprivate-key.pem"
RUN export SSLCERTIFICATELOC="/usr/local/ssl/certs/ldscert44.pem"


###################################################################
# --- force the cache to clear when the following are run
###################################################################
##### run <<sudo docker build -t open62541lds --cache-from=a7d330e2ccdf .>> where --cache-from is the value before <<git clone>>
RUN apt-get update && apt-get upgrade -y && apt-get dist-upgrade -y

RUN ls -la 
WORKDIR /root/
RUN git clone https://github.com/jackybek/LDS-44.git OPCUAProject
WORKDIR /root/OPCUAProject
RUN touch *.c
RUN make clean; make all

####################################
# create a soft symbolic link
####################################
WORKDIR /etc/ssl/certs/
RUN ln -s /usr/local/ssl/certs/ldscert44.pem ldscert44.pem
WORKDIR /etc/ssl/private
RUN ln -s /usr/local/ssl/private/ldsprivate-key.pem ldsprivate-key.pem

#########################################################################################
# -- link default (from docker hub) certificate and key to correct directories
# if these 2 steps fail, then manually run the commands in the container
#########################################################################################
WORKDIR /usr/local/ssl/certs/
RUN ln -s /root/OPCUAProject/ldscert44.pem ldscert44.pem
WORKDIR /usr/local/ssl/private/
RUN ln -s /root/OPCUAProject/ldsprivate-key.pem ldsprivate-key.pem

#########################################################################
# --- set the default command to execute when the container starts
#########################################################################
WORKDIR /root/OPCUAProject/
#entrypoint ["/bin/bash", "-c", "/root/OPCUAProject/myNewLDSServer 127.0.0.1"]

# for testing only - just enter the container
entrypoint ["/bin/bash"]
