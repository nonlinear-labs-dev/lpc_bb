APP = lpc-bb-test
PKGS = 
INSTALLFILE = $(APP)

ifeq ($(CROSS),YES)
ARCH=arm
TARGET_CXX_FLAGS=-O2 -funwind-tables

CROSS_STAGING_DIR = $(CROSS_PREFIX)/staging
CROSS_HOST_DIR    = $(CROSS_PREFIX)/host
CROSS_INSTALL_DIR = $(CROSS_PREFIX)/target

CC =  $(CROSS_HOST_DIR)/usr/bin/$(ARCH)-linux-gcc
LD =  $(CROSS_HOST_DIR)/usr/bin/$(ARCH)-linux-gcc

PKG_CONFIG             = $(CROSS_HOST_DIR)/usr/bin/pkg-config
PKG_CONFIG_SYSROOT_DIR = $(CROSS_STAGING_DIR)
PKG_CONFIG_PATH        = $(CROSS_STAGING_DIR)/usr/lib/pkgconfig:$(CROSS_STAGING_DIR)/usr/share/pkgconfig:$(CROSS_INSTALL_DIR)/usr/lib/pkgconfig
PKG_CONFIG_CALL        = PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG_SYSROOT_DIR=$(PKG_CONFIG_SYSROOT_DIR) $(PKG_CONFIG)

EXTRA_CC_FLAGS = \
	-I$(CROSS_STAGING_DIR)/usr/include \
	--sysroot=$(CROSS_STAGING_DIR) \
	-isysroot $(CROSS_STAGING_DIR)

INSTALLDIR = $(CROSS_INSTALL_DIR)/nonlinear/$(APP)

EXTRA_LDFLAGS = \
	-L$(CROSS_STAGING_DIR)/lib \
	-L$(CROSS_STAGING_DIR)/usr/lib \
	--sysroot=$(CROSS_STAGING_DIR)

else

INSTALLDIR = .
TARGET_CXX_FLAGS=-O0 -g

CC = gcc
LD = gcc
PKG_CONFIG_PATH_FIX=$(PKG_CONFIG_PATH):/usr/lib/pkgconfig:/usr/local/lib/pkgconfig
PKG_CONFIG_CALL = PKG_CONFIG_PATH=$(PKG_CONFIG_PATH_FIX) pkg-config

endif

OBJDIR = obj

CPPFILES = $(shell echo `find ./src -name '*.cpp'`)
CPPOBJS_INTERMEDIATE_2 = $(addprefix $(OBJDIR), $(CPPFILES))
CPPOBJS_INTERMEDIATE_3 = $(subst $(OBJDIR)., $(OBJDIR), $(CPPOBJS_INTERMEDIATE_2))
CPPOBJS_INTERMEDIATE_4 = $(CPPOBJS_INTERMEDIATE_3:.cpp=.o)
CPPOBJS += $(filter-out $(CPPFILE_EXCLUDES), $(CPPOBJS_INTERMEDIATE_4))

SRC_DIRS = $(shell echo `find src -type d`)
OBJ_DIRS_1 = $(addprefix /, $(SRC_DIRS))
OBJ_DIRS = $(addprefix $(OBJDIR), $(OBJ_DIRS_1))

ifeq ($(PKGS),)
PKG_CONFIG_CALL_CFLAGS = 
PKG_CONFIG_CALL_LIBS   = 
else
PKG_CONFIG_CALL_CFLAGS = `$(PKG_CONFIG_CALL) --cflags $(PKGS)`
PKG_CONFIG_CALL_LIBS   = `$(PKG_CONFIG_CALL) --libs $(PKGS)`
endif

CXXFLAGS = -Wall -fmessage-length=0 -I./src $(TARGET_CXX_FLAGS) 
CFLAGS = $(EXTRA_CC_FLAGS) $(PKG_CONFIG_CALL_CFLAGS) -Wall -D_REENTRANT -D_VERBOSE -D_POSIX $(TARGET_CXX_FLAGS)
LDFLAGS = $(EXTRA_LDFLAGS) $(PKG_CONFIG_CALL_LIBS)

all: MKDIR $(INSTALLDIR)/$(INSTALLFILE)

$(INSTALLDIR)/$(INSTALLFILE): $(OBJDIR)/$(APP)
	mkdir -p $(INSTALLDIR)
	cp -av $(OBJDIR)/$(APP) $(INSTALLDIR)

COMPILE_CPP = @echo "compiling $@ with $(CC):";\
	$(CC) -std=c++0x -pthread -c -MD -o $@ $< $(CXXFLAGS) $(CFLAGS);

$(OBJDIR)/$(APP): $(CPPOBJS)
	@echo "linking $@";\
	$(CC) $(CPPOBJS) $(LIBS) -lstdc++ -lpthread -lm $(LDFLAGS) -o $@ 

$(OBJDIR)/%.o: %.cpp
	$(COMPILE_CPP)
	
%.h::
	@echo "dependency missing => $@";


MKDIR:
	@echo "Making directories..."
	mkdir -p $(OBJ_DIRS)
	mkdir -p $(INSTALLDIR)
	@echo "Made directories."

clean:
	rm -f $(CPPOBJS)
	rm -rf $(OBJDIR)

install: MKDIR $(INSTALLDIR)/$(INSTALLFILE)

-include $(CPPOBJS:.o=.d)
