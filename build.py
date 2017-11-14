#!/usr/bin/python
import sys
import os
import ftplib
import glob

CC = "arm-none-eabi-gcc"
CP = "arm-none-eabi-g++"
OC = "arm-none-eabi-objcopy" 
LD = "arm-none-eabi-ld"

def allFile(pattern):
    s = "";
    for file in glob.glob(pattern):
        s += file + " ";
    return s;

def run(cmd):
	os.system(cmd)

cwd = os.getcwd() 

run(CC + " -w -O3 -s  -g -I include -I include/jpeg -I/c/devkitPro/portlibs/armv6k/include " + allFile('source/dsp/*.c') + " -c  -march=armv6 -mlittle-endian   -fomit-frame-pointer -ffast-math -march=armv6k -mtune=mpcore -mfloat-abi=hard ")
run(CC + " -w -Os -s  -g -I include -I include/jpeg -I/c/devkitPro/portlibs/armv6k/include " + allFile('source/ns/*.c') + allFile('source/*.c') + allFile('source/libctru/*.c') + " -c  -march=armv6 -mlittle-endian   -fomit-frame-pointer -ffast-math -march=armv6k -mtune=mpcore -mfloat-abi=hard ")
run(CC + "  -w -Os " +  allFile('source/ns/*.s')  + allFile('source/*.s') + allFile('source/libctru/*.s') + " -c -s -march=armv6 -mlittle-endian   -fomit-frame-pointer -ffast-math -march=armv6k -mtune=mpcore -mfloat-abi=hard ")

run(LD + " -L lib -g -A armv6k -pie --print-gc-sections  -T 3ds.ld " + allFile("*.o") + " -lc -lm -lgcc --nostdlib"  )

run(OC + " -O binary a.out ntr.n3ds.bin -S")

run("rm *.o")
run('mv ntr.n3ds.bin  F:/ntr.n3ds.bin')