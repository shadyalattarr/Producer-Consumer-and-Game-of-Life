# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++11

# Targets and source files
WRITER_SRC = writer.cpp
WRITER_BIN = writer
READER_SRC = reader.cpp
READER_BIN = reader

# Default target
all: $(WRITER_BIN) $(READER_BIN)

# Compile writer
# g++ writer.cpp -o writer
$(WRITER_BIN): $(WRITER_SRC)
	$(CXX) $(CXXFLAGS) -o $(WRITER_BIN) $(WRITER_SRC)

# Compile reader
$(READER_BIN): $(READER_SRC)
	$(CXX) $(CXXFLAGS) -o $(READER_BIN) $(READER_SRC)

# Clean up
clean:
	rm -f $(WRITER_BIN) $(READER_BIN)
