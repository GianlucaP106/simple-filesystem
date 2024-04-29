FROM ubuntu:latest

RUN apt-get update &&  \
    apt-get install -y build-essential &&  \
    apt-get install -y valgrind &&  \
    apt-get install -y vim && \
    apt-get install -y make && \
    apt-get install -y pkg-config && \
    apt-get install -y fuse && \
    apt-get install -y libfuse-dev

WORKDIR /app
COPY . /app

RUN make

CMD [ "bash" ]
