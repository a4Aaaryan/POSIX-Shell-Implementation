# Compiler
CXX = g++
CXXFLAGS = -Wall -g -Iinclude

# Linker flags
LDFLAGS = -lreadline

# Source files
SRCS = $(wildcard *.cpp)

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable name
TARGET = my_shell

# Default target
all: $(TARGET)

# Linking the executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile each .cpp to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the program
run: all
	./$(TARGET)

# Clean up compiled files
clean:
	rm -f $(OBJS) $(TARGET)
