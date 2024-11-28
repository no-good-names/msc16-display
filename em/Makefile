CFLAGS := -MMD -std=gnu99
CXXFLAGS := $(CFLAGS) -std=c++23 -g -fPIC
LDFLAGS := -lm -lglm -lglfw -shared

INCLUDE_DIRS := -Iinclude

ifeq ($(shell uname),Darwin)
	INCLUDE_DIRS += -I/opt/homebrew/include
	LDFLAGS += -L/opt/homebrew/lib -framework Cocoa -framework IOKit -framework OpenGL
else
	LDFLAGS += -lOpenGL
endif

CFLAGS += $(INCLUDE_DIRS)
CXXFLAGS += $(INCLUDE_DIRS)

CSRC := $(shell find . -type f -name '*.c')
CXXSRC := $(shell find . -type f -name '*.cc')
OBJ := $(CXXSRC:.cc=.o) $(CSRC:.c=.o)
DEP := $(OBJ:.o=.d)

TARGET := libmsc16.so

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "  LD     $@"
	@$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cc
	@echo "  CXX    $@"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

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
