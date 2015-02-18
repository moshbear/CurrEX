#          Copyright Andrey Moshbear 2014-2015.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#

ifndef CLANG
ifndef GCC
CLANG=1
endif
endif

ifdef CLANG
ifdef GCC
	$(error can\'t define both CLANG and GCC)
endif
ifndef NOSAN
ifndef SAN
	SAN = address,undefined
else #ifndef SAN
	SAN = $(SAN),undefined
endif #ifndef SAN
	XSAN = -fsanitize=
endif #ifndef NOSAN
	_CC_ = clang
	_CXX_ = clang++ # $(XSAN)$(SAN)
endif #ifdef CLANG

ifdef GCC
ifdef CLANG
	$(error can\'t define both CLANG and GCC)
endif
# -fsanitize is gcc 4.9+
ifdef GCC49
ifndef NOSAN
ifndef SAN
	SAN = address,undefined
else
	SAN = $(SAN),undefined
endif #ifndef SAN
	XSAN = -fsanitize=
endif #ifndef NOSAN
endif #ifdef GCC49
	_CC_ = gcc
	_CXX_ = g++ $(XSAN)$(SAN)
endif

ifdef _CC_
	CC = $(_CC_)
endif
ifdef _CXX_
	CXX = $(_CXX_)
endif

ifndef CFLAGS_G
	CFLAGS_G = -pipe -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic \
		-Wno-padded -Wno-global-constructors -Wno-exit-time-destructors \
		-Wno-missing-prototypes \
		-I.
endif

ifndef CFLAGS_D
	CFLAGS_D = -O2 -g -ggdb
endif

ifndef LDFLAGS
	LDFLAGS = -lm -lpthread
endif

ifndef CXX11
	CXX11 = $(CXX) -std=c++11
endif

ifndef CFLAGS
	CFLAGS = $(CFLAGS_G) $(CFLAGS_D)
endif

ifndef CXXFLAGS
	CXXFLAGS = $(CFLAGS)
endif

ifndef PERL5_CORE
	PERL5_CORE =  $(shell dirname $(shell find /usr/lib/perl* -name 'libperl.so'))
endif

ifndef NOLIB
	L = -fPIC
endif

SWIG_WIGNORE_CLANG = -Wno-old-style-cast -Wno-sign-conversion -Wno-gnu-statement-expression \
		     -Wno-cast-align -Wno-implicit-fallthrough -Wno-missing-noreturn \
		     -Wno-shorten-64-to-32 -Wno-conversion -Wno-sign-compare \
		     -Wno-covered-switch-default -Wno-unused-macros -Wno-unused-parameter \
		     -Wno-unused-variable -Wno-used-but-marked-unused \
		     -Wno-unreachable-code-return

SWIG_WIGNORE = $(SWIG_WIGNORE_CLANG)

LDFLAGS_pruner =
LDFLAGS_http = -lboost_system -lpthread

.PHONY: all clean

all: Currex_backend.so

Currex_backend.so: Currex_backend_wrap.o c-print.o d.o graph.o g-rategraph.o labeled.o pruner.o 
	$(CXX11) $(L) $(LD_SAN) $(LDFLAGS) -shared $^ -o $@

Currex_backend_wrap.o: Currex_backend_wrap.cxx
	sed 's/#include <algorithm>/#undef seed\n&/' -i $<
	$(CXX11) $(L) $(CXXFLAGS) -c -o $@ $< -I$(PERL5_CORE) $(SWIG_WIGNORE)

Currex_backend_wrap.cxx: Currex_backend.i
	swig -perl -c++ -Wall $<

pruner.o: pruner.cc pruner.hh d.hh algo.hh c-print.hh g-common.hh
	$(CXX11) $(L) $(CXXFLAGS) -c -o $@ $<

graph.o: graph.cc graph.hh d.hh algo.hh c-print.hh g-common.hh g-color.hh g-rategraph.hh labeled.hh
	$(CXX11) $(L) $(CXXFLAGS) -c -o $@ $<

g-common.hh: util.hh

g-rategraph.hh: c-print.hh

%.o: %.cc %.hh
	$(CXX11) $(L) $(CXXFLAGS) -c -o $@ $<

%.hh:

clean:
	rm -f *.o *.gch core {pre,post}.dot  *.pm *_wrap.cxx
