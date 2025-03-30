kCXX = g++
CXXFLAGS = -Wall -std=c++11

SRC_DIR = src
CA_DIR = ca
OBJ_DIR = obj

SOURCES = $(SRC_DIR)/digital_will.cpp \
          $(SRC_DIR)/encryption.cpp \
          $(SRC_DIR)/networking.cpp

OBJECTS = $(OBJ_DIR)/digital_will.o \
          $(OBJ_DIR)/encryption.o \
          $(OBJ_DIR)/networking.o

life: dwms will home ca/ca_server generate_data

dwms: $(OBJECTS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o bin/dwms $(OBJECTS) -lssl -lcrypto

will: $(SRC_DIR)/will.cpp $(OBJ_DIR)/encryption.o $(OBJ_DIR)/networking.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o bin/will $(SRC_DIR)/will.cpp $(OBJ_DIR)/encryption.o $(OBJ_DIR)/networking.o -lssl -lcrypto

home: $(SRC_DIR)/home.cpp $(OBJ_DIR)/encryption.o $(OBJ_DIR)/networking.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o bin/home $(SRC_DIR)/home.cpp $(OBJ_DIR)/encryption.o $(OBJ_DIR)/networking.o -lssl -lcrypto

ca/ca_server: $(CA_DIR)/certificate_authority.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o bin/ca_server $(CA_DIR)/certificate_authority.cpp -lssl -lcrypto

generate_data: $(SRC_DIR)/ca_data_generator.cpp $(OBJ_DIR)/encryption.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o bin/generate_data $(SRC_DIR)/ca_data_generator.cpp $(OBJ_DIR)/encryption.o -lssl -lcrypto

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -lssl -lcrypto

cleanse:
	rm -f bin/dwms bin/will bin/home bin/ca_server bin/generate_data
	rm -rf $(OBJ_DIR) bin

