TARGET = SaibaUltimate
OBJS = src/main.o

PSPSDK := $(shell psp-config --pspsdk-path)
INCDIR =
CFLAGS = -O2 -G0 -Wall -Wextra
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PSP_FW_VERSION = 660
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Saiba Ultimate: Guerreiro do Multiverso

LIBS = -lpspgu -lpspgum -lpspaudio -lpspdisplay -lpspctrl -lpspkernel -lm

include $(PSPSDK)/lib/build.mak
