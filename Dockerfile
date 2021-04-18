FROM ubuntu:18.04
RUN apt-get update && \
    apt-get -y install vim build-essential module-assistant gcc-multilib g++-multilib && \
    apt-get clean