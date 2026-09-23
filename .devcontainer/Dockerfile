FROM debian:bookworm

WORKDIR /root

RUN apt-get update

# https://pip-assets.raspberrypi.com/categories/610-raspberry-pi-pico/documents/RP-008276-DS-1-getting-started-with-pico.pdf page 32
RUN apt-get install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
RUN apt-get install -y g++ libstdc++-arm-none-eabi-newlib
# also needed
RUN apt-get install -y python3 python3-venv curl sudo
# pico setup
RUN curl https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh | bash
# required for dev container
RUN apt-get install -y libicu-dev

ENV PICO_PLATFORM="rp2350"


