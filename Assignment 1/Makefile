# Assignment-level build and process launcher.
#
# Launch commands run in the foreground and record their PID in .run/ while
# active.  They must not be backgrounded: a non-interactive make shell assigns
# /dev/null to a background job's stdin, which prevents ncurses from reading
# keystrokes. Start each service in a separate terminal.

SHELL := /bin/sh

RUN_DIR := .run

.DEFAULT_GOAL := help

.PHONY: help all build clean stop-all \
	phase1 phase2 phase3 phase4 phase5 \
	phase1-build phase2-build phase3-build phase4-build phase5-build \
	stop-phase1 stop-phase2 stop-phase3 stop-phase4 stop-phase5

help:
	@printf '%s\n' \
	  'Build: make phase1-build  (or phase2-build ... phase5-build; make build builds all)' \
	  'Run:   make phase3-server; make phase3-client1; make phase3-client2' \
	  '       Run each server, proxy, and client in its own terminal.' \
	  '       (Starting a service will automatically stop any existing instance of it)' \
	  'Stop:  make stop-phase3-server' \
	  '       make stop-phase3-client1' \
	  '       make stop-phase3   # stops all Phase 3 processes' \
	  '       make stop-all      # stops all processes launched by this Makefile'

all build: phase1-build phase2-build phase3-build phase4-build phase5-build
phase1: phase1-build
	@printf '%s\n' 'Phase 1 is built. Start it with: make phase1-server  (then make phase1-client1 in another terminal)'
phase2: phase2-build
	@printf '%s\n' 'Phase 2 is built. Start it with: make phase2-server  (then make phase2-client1 in another terminal)'
phase3: phase3-build
	@printf '%s\n' 'Phase 3 is built. Start it with: make phase3-server  (then make phase3-client1 in another terminal)'
phase4: phase4-build
	@printf '%s\n' 'Phase 4 is built. Start it with: make phase4-server  (then make phase4-client1 in another terminal)'
phase5: phase5-build
	@printf '%s\n' 'Phase 5 is built. Start it with: make phase5-server  (then make phase5-client1 in another terminal)'

# Convenience aliases for the first client in each phase.
phase1-client: phase1-client1
phase2-client: phase2-client1
phase3-client: phase3-client1
phase4-client: phase4-client1
phase5-client: phase5-client1

phase1-build:
	$(MAKE) -C Phase1 all
phase2-build:
	$(MAKE) -C Phase2 all
phase3-build:
	$(MAKE) -C Phase3 all
phase4-build:
	$(MAKE) -C Phase4 all
phase5-build:
	$(MAKE) -C Phase5 all

# Tracking variables to build the phase-level stop commands
PHASE1_STOPS :=
PHASE2_STOPS :=
PHASE3_STOPS :=
PHASE4_STOPS :=
PHASE5_STOPS :=

# $(1) = phase number, $(2) = service name, $(3) = executable and arguments,
# $(4) = listening port (blank for clients).
define RUN_SERVICE
PHASE$(1)_STOPS += stop-phase$(1)-$(2)

phase$(1)-$(2): stop-phase$(1)-$(2) phase$(1)-build
	@mkdir -p "$(RUN_DIR)/phase$(1)"
	@pidfile="$(RUN_DIR)/phase$(1)/$(2).pid"; \
	echo "$$$$$$$$" > "$$$$pidfile"; \
	echo "Started phase $(1) $(2) (PID $$$$$$$$). Press Ctrl-C or run make stop-phase$(1)-$(2)."; \
	cd "Phase$(1)" && exec $(3)

stop-phase$(1)-$(2):
	@pidfile="$(RUN_DIR)/phase$(1)/$(2).pid"; \
	if test -f "$$$$pidfile"; then \
		pid=$$$$(cat "$$$$pidfile"); \
		if kill -0 "$$$$pid" 2>/dev/null; then kill "$$$$pid" && echo "Stopped phase $(1) $(2) (PID $$$$pid)."; fi; \
	else \
		listener=$$$$(if test -n "$(4)"; then lsof -tiTCP:$(4) -sTCP:LISTEN 2>/dev/null | head -n 1; fi); \
		if test -n "$$$$listener"; then \
			kill "$$$$listener" && echo "Stopped the listener on port $(4) (PID $$$$listener)."; \
		fi; \
	fi; \
	rm -f "$$$$pidfile"
.PHONY: phase$(1)-$(2) stop-phase$(1)-$(2)
endef

# Phase 1 Setup
$(eval $(call RUN_SERVICE,1,server,build/run_server 12345,12345))
$(eval $(call RUN_SERVICE,1,client1,build/run_client 127.0.0.1 12345,))
$(eval $(call RUN_SERVICE,1,client2,build/run_client 127.0.0.1 12345,))

# Phase 2 Setup
$(eval $(call RUN_SERVICE,2,server,build/run_server 12345,12345))
$(eval $(call RUN_SERVICE,2,client1,build/run_client 127.0.0.1 12345,))
$(eval $(call RUN_SERVICE,2,client2,build/run_client 127.0.0.1 12345,))
$(eval $(call RUN_SERVICE,2,proxy,build/run_proxy 12346 127.0.0.1 12345,12346))
$(eval $(call RUN_SERVICE,2,victim-client1,build/run_client 127.0.0.1 12346,))
$(eval $(call RUN_SERVICE,2,victim-client2,build/run_client 127.0.0.1 12346,))

