sudo ./killdocker-44.sh
sudo docker build -f dockerfile-44 -t opcuaserver44:v1.0 --tag 'open62541' .
sudo docker run -it -d --name opcuaserver44 opcuaserver44 bash
sudo docker exec -it opcuaserver44 /bin/bash
