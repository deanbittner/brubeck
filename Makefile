
PLATFORM=$(shell uname -s)
ifeq ($(PLATFORM),Darwin)
       include Makefile.mac
else
	include /etc/os-release
	ifeq ($(ID),alpine)
		include Makefile.alpine
	else
		include Makefile.linux
	endif
endif

include Makefile.common
include Makefile.distro
