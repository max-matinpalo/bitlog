PREFIX = /usr/local

install:
	mkdir -p $(PREFIX)/include $(PREFIX)/share/man/man3
	cp bitlog.h $(PREFIX)/include/
	cp bitlog.3 $(PREFIX)/share/man/man3/

uninstall:
	rm -f $(PREFIX)/include/bitlog.h
	rm -f $(PREFIX)/share/man/man3/bitlog.3