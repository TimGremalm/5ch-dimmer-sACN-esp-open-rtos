# make
# make flash
# make test
# make test ESPPORT=/dev/ttyUSB0
PROGRAM=sACN_dimmer
EXTRA_COMPONENTS = extras/multipwm
include $(ESPOPENRTOS)/common.mk
