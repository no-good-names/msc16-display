ASMBLR = ./asm/
EMULTR = ./em/
DISPLAY = ./display/

all: em asm

em:
	$(MAKE) -C $(EMULTR)

asm:
	$(MAKE) -C $(ASMBLR)

build_display: all
	$(MAKE) -C $(DISPLAY)

display_test: build_display
	$(MAKE) -C $(EMULTR) TEST=../display/display_test.s run

run_em: all
	$(MAKE) -C $(EMULTR) run

run_asm: all
	$(MAKE) -C $(ASMBLR) run

.PHONY: clean
clean:
	$(MAKE) -C $(EMULTR) clean
	$(MAKE) -C $(ASMBLR) clean
	$(MAKE) -C $(DISPLAY) clean
