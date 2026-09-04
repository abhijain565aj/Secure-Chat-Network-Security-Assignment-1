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
PROXY_FAKE_PORT := 12347
PROXY_THIEF_PORT := 12348

CERTIF_FILES := $(BUILD_DIR)/certs/ca.key \
				 $(BUILD_DIR)/certs/ca.crt \
				 $(BUILD_DIR)/certs/server.key \
				 $(BUILD_DIR)/certs/server.crt \
				 $(BUILD_DIR)/certs/server.csr

PROXY_CERTIF_FILES := $(BUILD_DIR)/proxy-certs/ca.key \
					 $(BUILD_DIR)/proxy-certs/ca.crt \
					 $(BUILD_DIR)/proxy-certs/server.key \
					 $(BUILD_DIR)/proxy-certs/server.crt \
					 $(BUILD_DIR)/proxy-certs/server.csr

GENERATOR_SCRIPT := src/crypto/generate_certs.sh

.PHONY: all client server proxy proxy-fake proxy-thief victim-client victim-client-fake victim-client-thief clean

all: $(RUN_CLIENT) $(RUN_SERVER) $(RUN_PROXY) $(CERTIF_FILES)

client: $(RUN_CLIENT) $(CERTIF_FILES)
	./$(RUN_CLIENT) $(SERVER_IP) $(SERVER_PORT) $(BUILD_DIR)/certs

server: $(RUN_SERVER) $(CERTIF_FILES)
	./$(RUN_SERVER) $(SERVER_PORT) $(BUILD_DIR)/certs

proxy: $(RUN_PROXY)
	./$(RUN_PROXY) $(PROXY_PORT) $(SERVER_IP) $(SERVER_PORT)

proxy-fake: $(RUN_PROXY) $(PROXY_CERTIF_FILES) $(CERTIF_FILES)
	./$(RUN_PROXY) fake $(PROXY_FAKE_PORT) $(SERVER_IP) $(SERVER_PORT) $(BUILD_DIR)/proxy-certs

proxy-thief: $(RUN_PROXY) $(CERTIF_FILES)
	./$(RUN_PROXY) thief $(PROXY_THIEF_PORT) $(SERVER_IP) $(SERVER_PORT) $(BUILD_DIR)/certs

victim-client: $(RUN_CLIENT) $(CERTIF_FILES)
	./$(RUN_CLIENT) $(PROXY_IP) $(PROXY_PORT) $(BUILD_DIR)/certs

victim-client-fake: $(RUN_CLIENT) $(CERTIF_FILES) $(PROXY_CERTIF_FILES)
	./$(RUN_CLIENT) $(PROXY_IP) $(PROXY_FAKE_PORT) $(BUILD_DIR)/certs

victim-client-thief: $(RUN_CLIENT) $(CERTIF_FILES)
	./$(RUN_CLIENT) $(PROXY_IP) $(PROXY_THIEF_PORT) $(BUILD_DIR)/certs

$(RUN_CLIENT): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o $@

$(RUN_SERVER): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o $@

$(RUN_PROXY): $(PROXY_OBJS) $(COMMON_OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(CERTIF_FILES): $(GENERATOR_SCRIPT) $(BUILD_DIR) 
	bash $(GENERATOR_SCRIPT) ${BUILD_DIR}

$(PROXY_CERTIF_FILES): $(GENERATOR_SCRIPT) $(BUILD_DIR)
	bash $(GENERATOR_SCRIPT) ${BUILD_DIR} proxy-certs

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)