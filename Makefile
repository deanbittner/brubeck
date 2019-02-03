PLATFORM=$(shell uname -s)
ifeq ($(PLATFORM),Darwin)
       include Makefile.mac
else
	. /etc/os-release
	case $ID in
	   alpine)
		include Makefile.alpine
		;;	
	   *)
		include Makefile.linux
		;;
	esac
endif
