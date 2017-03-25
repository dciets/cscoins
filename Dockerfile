FROM ubuntu
RUN apt-get update
# RUN apt-get install -y gcc
RUN apt-get install -y python3
RUN apt-get install -y python3-pip
RUN apt-get install -y git
RUN apt-get install -y openssl
RUN apt-get install -y libssl-dev
RUN pip3 install websockets
RUN pip3 install asyncio
RUN pip3 install pycrypto
RUN git clone https://f5be9eb9ff8353cb816eb736fa5936cffddea92d@github.com/dciets/cscoins.git
WORKDIR cscoins/minerclient/
run git checkout 
run make
ENTRYPOINT python3 main.py
