
install.medpost :
	perl util/install.perl $(OSTYPE)

uninstall :
	perl uninstall.perl

distribute :
	perl uninstall.perl
	tar cf medpost.tar `cat file.list`
	gzip -f medpost.tar
