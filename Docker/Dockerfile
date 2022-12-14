#开发环境 linux gcc g++ mysql-server libmysqlclient-dev c++17
FROM ubuntu
RUN set -x; buildDeps='g++ libmysqlclient-dev' \  
&& apt-get update \
&& apt-get install -y $buildDeps\
&& cd /root\
&& mkdir linuxWebserver