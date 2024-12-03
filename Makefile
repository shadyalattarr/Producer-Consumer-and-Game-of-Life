# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++11

# Targets and source files
PRODUCER_SRC = producer.cpp
PRODUCER_BIN = producer
CONSUMER_SRC = consumer.cpp
CONSUMER_BIN = consumer

# Default target
all: $(PRODUCER_BIN) $(CONSUMER_BIN)

# Compile producer
$(PRODUCER_BIN): $(PRODUCER_SRC)
	$(CXX) $(CXXFLAGS) -o $(PRODUCER_BIN) $(PRODUCER_SRC)

# Compile consumer
$(CONSUMER_BIN): $(CONSUMER_SRC)
	$(CXX) $(CXXFLAGS) -o $(CONSUMER_BIN) $(CONSUMER_SRC)

# Clean up
clean:
	rm -f $(PRODUCER_BIN) $(CONSUMER_BIN)
