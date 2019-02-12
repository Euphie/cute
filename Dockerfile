FROM alpine:3.6

ENV CUTE_ADDR 0.0.0.0
ENV CUTE_PORT 7777
ENV CUTE_ACCOUTS cute:123456

ADD https://github.com/Euphie/cute/archive/master.tar.gz /tmp

WORKDIR /tmp

RUN tar -zxvf master.tar.gz && \
	rm -rf cute-master/bin/* && \
	cd cute-master/src && \
	apk update --no-cache && \
	apk add --virtual BUILD build-base && \
	make && \
	apk del BUILD

EXPOSE $CUTE_PORT
CMD cute-master/bin/cute -l $CUTE_ADDR -p $CUTE_PORT -a $CUTE_ACCOUTS
