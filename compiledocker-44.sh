sudo ./killdocker-44.sh
sudo docker build -f dockerfile-44 -t opcualdsserver44:v1.0 --tag 'lds-44' .
sudo docker run -it -d --name opcualdsserver44 opcualdsserver44 bash
sudo docker exec -it opcualdsserver44 /bin/bash
