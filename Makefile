#COMAKE2 edit-mode: -*- Makefile -*-
####################64Bit Mode####################
ifeq ($(shell uname -m),x86_64)
CC=gcc
CXX=g++
CXXFLAGS=-g \
  -pipe \
  -W \
  -O \
  -Wall \
  -fPIC \
  -DDEBUG
CFLAGS=-g \
  -pipe \
  -W \
  -O \
  -Wall \
  -fPIC \
  -DDEBUG
CPPFLAGS=-D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS
CUR_PATH=./
INCPATH=-I$(CUR_PATH)zk/include \
  -I$(CUR_PATH)zk/include/zookeeper \
  -I$(CUR_PATH)yaml/include

#============ CCP vars ============
syncobject=src/cs.o \
    src/cs_option.o \
    src/cs_md5.o \
    src/cs_log.o \
    src/cs_signal.o \
    src/cs_util.o \
    src/cs_daemon.o \
    src/cs_sync.o \
    src/cs_epoll.o \
    src/cs_zk.o \
    src/cs_conf.o

.PHONY:all
all: comsync csm
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mall[0m']"
	@echo "make all done"

.PHONY:clean
clean:
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mclean[0m']"
	rm -rf csm
	rm -rf src/*.o

comsync:$(syncobject)
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcomsync[0m']"
	$(CXX) $(syncobject) \
    -Xlinker "-(" $(CUR_PATH)zk/lib/libzookeeper_mt.a \
    $(CUR_PATH)yaml/lib/libyaml.a \
    -lpthread -lidn -lrt -lz -ldl \
    -rdynamic -Xlinker "-)" -o comsync
	mkdir -p ./csm/bin
	mv -f comsync ./csm/bin
	cp -rf conf csm

#========================comsync===========================================
src/cs.o:src/cs.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs.o src/cs.cpp

src/cs_option.o:src/cs_option.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_option.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_option.o src/cs_option.cpp

src/cs_md5.o:src/cs_md5.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_md5.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_md5.o src/cs_md5.cpp

src/cs_log.o:src/cs_log.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_log.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_log.o src/cs_log.cpp

src/cs_signal.o:src/cs_signal.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_signal.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_signal.o src/cs_signal.cpp

src/cs_util.o:src/cs_util.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_util.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_util.o src/cs_util.cpp

src/cs_daemon.o:src/cs_daemon.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_daemon.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_daemon.o src/cs_daemon.cpp

src/cs_sync.o:src/cs_sync.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_sync.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_sync.o src/cs_sync.cpp

src/cs_epoll.o:src/cs_epoll.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_epoll.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_epoll.o src/cs_epoll.cpp

src/cs_zk.o:src/cs_zk.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_zk.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_zk.o src/cs_zk.cpp

src/cs_conf.o:src/cs_conf.cpp
	@echo "[[1;32;40mMAKE:BUILD[0m][Target:'[1;32;40mcs_conf.o[0m']"
	$(CXX) -c $(INCPATH) -I./src $(CPPFLAGS) $(CXXFLAGS) -o src/cs_conf.o src/cs_conf.cpp

endif #ifeq ($(shell uname -m),x86_64)
