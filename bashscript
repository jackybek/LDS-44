#!/bin/bash
###########################################################################################################
#  bash file : creates a image that runs a open62541 instance called Local Directory Server (LDS) #
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
sudo apt-get update && apt-get upgrade -y && apt-get dist-upgrade -y 

# -- prepare the build environment for OPC62541
sudo DEBIAN_FRONTEND="noninteractive" apt-get install build-essential pkg-config python3 net-tools iputils-ping cmake-curses-gui check libsubunit-dev libmbedtls-dev wget -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install apt-utils lib32readline8 lib32readline-dev -y

###############################
# -- pre-requisites for GCC
###############################
sudo DEBIAN_FRONTEND="noninteractive" apt-get install flex zlib1g-dev g++ git -y
# -------DO NOT remove GCC otherwise git-clone will be very slow
#sudo DEBIAN_FRONTEND="noninteractive" apt-get remove gcc -y

#############################################################################
# -- build GCC from source to get the latest version
# -- https://iq.opengenus.org/build-gcc-from-source/
# -- get gcc from source using git-clone : https://gcc.gnu.org/gcc-14/
#############################################################################
sudo git clone git://gcc.gnu.org/git/gcc.git
cd /root/gcc
sudo git checkout releases/gcc-14.2.0
sudo ./contrib/download_prerequisites
cd /root
sudo mkdir objdir
cd /root/objdir
sudo ../gcc/configure --prefix=/usr/local/gcc14.2.0 --disable-multilib --with-system-zlib --enable-languages=c,c++ --program-suffix=14.2.0
sudo ulimit -m unlimited
sudo ulimit -v unlimited
sudo make -j4                     
sudo make install
cd /etc
sudo echo "PATH=/usr/local/gcc14.2.0/bin:"$PATH > environment 
# original environment : PATH=/root/cmake-3.31.6/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
sudo source /etc/environment
sudo echo $PATH
cd /usr/local/gcc14.2.0/bin
sudo ln -s gcc14.2.0 gcc
sudo gcc --version -a

cd /etc/profile.d
#binaries are installed in:/usr/local/gcc14.2.0/bin
#Libraries have been installed in: /usr/local/gcc14.2.0/lib
#If you ever happen to want to link against installed libraries
#in a given directory, LIBDIR, you must either use libtool, and
#specify the full pathname of the library, or use the `-LLIBDIR'
#flag during linking and do at least one of the following:
#   - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
#     during execution
#   - add LIBDIR to the `LD_sudo_PATH' environment variable
#     during linking
#   - use the `-Wl,-rpath -Wl,LIBDIR' linker flag
#   - have your system administrator add LIBDIR to `/etc/ld.so.conf'


####################################
# -- remove & reinstall openssl libraries
####################################
cd /root
sudo DEBIAN_FRONTEND="noninteractive" apt-get remove openssl -y
sudo wget https://www.openssl.org/source/openssl-3.4.1.tar.gz
sudo tar -xf openssl-3.4.1.tar.gz
sudo pwd
cd /root/openssl-3.4.1
sudo ./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl/ shared zlib
sudo make -j4
# sudo make test
sudo make install_sw
cd /etc/ld.so.conf.d/
sudo touch openssl-open62541.conf 
sudo echo "/usr/local/ssl/lib64/" | tee -a /etc/ld.so.conf.d/openssl-open62541.conf
sudo export LD_LIBRARY_PATH=/usr/local/ssl/lib64/
cd /etc/profile.d
sudo echo "export LD_LIBRARY_PATH=/usr/local/ssl/lib64; ldconfig" | tee -a ssl_export_ld_library_path.sh
sudo ldconfig -v
cd /etc/
#sudo echo ":/usr/local/ssl/bin" | tee -a environment 
#sudo source /etc/environment
sudo echo $PATH
sudo /usr/local/ssl/bin/openssl version -a

############################################################################
# -- build CMAKE from source to get the latest version : https://cmake.org
# -- https://markusthill.github.io/blog/2024/installing-cmake/
############################################################################
cd /root
sudo DEBIAN_FRONTEND="noninteractive" apt-get remove --purge --autoremove cmake -y
sudo wget https://cmake.org/files/v3.31/cmake-3.31.6.tar.gz
sudo tar -xvf cmake-3.31.6.tar.gz
cd /root/cmake-3.31.6/
sudo ./configure
sudo gmake
sudo make install
cd /etc/profile.d
sudo echo "export CMAKE_ROOT=/root/cmake-3.31.6/bin/; sudo ldconfig" | tee -a cmake_export_CMAKE_root_path.sh
sudo ldconfig -v
cd /etc
#sudo echo "export PATH=export PATH=/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X11/bin:/root/cmake-3.31.6/bin:$PATH" | tee -a environment
sudo echo "PATH=/root/cmake-3.31.6/bin:"$PATH > environment
sudo source /etc/environment
sudo echo $PATH
sudo cmake --version -a
# -- alternative - use apt-get
# DEBIAN_FRONTEND="noninteractive" apt-get install cmake

