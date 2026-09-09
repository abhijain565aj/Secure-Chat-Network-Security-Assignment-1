# Assignment-level build and process launcher.
#
# Launch commands run in the foreground, but record their PID in .run/ while
# active.  Start each one in a separate terminal, then use the matching stop
# target from any terminal.

SHELL := /bin/sh

RUN_DIR := .run
INSTANCE ?= default

.DEFAULT_GOAL := help

.PHONY: help all build \
	phase1 phase2 phase3 phase4 phase5 \
	phase1-build phase2-build phase3-build phase4-build phase5-build \
	phase1-server phase1-client \
	phase2-server phase2-client phase2-proxy phase2-victim-client \
	phase3-server phase3-client phase3-proxy phase3-proxy-fake phase3-proxy-thief phase3-victim-client phase3-victim-client-fake phase3-victim-client-thief \
	phase4-server phase4-client phase4-proxy phase4-proxy-fake phase4-proxy-thief phase4-victim-client phase4-victim-client-fake phase4-victim-client-thief \
	phase5-server phase5-client phase5-proxy phase5-proxy-fake phase5-proxy-thief phase5-victim-client phase5-victim-client-fake phase5-victim-client-thief \
	stop stop-all clean

help:
	@printf '%s\n' \
	  'Build: make phase1-build  (or phase2-build ... phase5-build; make build builds all)' \
	  'Run:   make phase3-server; make phase3-client INSTANCE=alice' \
	  '       Run each server, proxy, and client in its own terminal.' \
	  'Stop:  make stop-phase3-server' \
	  '       make stop-phase3-client INSTANCE=alice' \
	  '       make stop PHASE=3 SERVICE=server [INSTANCE=default]' \
	  '       make stop-all   # stops only processes launched by this Makefile'

all build: phase1-build phase2-build phase3-build phase4-build phase5-build
phase1: phase1-build
	@printf '%s\n' 'Phase 1 is built. Start it with: make phase1-server  (then make phase1-client INSTANCE=<name> in another terminal)'
phase2: phase2-build
	@printf '%s\n' 'Phase 2 is built. Start it with: make phase2-server  (then make phase2-client INSTANCE=<name> in another terminal)'
phase3: phase3-build
	@printf '%s\n' 'Phase 3 is built. Start it with: make phase3-server  (then make phase3-client INSTANCE=<name> in another terminal)'
phase4: phase4-build
	@printf '%s\n' 'Phase 4 is built. Start it with: make phase4-server  (then make phase4-client INSTANCE=<name> in another terminal)'
phase5: phase5-build
	@printf '%s\n' 'Phase 5 is built. Start it with: make phase5-server  (then make phase5-client INSTANCE=<name> in another terminal)'

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

# $(1) = phase number, $(2) = service name, $(3) = executable and arguments,
# $(4) = listening port (blank for clients).
define RUN_SERVICE
phase$(1)-$(2): phase$(1)-build
	@mkdir -p "$(RUN_DIR)/phase$(1)"
	@pidfile="$(RUN_DIR)/phase$(1)/$(2)$$$$(echo "$(2)" | grep -q client && printf '.%s' "$(INSTANCE)").pid"; \
	if test -f "$$$$pidfile" && kill -0 "$$$$(cat "$$$$pidfile")" 2>/dev/null; then \
		echo "$(2) is already running (PID $$$$(cat "$$$$pidfile"))"; exit 1; \
	fi; \
	rm -f "$$$$pidfile"; \
	trap 'rm -f "$$$$pidfile"' EXIT INT TERM; \
	( cd "Phase$(1)" && exec $(3) ) & pid=$$$$!; \
	echo "$$$$pid" > "$$$$pidfile"; \
	echo "Started phase $(1) $(2) (PID $$$$pid). Press Ctrl-C or run make stop-phase$(1)-$(2)$$$$(echo "$(2)" | grep -q client && printf ' INSTANCE=%s' "$(INSTANCE)")."; \
	wait "$$$$pid"

stop-phase$(1)-$(2):
	@pidfile="$(RUN_DIR)/phase$(1)/$(2)$$$$(echo "$(2)" | grep -q client && printf '.%s' "$(INSTANCE)").pid"; \
	if test ! -f "$$$$pidfile"; then \
		listener=$$$$(if test -n "$(4)"; then lsof -tiTCP:$(4) -sTCP:LISTEN 2>/dev/null | head -n 1; fi); \
		if test -n "$$$$listener"; then \
			kill "$$$$listener" && echo "Stopped the listener on port $(4) (PID $$$$listener)."; \
		else echo "No recorded phase $(1) $(2) process."; fi; \
		exit 0; \
	fi; \
	pid=$$$$(cat "$$$$pidfile"); \
	if kill -0 "$$$$pid" 2>/dev/null; then kill "$$$$pid" && echo "Stopped phase $(1) $(2) (PID $$$$pid)."; \
	else echo "Removed stale PID file for phase $(1) $(2)."; fi; \
	rm -f "$$$$pidfile"
