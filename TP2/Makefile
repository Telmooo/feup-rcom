.POSIX:

# -- C Compiler
CC = gcc

# -- Directories
# - Shared
# Source files
SSDIR = ./shared/src
# Header files
SIDIR = ./shared/include
# Library files
#SLDIR = ./shared/lib
# Object files
SODIR = ./shared/obj

# - Client
# Source files
CSDIR = ./client/src
# Header files
CIDIR = ./client/include
# Library files
#CLDIR = ./client/lib
# Object files
CODIR = ./client/obj

# - Server
# Source files
SVSDIR = ./server/src
# Header files
SVIDIR = ./server/include
# Library files
#SVLDIR = ./server/lib
# Object files
SVODIR = ./server/obj

# -- Flags
# - Shared
SIFLAGS = -I$(SIDIR)
#SLFLAGS = -L$(SLDIR)
SCFLAGS = -g -Wall#-Werror
# - Client
CIFLAGS = -I$(CIDIR) $(SIFLAGS)
#CLFLAGS = -L$(CLDIR) $(SLFLAGS)
CCFLAGS = -g -Wall#-Werror
# - Server
SVIFLAGS = -I$(SVIDIR) $(SIFLAGS)
#SVLFLAGS = -L$(SVLDIR) $(SVLFLAGS)
SVCFLAGS = -g -Wall#-Werror

# -- Dependencies
# - Shared
SDEPS = $(SODIR)/utils.o $(SODIR)/ftp.o
# - Client
CDEPS = $(SDEPS)
CMAIN = main.o
CTARGET = download
# - Server
SVDEPS = $(SDEPS)
SVMAIN = main.o
SVTARGET = app_server

.PHONY: all client server

all: client server

client: $(CTARGET)

server: $(SVTARGET)

# -- Create Directories
.PHONY: makesharedfolders makeclientfolders makeserverfolders
# - Shared
makesharedfolders:
	mkdir -p $(SODIR)

# - Client
makeclientfolders:
	mkdir -p $(CODIR)

# - Server
makeserverfolders:
	mkdir -p $(SVODIR)

# -- Create Object Files
# - Shared
$(SODIR)/%.o: $(SSDIR)/%.c
	$(CC) $(SCFLAGS) -c $(SIFLAGS) $< -o $@

# - Client
$(CODIR)/%.o: $(CSDIR)/%.c
	$(CC) $(CCFLAGS) -c $(CIFLAGS) $< -o $@

# - Server
$(SVODIR)/%.o: $(SVSDIR)/%.c
	$(CC) $(SVCFLAGS) -c $(SVIFLAGS) $< -o $@

# -- Create Executables
# - Client
$(CTARGET): makesharedfolders makeclientfolders $(CODIR)/$(CMAIN) $(CDEPS)
	$(CC) $(CCFLAGS) -o $@ $(word 3, $^) $(CDEPS)
	@echo "Compiled client as "$@" successfully"

# - Server
$(SVTARGET): makesharedfolders makeserverfolders  $(SVODIR)/$(SVMAIN) $(SVDEPS)
	$(CC) $(SVCFLAGS) -o $@ $(word 3, $^) $(SVDEPS)
	@echo "Compiled server as "$@" successfully"

# -- Cleaning up
.PHONY: clean cleanshared cleanclient cleanserver
clean: cleanshared
	@echo "Cleaned everything!"

cleanshared: cleanclient cleanserver
	rm -rf $(SODIR)
	@echo "Cleaned shared object files "$(SODIR)

cleanclient:
	rm -rf $(CODIR)
	@echo "Cleaned client object files "$(CODIR)

cleanserver:
	rm -rf $(SVODIR)
	@echo "Cleaned server object files "$(SVODIR)


.PHONY: remove removeclient removeserver
remove: clean removeclient removeserver
	@echo "Cleared everything!"

removeclient:
	rm -f $(CTARGET)
	@echo "Cleared client executable file "$(CTARGET)

removeserver:
	rm -f $(SVTARGET)
	@echo "Cleared server executable file "$(SVTARGET)