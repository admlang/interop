# genhdr.mk: generates MicroPython's headers (qstrs, module table, root pointers, version)
# from mpconfigport.h and qstrdefsport.h, with the release's own embed port.
#   make -f genhdr.mk MICROPYTHON_TOP=<micropython-1.29.0> BUILD=<scratch>/build
# Run it in a scratch folder that holds copies of mpconfigport.h and mphalport.h; the headers
# land in $(BUILD)/genhdr, which amalgam.py reads.
PORT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
QSTR_DEFS = $(PORT)qstrdefsport.h
CFLAGS += -I$(PORT)
SRC_QSTR += extmod/modjson.c extmod/modre.c extmod/modheapq.c extmod/modbinascii.c extmod/modrandom.c
include $(MICROPYTHON_TOP)/ports/embed/embed.mk
