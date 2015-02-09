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
	_CXX_ = clang++ $(XSAN)$(SAN)
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
		-I.
endif

ifdef D
ifndef CFLAGS_D
	CFLAGS_D = -O0 -g -ggdb
endif
else
ifndef CFLAGS_D
	CFLAGS_D = -O2 -g -ggdb
endif
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

LDFLAGS_pruner =
LDFLAGS_json = -ljson-c
LDFLAGS_http = -lboost_system -lpthread
LDFLAGS_rates = $(LDFLAGS_json) $(LDFLAGS_http)
LDFLAGS_instr_ls = $(LDFLAGS_json) $(LDFLAGS_http)

.PHONY: all clean

all: run-instr-ls run-pruner run-rates run-graph run-eval main

main: d.o http.o instr-ls.o pruner.o rates.o graph.o labeled.o c-print.o eval.cc
	$(CXX11) $(CXXFLAGS) $(LDFLAGS) $(LDFLAGS_instr_ls) $(LDFLAGS_pruner) $(LDFLAGS_rates) -o $@ $^

run-eval: d.o run-eval.cc
	$(CXX11) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

run-graph: d.o labeled.o c-print.o graph.o run-graph.cc
	$(CXX11) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

run-instr-ls: http.o instr-ls.o run-instr-ls.cc
	$(CXX11) $(CXXFLAGS) $(LDFLAGS) $(LDFLAGS_instr_ls) -o $@ $^

run-pruner: d.o c-print.o pruner.o run-pruner.cc
	$(CXX11) $(CXXFLAGS) $(LDFLAGS) $(LDFLAGS_pruner) -o $@ $^

run-rates: http.o rates.o run-rates.cc 
	$(CXX11) $(CXXFLAGS) $(LDFLAGS) $(LDFLAGS_rates) -o $@ $^

instr-ls.o: instr-ls.cc instr-ls.hh http.hh
	$(CXX11) $(CXXFLAGS) -c -o $@ $<

pruner.o: pruner.cc pruner.hh d.hh algo.hh c-print.hh g-common.hh
	$(CXX11) $(CXXFLAGS) -c -o $@ $<

rates.o: rates.cc rates.hh http.hh
	$(CXX11) $(CXXFLAGS) -c -o $@ $<

graph.o: graph.cc graph.hh d.hh algo.hh c-print.hh g-common.hh g-color.hh g-rategraph.hh labeled.hh
	$(CXX11) $(CXXFLAGS) -c -o $@ $<

g-common.hh: util.hh

g-rategraph.hh: c-print.hh

%.o: %.cc %.hh
	$(CXX11) $(CXXFLAGS) -c -o $@ $<

%.hh:

clean:
	rm -f *.o *.gch core {pre,post}.dot run-{instr-ls,pruner,rates,graph,eval} main
