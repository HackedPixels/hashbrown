BINNAME := hashbrown
CFLAGS := -g -fdiagnostics-color=always
CC := gcc

# Dont clean up when building these
NODEPS := clean nuke
# Find all the source files
SOURCES := $(shell find src/ -name "*.c")
# The Dependency files
DEPFILES := $(patsubst src/%.c,deps/%.d,$(SOURCES))

.DEFAULT_GOAL := $(BINNAME)
.PHONY := clean

# Dont build dependencies when cleaning up
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
	-include $(DEPFILES)
endif

#RULE: main target
$(BINNAME): $(patsubst src/%.c,obj/%.o,$(SOURCES))
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

#RULE: build dependency files
deps/%.d: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MM -MT '$(patsubst src/%.c,obj/%.o,$<)' $< -MF $@

#RULE: this does the compiling
obj/%.o: src/%.c deps/%.d src/%.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ 

#RULE: clean all the files
clean:
	rm -f $(BINNAME)
	rm -f $(patsubst src/%.c,obj/%.o,$(SOURCES))

#RULE: clean all the files & delete folders
nuke: clean
	rm -f -r deps obj
