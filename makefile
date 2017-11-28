CXX = g++
FLAGS = -std=c++14 -O3 -I include/ -Wall

all: aggr

threads_runner.o: threads_runner.cpp threads_runner.h aggregator.h
	$(CXX) -c $< $(FLAGS) -o $@

aggregator.o: aggregator.cpp aggregator.h
	$(CXX) -c $< $(FLAGS) -o $@

aggr: main.cpp threads_runner.o aggregator.o
	$(CXX) $^ $(FLAGS) -lpthread -o $@

clean:
	rm -f aggr threads_runner.o aggregator.o