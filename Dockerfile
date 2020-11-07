FROM ubuntu:latest

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
		avr-libc \
		avrdude \
		binutils-avr \
		gcc-avr \
		gdb-avr \
		git \
		make \
		openssh-client
