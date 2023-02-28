FROM ubuntu:20.04
RUN mkdir /code
WORKDIR /code
ENV TZ=GMT
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get update && apt-get -y --no-install-recommends install \
    g++ make libpq-dev libssl-dev libxerces-c-dev libpqxx-dev libboost-all-dev

ADD Makefile Proxy.hpp Proxy.cpp Cache.hpp Cache.cpp run.sh  /code/
RUN mkdir /code/boost_1_81_0
ADD Makefile Proxy.hpp Proxy.cpp Cache.hpp Cache.cpp  /code/
RUN make proxy
CMD ["bash","run.sh"]
