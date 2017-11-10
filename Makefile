rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OC := arm-none-eabi-objcopy

name := ntr.n3ds.bin

dir_source := source
dir_build := build
dir_include := include

objects = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
			  $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
			  $(call rwildcard, $(dir_source), *.s *.c)))
              
ASFLAGS := -c -march=armv6 -mlittle-endian -fomit-frame-pointer -ffast-math -march=armv6k -mtune=mpcore -mfloat-abi=hard
CFLAGS := -O3 -s -g -I $(dir_include) -I $(dir_include)/jpeg -I/c/devkitPro/portlibs/armv6k/include $(ASFLAGS)
LDFLAGS := -L . -g -A armv6k -pie --print-gc-sections -T 3ds.ld -Map=test.map $(objects) -lc -lm -lgcc --nostdlib

.PHONY: all
all: $(dir_build)/main.bin

.PHONY: clean
clean: 
	rm -rf $(dir_build)
    
$(dir_build)/main.bin: $(dir_build)/main.elf
	$(OC) -O binary $< $@
    
$(dir_build)/main.elf: $(objects)
	$(LD) $(LDFLAGS) $^
    
$(dir_build)/%.o: $(call rwildcard, $(dir_source), *.c *s)
	@mkdir -p "$(@D)"
	$(CC) -Os -s  -g -I include -I include/jpeg -I/c/devkitPro/portlibs/armv6k/include $(OUTPUT_OPTION) $<