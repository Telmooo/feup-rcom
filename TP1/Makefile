.POSIX:

TARGET = app

CC = gcc
CFLAGS = -g -Wall #-Werror

SRCDIR = ./src
ODIR = ./obj
IDIR = ./include

IFLAGS = -I $(IDIR)

SRCS = main.c link_layer.c serial_port.c signal_handling.c state_machine.c app_layer.c

SRC  := $(addprefix $(SRCDIR)/, $(SRCS))
OBJ  := $(addprefix $(ODIR)/, $(SRCS:%.c=%.o))

$(TARGET): $(OBJ)
	@$(CC) -o $@ $(OBJ) $(CFLAGS) $(IFLAGS)
	@echo "Linking complete!"

$(OBJ): $(ODIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully"

.PHONY: clean
clean:
	@rm -f $(OBJ)
	@echo "Cleaned!"

.PHONY: remove
remove: clean
	@rm -f $(TARGET)
	@echo "Everything was cleared"