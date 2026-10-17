TARGET = SaibaUltimate
OBJS = src/main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -Wextra
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PSP_FW_VERSION = 660
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Saiba Ultimate

LIBS = -lpspdebug -lpspdisplay -lpspctrl -lpspkernel

include $(PSPSDK)/lib/build.mak
