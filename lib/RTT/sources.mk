THIS_PATH := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

C_INCLUDES +=                                                          \
	-I$(THIS_PATH)/inc                                                 \

SUBM_SOURCES += $(shell find $(THIS_PATH) -name "*.c")

undefine THIS_PATH