# Phase 3 Setup
$(eval $(call RUN_SERVICE,3,server,build/run_server 12345 build/certs,12345))
$(eval $(call RUN_SERVICE,3,client1,build/run_client 127.0.0.1 12345 build/certs,))
$(eval $(call RUN_SERVICE,3,client2,build/run_client 127.0.0.1 12345 build/certs,))
$(eval $(call RUN_SERVICE,3,proxy,build/run_proxy normal 12346 127.0.0.1 12345,12346))
$(eval $(call RUN_SERVICE,3,proxy-fake,build/run_proxy fake 12347 127.0.0.1 12345 build/proxy-certs,12347))
$(eval $(call RUN_SERVICE,3,proxy-thief,build/run_proxy thief 12348 127.0.0.1 12345 build/certs,12348))
$(eval $(call RUN_SERVICE,3,victim-client1,build/run_client 127.0.0.1 12346 build/certs,))
$(eval $(call RUN_SERVICE,3,victim-client2,build/run_client 127.0.0.1 12346 build/certs,))
$(eval $(call RUN_SERVICE,3,victim-client1-fake,build/run_client 127.0.0.1 12347 build/certs,))
$(eval $(call RUN_SERVICE,3,victim-client2-fake,build/run_client 127.0.0.1 12347 build/certs,))
$(eval $(call RUN_SERVICE,3,victim-client1-thief,build/run_client 127.0.0.1 12348 build/certs,))
$(eval $(call RUN_SERVICE,3,victim-client2-thief,build/run_client 127.0.0.1 12348 build/certs,))

# Phase 4 Setup
$(eval $(call RUN_SERVICE,4,server,build/run_server 12345 build/certs,12345))
$(eval $(call RUN_SERVICE,4,client1,build/run_client 127.0.0.1 12345 build/certs,))
$(eval $(call RUN_SERVICE,4,client2,build/run_client 127.0.0.1 12345 build/certs,))
$(eval $(call RUN_SERVICE,4,proxy,build/run_proxy normal 12346 127.0.0.1 12345,12346))
$(eval $(call RUN_SERVICE,4,proxy-fake,build/run_proxy fake 12347 127.0.0.1 12345 build/proxy-certs,12347))
$(eval $(call RUN_SERVICE,4,proxy-thief,build/run_proxy thief 12348 127.0.0.1 12345 build/certs,12348))
$(eval $(call RUN_SERVICE,4,victim-client1,build/run_client 127.0.0.1 12346 build/certs,))
$(eval $(call RUN_SERVICE,4,victim-client2,build/run_client 127.0.0.1 12346 build/certs,))
$(eval $(call RUN_SERVICE,4,victim-client1-fake,build/run_client 127.0.0.1 12347 build/certs,))
$(eval $(call RUN_SERVICE,4,victim-client2-fake,build/run_client 127.0.0.1 12347 build/certs,))
$(eval $(call RUN_SERVICE,4,victim-client1-thief,build/run_client 127.0.0.1 12348 build/certs,))
$(eval $(call RUN_SERVICE,4,victim-client2-thief,build/run_client 127.0.0.1 12348 build/certs,))

# Phase 5 Setup
$(eval $(call RUN_SERVICE,5,server,build/run_server 12345 build/certs,12345))
$(eval $(call RUN_SERVICE,5,client1,build/run_client 127.0.0.1 12345 build/certs,))
$(eval $(call RUN_SERVICE,5,client2,build/run_client 127.0.0.1 12345 build/certs,))
$(eval $(call RUN_SERVICE,5,proxy,build/run_proxy normal 12346 127.0.0.1 12345,12346))
$(eval $(call RUN_SERVICE,5,proxy-fake,build/run_proxy fake 12347 127.0.0.1 12345 build/proxy-certs,12347))
$(eval $(call RUN_SERVICE,5,proxy-thief,build/run_proxy thief 12348 127.0.0.1 12345 build/certs,12348))
$(eval $(call RUN_SERVICE,5,victim-client1,build/run_client 127.0.0.1 12346 build/certs,))
$(eval $(call RUN_SERVICE,5,victim-client2,build/run_client 127.0.0.1 12346 build/certs,))
$(eval $(call RUN_SERVICE,5,victim-client1-fake,build/run_client 127.0.0.1 12347 build/certs,))
$(eval $(call RUN_SERVICE,5,victim-client2-fake,build/run_client 127.0.0.1 12347 build/certs,))
$(eval $(call RUN_SERVICE,5,victim-client1-thief,build/run_client 127.0.0.1 12348 build/certs,))
$(eval $(call RUN_SERVICE,5,victim-client2-thief,build/run_client 127.0.0.1 12348 build/certs,))

# Group Stop Targets
stop-phase1: $(PHASE1_STOPS)
stop-phase2: $(PHASE2_STOPS)
stop-phase3: $(PHASE3_STOPS)
stop-phase4: $(PHASE4_STOPS)
stop-phase5: $(PHASE5_STOPS)

stop-all: stop-phase1 stop-phase2 stop-phase3 stop-phase4 stop-phase5

clean:
	@for phase in Phase1 Phase2 Phase3 Phase4 Phase5; do $(MAKE) -C "$$phase" clean; done
