FROM	debian:latest AS build
MAINTAINER	lufia <lufia@lufia.org>
ENV	PLAN9=/usr/local/plan9
RUN	apt-get update && \
	apt-get install -y gcc git
RUN	mkdir -p $PLAN9 && \
	cd $PLAN9 && \
	git clone https://github.com/9fans/plan9port . && \
	./INSTALL && \
	rm -rf \
		.git .gitignore \
		CHANGES CONTRIBUTING.md CONTRIBUTORS \
		dist face font lib lp \
		mac mail man news \
		postscript proto \
		src tmac troff unix
ADD	*.c mkfile /tmp/
RUN	cd /tmp && \
	mk install && \
	mk clean &&
	rm -f *.c mkfile

FROM	alpine:latest
ENV	PLAN9=/usr/local/plan9
COPY	--from=build $PLAN9 $PLAN9
RUN	mkdir /mnt/venti
VOLUME	["/mnt/venti"]
WORKDIR	/mnt/venti
