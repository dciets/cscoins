FROM ubuntu:16.04
RUN apt-get update
RUN apt-get install -y python3
RUN apt-get install -y python3-pip
RUN apt-get install -y git
RUN pip3 install websockets
RUN pip3 install asyncio
RUN pip3 install pycrypto
RUN pip3 install posix_ipc
ADD . /cscoins
WORKDIR /cscoins/minerclient/
run make release
ENTRYPOINT python3 -m main
