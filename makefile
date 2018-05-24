all: 	axfui
axfui: 	main.cxx midimsg_t.c midiEngine_t.c app.cxx
	g++ -Wall -O -o afxui `fltk-config --cflags --ldflags` main.cxx
	strip afxui
clean:
	rm -f afxui
	rm -f *~
.PHONY:
	clean