######################################
# -- add websockets capability
######################################
# -- library is installed  to /usr/local/include
cd /root
sudo DEBIAN_FRONTEND="noninteractive" apt-get install git -y
sudo git clone https://libwebsockets.org/repo/libwebsockets
cd /root/libwebsockets
sudo mkdir build
cd /root/libwebsockets/build
sudo cmake ..
sudo make -j4
sudo make install
sudo ldconfig
sudo pkg-config --modversion libwebsockets
#sudo DEBIAN_FRONTEND="noninteractive" apt-get remove git -y
# -- alternative - use apt-get
#sudo echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
#sudo DEBIAN_FRONTEND="noninteractive" apt-get install libwebsockets-dev -y

#######################################################################################
# -- install other libraries needed for user-defined application e.g. open62541lds
#######################################################################################
cd /root
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libjson-c-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libxml2-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install mariadb-client -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libmariadb3 libmariadb-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install mosquitto-clients -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install net-tools proftpd nano -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install python3-sphinx graphviz -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install python3-sphinx-rtd-theme -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install texlive-latex-recommended -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libavahi-client-dev libavahi-common-dev -y

################################################
# -- get the open62541 source from github
################################################
cd /root 
#sudo DEBIAN_FRONTEND="noninteractive" apt-get install git -y
sudo git clone https://github.com/open62541/open62541.git --branch v1.4.9 -c advice.detachedHead=FALSE
cd /root/open62541
sudo git submodule update --init --recursive

##################################
# -- install options for cmake
##################################
sudo DEBIAN_FRONTEND="noninteractive" apt-get install biber -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install clang-format -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install clang-tidy -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install latex2html -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install texlive-xetex -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install xindy -y

###########################################
# -- build the base open62541 libraries
###########################################
cd /root/open62541
sudo mkdir build
cd /root/open62541/build
# -- clears cache
sudo rm CMakeCache.txt
sudo rm *.cmake
sudo rm -r CMakeFiles
sudo rm -r doc

# once gcc is updated to v14.2.0, need to modify the following
# -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 
# -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0
# -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0
sudo cmake -S .. -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0 \
-DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib64/libcrypto.so \
-DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib64/libssl.so -DUA_ARCHITECTURE=posix -DUA_DEBUG_FILE_LINE_INFO=ON \
-DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON \
-DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON \
-DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF \
-DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON \
-DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON \
-DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=100 -DUA_NAMESPACE_ZERO=FULL

cd /root/open62541/build/
sudo make -j4
sudo make doc
sudo make doc_pdf
sudo make latexpdf
sudo export open62541_NODESET_DIR='/root/open62541/deps/ua-nodeset/Schema/'

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
sudo export PRODUCT_URI="http://lds.virtualskies.com.sg"
sudo export MANUFACTURER_NAME="Virtual Skies"
sudo export PRODUCT_NAME="Virtual Skies OPC UA LDS Server"
sudo export APPLICATION_URI_SERVER="lds.virtualskies.com.sg"
sudo export APPLICATION_NAME="OPCUA LDS Server based on open62541"
sudo export PRIVATEKEYLOC="/usr/local/ssl/private/ldsprivate-key.pem"
sudo export SSLCERTIFICATELOC="/usr/local/ssl/certs/ldscert44.pem"


###################################################################
# --- force the cache to clear when the following are run
###################################################################
##### run <<sudo docker build -t open62541lds --cache-from=a7d330e2ccdf .>> where --cache-from is the value before <<git clone>>
sudo apt-get update && apt-get upgrade -y && apt-get dist-upgrade -y

sudo ls -la 
cd /root/
sudo git clone https://github.com/jackybek/LDS-44.git OPCUAProject
cd /root/OPCUAProject
sudo touch *.c
sudo make clean; make all

####################################
# create a soft symbolic link
####################################
cd /etc/ssl/certs/
sudo ln -s /usr/local/ssl/certs/ldscert44.pem ldscert44.pem
cd /etc/ssl/private
sudo ln -s /usr/local/ssl/private/ldsprivate-key.pem ldsprivate-key.pem

#########################################################################################
# -- link default (from docker hub) certificate and key to correct directories
# if these 2 steps fail, then manually run the commands in the container
#########################################################################################
cd /usr/local/ssl/certs/
sudo ln -s /root/OPCUAProject/ldscert44.pem ldscert44.pem
cd /usr/local/ssl/private/
sudo ln -s /root/OPCUAProject/ldsprivate-key.pem ldsprivate-key.pem

#########################################################################
# --- set the default command to execute when the container starts
#########################################################################
cd /root/OPCUAProject/
#entrypoint ["/bin/bash", "-c", "/root/OPCUAProject/myNewLDSServer 127.0.0.1"]

# for testing only - just enter the container
entrypoint ["/bin/bash"]
