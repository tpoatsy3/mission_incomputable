# Makefile (starter) for Mission Incomputable

MAKE = make

# These targets do not produce real "targets"
.PHONY: all clean start-proxy stop-proxy

############## default: make all libs and programs ##########

all: 
	@echo "Making everything..."
	$(MAKE) -C field_agent
	$(MAKE) -C guide_agent
	$(MAKE) -C game_server

########################### proxy ###########################

# Start the proxy running - add flags as desired. Options: -all, -fa2gs, -gs2fa
start-proxy: proxy/proxy
	@cd proxy; node proxy

# Kill any running instances of the proxy.
stop-proxy:
	@pkill -f 'node proxy'

# Use npm to install all proxy dependencies (only need to do this initially).
install-proxy-deps:
	@cd proxy; npm install

########################### proxy ###########################

clean:
	@echo "Cleaning everything..."
	$(MAKE) -C field_agent clean
	$(MAKE) -C guide_agent clean
	$(MAKE) -C game_server clean
	rm -f *~ *.o
