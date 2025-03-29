# Compiler
CXX = g++
CXXFLAGS = -Wall -std=c++11

# Paths
SRC_DIR = src
CA_DIR = ca
OBJ_DIR = obj

# Source Files
SOURCES = $(SRC_DIR)/digital_will.cpp \
          $(SRC_DIR)/encryption.cpp \
          $(SRC_DIR)/networking.cpp

# Object Files
OBJECTS = $(OBJ_DIR)/digital_will.o \
          $(OBJ_DIR)/encryption.o \
          $(OBJ_DIR)/networking.o

# Targets
all: dwms ca/ca_server generate_data

# Main Digital Will Management System
dwms: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o dwms $(OBJECTS) -lssl -lcrypto

# CA Server
ca/ca_server: $(CA_DIR)/certificate_authority.cpp
	$(CXX) $(CXXFLAGS) -o ca/ca_server $(CA_DIR)/certificate_authority.cpp -lssl -lcrypto

# Generate CA Data
generate_data: $(SRC_DIR)/ca_data_generator.cpp $(SRC_DIR)/encryption.cpp
	$(CXX) $(CXXFLAGS) -o generate_data $(SRC_DIR)/ca_data_generator.cpp $(SRC_DIR)/encryption.cpp -lssl -lcrypto

# Object Compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -lssl -lcrypto

# Clean Generated Files
clean:
	rm -f dwms ca/ca_server generate_data $(OBJECTS)
	rm -rf $(OBJ_DIR)

