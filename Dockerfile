###########################################################################################################
#  Dockerfile : creates a docker image that runs a open62541 instance called Local Directory Server (LDS) #
###########################################################################################################

# -- get the OS 
FROM debian:bookworm
LABEL maintainer="jacky81100@yahoo.com"
SHELL ["/bin/bash", "-c"]

# -- update and upgrade OS patches
RUN apt-get update && apt-get upgrade -y && apt-get dist-upgrade -y 

RUN apt-get install apt-utils -y
# -- prepare the build environment for OPC62541
RUN apt-get install build-essential gcc pkg-config cmake python3 net-tools -y
RUN apt-get install cmake-curses-gui -y
RUN apt-get install check libsubunit-dev -y
RUN apt-get install libmbedtls-dev -y
RUN apt-get install wget -y

# -- remove default openssl libraries
RUN apt-get remove openssl -y

# -- install zlib
RUN apt-get install zlib1g-dev -y
# -- alternate - compile from source
#WORKDIR /root
#RUN wget https://github.com/madler/zlib/releases/download/v1.2.13/zlib-1.2.13.tar.gz
#RUN tar -xvf zlib-1.2.13.tar.gz
#WORKDIR /root/zlib-1.2.13
#RUN ./configure --prefix=/usr/local/zlib
#RUN make
#RUN make install

# -- reinstall openssl libraries
WORKDIR /root
RUN wget https://www.openssl.org/source/openssl-3.4.1.tar.gz
RUN tar -xf openssl-3.4.1.tar.gz
RUN pwd
WORKDIR /root/openssl-3.4.1
RUN ./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl/ shared zlib
RUN make
RUN make test
RUN make install
RUN sleep 5
WORKDIR /etc/ld.so.conf.d/
RUN touch openssl-open62541.conf 
RUN echo "/usr/local/ssl/lib64/" | tee -a /etc/ld.so.conf.d/openssl-open62541.conf
RUN export LD_LIBRARY_PATH=/usr/local/ssl/lib64/
WORKDIR /etc/profile.d
RUN echo "export LD_LIBRARY_PATH=/usr/local/ssl/lib64; ldconfig" | tee -a ssl_export_ld_library_path.sh
RUN sleep 10
RUN ldconfig -v

WORKDIR /etc/
#RUN echo ":/usr/local/ssl/bin" | tee -a environment 
#RUN source /etc/environment
RUN echo $PATH
RUN sleep 5
RUN echo $PATH
RUN sleep 5
RUN /usr/local/ssl/bin/openssl version -a
RUN sleep 5

# -- add websockets capability :: fail at step <cmake ..>
# -- library is installed  to /usr/local/include
#WORKDIR /root
#RUN apt-get install git -y
#RUN git clone https://libwebsockets.org/repo/libwebsockets
#WORKDIR /root/libwebsockets
#RUN mkdir build
#WORKDIR /root/libwebsockets/build
#RUN cmake ..
#RUN make
#RUN make install
#RUN ldconfig
#RUN pkg-config --modversion libwebsockets
#RUN apt-get remove git -y

# -- alternative - use apt-get
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN apt-get install libwebsockets-dev -y

# -- install other libraries needed for user-defined application e.g. open62541lds
WORKDIR /root
RUN apt-get install libjson-c-dev -y
RUN apt-get install libxml2-dev -y
RUN apt-get install mariadb-client -y
RUN apt-get install libmariadb3 libmariadb-dev -y
RUN apt-get install mosquitto-clients -y
RUN apt-get install net-tools proftpd nano -y
RUN apt-get install python3-sphinx -y
RUN apt-get install texlive-latex-recommended -y

# -- get the open62541 source from github
WORKDIR /root 
RUN apt-get install git -y
RUN git clone https://github.com/open62541/open62541.git --branch v1.4.9 -c advice.detachedHead=FALSE
WORKDIR /root/open62541
RUN git submodule update --init --recursive

# -- build the base open62541 libraries
WORKDIR /root/open62541
RUN mkdir build
WORKDIR /root/open62541/build
RUN cmake ..
RUN cmake -S .. -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib64/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib64/libssl.so -DUA_ARCHITECTURE=posix -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_NAMESPACE_ZERO=FULL

RUN sleep 5
WORKDIR /root/open62541/build/
RUN make
RUN export open62541_NODESET_DIR='/root/open62541/deps/ua-nodeset/Schema/'

# -- creates the volume in container
# 1. ldscerts44.pem are kept in /usr/local/ssl/certs/ softlink to /etc/ssl/certs/
# 2. ldsprivate-key.pem are kept in /usr/local/ssl/private/ softlink to /etc/ssl/private/
# 3. trustlist.crl is not used
VOLUME /usr/local/ssl/certs
VOLUME /usr/local/ssl/private
# VOLUME /usr/local/ssl/trustlist/trustlist.crl

# -- set the default environment variables for docker container, can be changed in docker-compose.yml
ENV PRODUCT_URI="http://lds.virtualskies.com.sg"
ENV MANUFACTURER_NAME="Virtual Skies"
ENV PRODUCT_NAME="Virtual Skies OPC UA LDS Server"
ENV APPLICATION_URI_SERVER="lds.virtualskies.com.sg"
ENV APPLICATION_NAME="OPCUA LDS Server based on open62541"
# ENV APPLICATION_URI - not used
ENV PRIVATEKEYLOC="/usr/local/ssl/private/ldsprivate-key.pem"
ENV SSLCERTIFICATELOC="/etc/ssl/certs/ldscert44.pem"

# -- set the listening port
EXPOSE 4840

# -- build the user-defined application
RUN export PRODUCT_URI="http://lds.virtualskies.com.sg"
RUN export MANUFACTURER_NAME="Virtual Skies"
RUN export PRODUCT_NAME="Virtual Skies OPC UA LDS Server"
RUN export APPLICATION_URI_SERVER="lds.virtualskies.com.sg"
RUN export APPLICATION_NAME="OPCUA LDS Server based on open62541"
RUN export PRIVATEKEYLOC="/usr/local/ssl/private/ldsprivate-key.pem"
RUN export SSLCERTIFICATELOC="/etc/ssl/certs/ldscert44.pem"


# --- force the cache to clear when the following are run
RUN sleep 2
RUN head -c 5 /dev/random > random_bytes && ls -la
RUN head -c 5 /dev/random > random_bytes && pwd

WORKDIR /root/
RUN git clone https://github.com/jackybek/LDS-44.git OPCUAProject
WORKDIR /root/OPCUAProject
RUN touch *.c
RUN make

# create a soft symbolic link
WORKDIR /etc/ssl/certs/
RUN ln -s /usr/local/ssl/certs/ldscert44.pem ldscert44.pem
RUN ls -la
WORKDIR /etc/ssl/private
RUN ln -s /usr/local/ssl/private/ldsprivate-key.pem ldsprivate-key.pem
RUN ls -la 

# -- link default certificate and key to correct directories
# if these 2 steps fail, then manually run the commands in the container
WORKDIR /usr/local/ssl/certs/
RUN ln -s /root/OPCUAProject/ldscert44.pem ldscert44.pem
RUN ls -la
WORKDIR /usr/local/ssl/private/
RUN ln -s /root/OPCUAProject/ldsprivate-key.pem ldsprivate-key.pem
RUN ls -la

# --- set the default command to execute when the container starts
WORKDIR /root/OPCUAProject/
#entrypoint ["/bin/bash", "-c", "/root/OPCUAProject/myNewLDSServer 127.0.0.1"]

# for testing only - just enter the container
entrypoint ["/bin/bash"]
