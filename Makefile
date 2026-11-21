PTRACE_DIR ?= ../lib-ptrace

ifneq ($(MAKECMDGOALS),kernel)
    ifdef PS4_PAYLOAD_SDK
        include $(PS4_PAYLOAD_SDK)/toolchain/orbis.mk
    else
        $(error PS4_PAYLOAD_SDK is undefined)
    endif

    ifeq ($(wildcard $(PTRACE_DIR)/headers/ptrace.h),)
        $(error $(PTRACE_DIR)/headers/ptrace.h not found; set PTRACE_DIR to a lib-ptrace checkout)
    endif
endif

OBJCOPY ?= objcopy
.DEFAULT_GOAL := all
BUILD ?= build
OBJ := $(BUILD)/obj/kstuff
LIB := $(BUILD)/lib/libkstuff.a
KERNEL_ELF := $(BUILD)/bin/kstuff-kernel.elf
KERNEL_BIN := $(BUILD)/bin/kstuff-kernel.bin
ELF := $(BUILD)/bin/kstuff.elf
BIN := $(BUILD)/bin/kstuff.bin
PTRACE_LIB := $(PTRACE_DIR)/build/ps4/libptrace.a
LOADER_ELF := $(OBJ)/payload/bootstrap.elf
GEN := $(BUILD)/generated/kstuff_kernel.h
INCFLAGS := -Iheaders -Isource/core -Isource/generated -I$(PTRACE_DIR)/headers
BASEFLAGS := -fPIC -Oz $(if $(DEBUG),-g) -Wall -Werror -mstackrealign -ffunction-sections -fdata-sections $(INCFLAGS) -MMD -MP
CFLAGS := $(BASEFLAGS) -std=gnu11
CXXFLAGS := $(BASEFLAGS) -std=c++2b -fno-exceptions -fno-rtti -fno-threadsafe-statics
SOURCES := $(wildcard source/*/*.c source/*/*.cpp)
OBJECTS := $(SOURCES:%=$(OBJ)/%.o)
KERNELOBJ := $(patsubst kernel/%.c,$(OBJ)/kernel/%.o,$(wildcard kernel/*.c))
KERNEL_CC ?= gcc
KERNEL_CFLAGS := -Os -std=gnu11 -fpie -fno-builtin -fno-stack-protector -fcf-protection=none -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-omit-frame-pointer -mno-red-zone -mno-sse -mno-mmx -msoft-float -masm=intel -march=btver2 -m64 -Wall -Werror -Wno-frame-address -Ikernel -Iheaders -Ivendor/ps4-hen -MMD -MP
.PHONY: all lib elf bin kernel clean master
all: lib elf bin
lib: $(LIB)
elf: $(ELF)
bin: $(BIN)
kernel: $(KERNEL_BIN)
$(OBJ)/kernel/%.o: kernel/%.c Makefile
	@mkdir -p $(dir $@)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -c $< -o $@
$(KERNEL_ELF): $(KERNELOBJ) kernel/linker.x
	@mkdir -p $(dir $@)
	$(KERNEL_CC) -nostdlib -nostartfiles -no-pie -Wl,--build-id=none,-T,kernel/linker.x $(KERNELOBJ) -o $@
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary --only-section=.payload $< $@
$(GEN): $(KERNEL_BIN)
	@mkdir -p $(dir $@)
	xxd -i -n kstuff_kernel $< > $@
$(OBJ)/source/core/kernel_payload.cpp.o: $(GEN)
$(OBJ)/source/core/kernel_exec.c.o: CFLAGS += -mno-red-zone -mno-sse -mno-mmx -fno-stack-protector -fno-builtin
$(OBJ)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(BUILD)/generated -c $< -o $@
$(LIB): $(OBJECTS)
	@mkdir -p $(dir $@)
	rm -f $@
	$(AR) rcs $@ $^
$(PTRACE_LIB):
	$(MAKE) -C $(PTRACE_DIR) PLATFORM=ps4 PS4_PAYLOAD_SDK=$(PS4_PAYLOAD_SDK) all
$(OBJ)/payload/runner.c.o: payload/runner.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
$(ELF): $(OBJ)/payload/runner.c.o $(LIB) $(PTRACE_LIB)
	@mkdir -p $(dir $@)
	$(CXX) $< $(LIB) $(PTRACE_LIB) -o $@
$(OBJ)/payload/bootstrap.c.o: payload/bootstrap.c $(ELF)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -ffreestanding -fno-builtin -fno-stack-protector -DKSTUFF_ELF='"$(abspath $(ELF))"' -c $< -o $@
$(LOADER_ELF): $(OBJ)/payload/bootstrap.c.o payload/bootstrap.x
	$(LD) -T payload/bootstrap.x -o $@ $<
$(BIN): $(LOADER_ELF)
	@mkdir -p $(dir $@)
	$(OBJCOPY) -O binary --only-section=.text $< $@
-include $(OBJECTS:.o=.d) $(KERNELOBJ:.o=.d)
clean:
	rm -rf $(OBJ) $(LIB) $(ELF) $(BIN) $(KERNEL_ELF) $(KERNEL_BIN) $(GEN)
master:
	@$(MAKE) clean
	@$(MAKE) all
