CFLAGS := -MMD -std=gnu99 -O2 -g
LDFLAGS := -lm -lglm -lglfw 

INCLUDE_DIRS := -Iinclude

ifeq ($(shell uname),Darwin)
	INCLUDE_DIRS += -I/opt/homebrew/include
	LDFLAGS += -L/opt/homebrew/lib -framework Cocoa -framework IOKit -framework OpenGL
else
	LDFLAGS += -lOpenGL
endif

CFLAGS += $(INCLUDE_DIRS)

CSRC := $(shell find . -type f -name '*.c')
OBJ := $(CSRC:.c=.o)
DEP := $(OBJ:.o=.d)

TARGET := em

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "  LD     $@"
	@$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo "  CC     $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ) $(DEP)

.PHONY: run
run: $(TARGET)
	./$(TARGET)

-include $(DEP)
$(OBJ): Makefile

print-%:
	@echo $* = $($*)