.PHONY: phase$(1)-$(2) stop-phase$(1)-$(2)
endef

$(eval $(call RUN_SERVICE,1,server,build/run_server 12345,12345))
$(eval $(call RUN_SERVICE,1,client,build/run_client 127.0.0.1 12345))

$(eval $(call RUN_SERVICE,2,server,build/run_server 12345,12345))
$(eval $(call RUN_SERVICE,2,client,build/run_client 127.0.0.1 12345))
$(eval $(call RUN_SERVICE,2,proxy,build/run_proxy 12346 127.0.0.1 12345,12346))
$(eval $(call RUN_SERVICE,2,victim-client,build/run_client 127.0.0.1 12346))

$(eval $(call RUN_SERVICE,3,server,build/run_server 12345 build/certs,12345))
$(eval $(call RUN_SERVICE,3,client,build/run_client 127.0.0.1 12345 build/certs))
$(eval $(call RUN_SERVICE,3,proxy,build/run_proxy normal 12346 127.0.0.1 12345,12346))
$(eval $(call RUN_SERVICE,3,proxy-fake,build/run_proxy fake 12347 127.0.0.1 12345 build/proxy-certs,12347))
$(eval $(call RUN_SERVICE,3,proxy-thief,build/run_proxy thief 12348 127.0.0.1 12345 build/certs,12348))
$(eval $(call RUN_SERVICE,3,victim-client,build/run_client 127.0.0.1 12346 build/certs))
$(eval $(call RUN_SERVICE,3,victim-client-fake,build/run_client 127.0.0.1 12347 build/certs))
$(eval $(call RUN_SERVICE,3,victim-client-thief,build/run_client 127.0.0.1 12348 build/certs))

$(eval $(call RUN_SERVICE,4,server,build/run_server 12345 build/certs,12345))
$(eval $(call RUN_SERVICE,4,client,build/run_client 127.0.0.1 12345 build/certs))
$(eval $(call RUN_SERVICE,4,proxy,build/run_proxy normal 12346 127.0.0.1 12345,12346))
$(eval $(call RUN_SERVICE,4,proxy-fake,build/run_proxy fake 12347 127.0.0.1 12345 build/proxy-certs,12347))
$(eval $(call RUN_SERVICE,4,proxy-thief,build/run_proxy thief 12348 127.0.0.1 12345 build/certs,12348))
$(eval $(call RUN_SERVICE,4,victim-client,build/run_client 127.0.0.1 12346 build/certs))
$(eval $(call RUN_SERVICE,4,victim-client-fake,build/run_client 127.0.0.1 12347 build/certs))
$(eval $(call RUN_SERVICE,4,victim-client-thief,build/run_client 127.0.0.1 12348 build/certs))

$(eval $(call RUN_SERVICE,5,server,build/run_server 12345 build/certs,12345))
$(eval $(call RUN_SERVICE,5,client,build/run_client 127.0.0.1 12345 build/certs))
$(eval $(call RUN_SERVICE,5,proxy,build/run_proxy normal 12346 127.0.0.1 12345,12346))
$(eval $(call RUN_SERVICE,5,proxy-fake,build/run_proxy fake 12347 127.0.0.1 12345 build/proxy-certs,12347))
$(eval $(call RUN_SERVICE,5,proxy-thief,build/run_proxy thief 12348 127.0.0.1 12345 build/certs,12348))
$(eval $(call RUN_SERVICE,5,victim-client,build/run_client 127.0.0.1 12346 build/certs))
$(eval $(call RUN_SERVICE,5,victim-client-fake,build/run_client 127.0.0.1 12347 build/certs))
$(eval $(call RUN_SERVICE,5,victim-client-thief,build/run_client 127.0.0.1 12348 build/certs))

# Generic targeted stop. Example: make stop PHASE=3 SERVICE=proxy-fake
stop:
	@test -n "$(PHASE)" && test -n "$(SERVICE)" || { echo 'Usage: make stop PHASE=<1-5> SERVICE=<service> [INSTANCE=<name>]'; exit 2; }
	@$(MAKE) --no-print-directory stop-phase$(PHASE)-$(SERVICE) INSTANCE="$(INSTANCE)"

stop-all:
	@for pidfile in $(RUN_DIR)/phase*/*.pid; do \
		test -f "$$pidfile" || continue; pid=$$(cat "$$pidfile"); \
		if kill -0 "$$pid" 2>/dev/null; then kill "$$pid" && echo "Stopped PID $$pid."; fi; \
		rm -f "$$pidfile"; \
	done

clean:
	@for phase in Phase1 Phase2 Phase3 Phase4 Phase5; do $(MAKE) -C "$$phase" clean; done