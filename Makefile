PLATFORM=$(shell uname -s)
ifeq ($(PLATFORM),Darwin)
       include Makefile.mac
else
       include Makefile.linux
endif
