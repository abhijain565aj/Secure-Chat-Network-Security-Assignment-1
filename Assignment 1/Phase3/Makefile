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
PROXY_SRCS := $(wildcard src/proxy/*.cpp)
COMMON_SRCS := $(wildcard src/utils/*.cpp) $(wildcard src/chat/*.cpp) $(wildcard src/crypto/*.cpp)

CLIENT_OBJS := $(CLIENT_SRCS:src/%.cpp=$(BUILD_DIR)/%.o)
SERVER_OBJS := $(SERVER_SRCS:src/%.cpp=$(BUILD_DIR)/%.o)
PROXY_OBJS := $(PROXY_SRCS:src/%.cpp=$(BUILD_DIR)/%.o)
COMMON_OBJS := $(COMMON_SRCS:src/%.cpp=$(BUILD_DIR)/%.o)

RUN_CLIENT := $(BUILD_DIR)/run_client
RUN_SERVER := $(BUILD_DIR)/run_server
RUN_PROXY := $(BUILD_DIR)/run_proxy

DEPS := $(CLIENT_OBJS:.o=.d) \
        $(SERVER_OBJS:.o=.d) \
		$(PROXY_OBJS:.o=.d) \
        $(COMMON_OBJS:.o=.d)

SERVER_IP := "127.0.0.1"
SERVER_PORT := 12345
PROXY_IP := "127.0.0.1"
PROXY_PORT := 12346

CERTIF_FILES := $(BUILD_DIR)/certs/ca.key \
				 $(BUILD_DIR)/certs/ca.crt \
				 $(BUILD_DIR)/certs/server.key \
				 $(BUILD_DIR)/certs/server.crt \
				 $(BUILD_DIR)/certs/server.csr

.PHONY: all client server proxy clean

all: $(RUN_CLIENT) $(RUN_SERVER) $(RUN_PROXY)

client: $(RUN_CLIENT) $(CERTIF_FILES)
	./$(RUN_CLIENT) $(SERVER_IP) $(SERVER_PORT)

server: $(RUN_SERVER) $(CERTIF_FILES)
	./$(RUN_SERVER) $(SERVER_PORT)

proxy: $(RUN_PROXY) $(CERTIF_FILES)
	./$(RUN_PROXY) $(PROXY_PORT) $(SERVER_IP) $(SERVER_PORT)

victim-client: $(RUN_CLIENT) $(CERTIF_FILES)
	./$(RUN_CLIENT) $(PROXY_IP) $(PROXY_PORT)

$(RUN_CLIENT): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o $@

$(RUN_SERVER): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o $@

$(RUN_PROXY): $(PROXY_OBJS) $(COMMON_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(CERTIF_FILES): $(BUILD_DIR)
	bash src/ca/generate_certs.sh ${BUILD_DIR}

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)