COMPONENT_ADD_INCLUDEDIRS += . \
							 pikascript/pikascript-core \
							 pikascript/pikascript-api \

COMPONENT_SRCS := 

CFLAGS += -DPIKASCRIPT -DPIKA_CONFIG_ENABLE -DCONFIG_SYS_VFS_ENABLE=1 \
          -DLFS_YES_TRACE

COMPONENT_OBJS := $(patsubst %.c,%.o, $(COMPONENT_SRCS))

COMPONENT_SRCDIRS := . \
					 pikascript/pikascript-core \
					 pikascript/pikascript-api \
					 pikascript/pikascript-lib/pika_lvgl \
					 pikascript/pikascript-lib/PikaStdLib \
					 pikascript/pikascript-lib/PikaStdDevice \
					 pikascript/pikascript-lib/BL808
