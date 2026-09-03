CXX:=g++-15
CXXFLAGS:=-std=c++17 -Wall -Wextra -MMD

OBJS = 

CLIENT_OBJS = client.o run_client.o
SERVER_OBJS = server.o run_server.o

RUN_CLIENT = run_client
RUN_SERVER = run_server

DEPS = $(CLIENT_OBJS:.o=.d) $(SERVER_OBJS:.o=.d) $(OBJS:.o=.d)

all: $(RUN_CLIENT) $(RUN_SERVER)

server: $(RUN_SERVER)
	./$(RUN_SERVER)
	
client: $(RUN_CLIENT)
	./$(RUN_CLIENT)

$(RUN_CLIENT): $(OBJS) $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(RUN_SERVER): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJS) $(CLIENT_OBJS) $(SERVER_OBJS) $(RUN_CLIENT) $(RUN_SERVER) *.d