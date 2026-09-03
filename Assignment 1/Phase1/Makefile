UNAME_S := $(shell uname -s)

CXXFLAGS := -std=c++17 -Wall -Wextra -MMD -MP
CPPFLAGS := -Isrc -Iinclude
LDLIBS := -lcrypto -lncurses

ifeq ($(UNAME_S),Linux)
    CXX := g++
else ifeq ($(UNAME_S),Darwin)
    CXX := g++-15
	OPENSSL_DIR := $(shell brew --prefix openssl)
    CXXFLAGS += -I$(OPENSSL_DIR)/include
    LDFLAGS  += -L$(OPENSSL_DIR)/lib
endif

BUILD_DIR := build

CLIENT_SRCS := $(wildcard src/client/*.cpp)
SERVER_SRCS := $(wildcard src/server/*.cpp)
COMMON_SRCS := $(wildcard src/utils/*.cpp) $(wildcard src/chat/*.cpp)

CLIENT_OBJS := $(CLIENT_SRCS:src/%.cpp=$(BUILD_DIR)/%.o)
SERVER_OBJS := $(SERVER_SRCS:src/%.cpp=$(BUILD_DIR)/%.o)
COMMON_OBJS := $(COMMON_SRCS:src/%.cpp=$(BUILD_DIR)/%.o)

RUN_CLIENT := $(BUILD_DIR)/run_client
RUN_SERVER := $(BUILD_DIR)/run_server

DEPS := $(CLIENT_OBJS:.o=.d) \
        $(SERVER_OBJS:.o=.d) \
        $(COMMON_OBJS:.o=.d)

.PHONY: all client server clean

all: $(RUN_CLIENT) $(RUN_SERVER)

client: $(RUN_CLIENT)
	./$(RUN_CLIENT)

server: $(RUN_SERVER)
	./$(RUN_SERVER)

$(RUN_CLIENT): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o $@

$(RUN_SERVER): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)