CXX:=g++-15
CXXFLAGS:=-std=c++17 -Wall -Wextra -MMD


OBJS = client.o server.o

RUN_CLIENT = run_client
RUN_CLIENT_OBJ = run_client.o

RUN_SERVER = run_server
RUN_SERVER_OBJ = run_server.o

DEPS = $(OBJS:.o=.d)

all: $(RUN_CLIENT) $(RUN_SERVER)

server: $(RUN_SERVER)
	./$(RUN_SERVER)
	
client: $(RUN_CLIENT)
	./$(RUN_CLIENT)

$(RUN_CLIENT): $(OBJS) $(RUN_CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(RUN_SERVER): $(OBJS) $(RUN_SERVER_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(OBJS) $(RUN_CLIENT_OBJ) $(RUN_SERVER_OBJ) $(RUN_CLIENT) $(RUN_SERVER) *.d