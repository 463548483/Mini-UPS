FROM ubuntu:20.04
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update
RUN apt-get install -y g++ make
RUN apt-get install -y postgresql postgresql-client
RUN apt-get install -y libpqxx-dev libtbb-dev libboost-all-dev
RUN apt-get install -y protobuf-compiler
RUN apt-get install -y nlohmann-json3-dev
RUN mkdir /ups_server
WORKDIR /ups_server
ADD . /ups_server
RUN cd /ups_server
RUN make clean
