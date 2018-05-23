all: 	axfui
axfui: 	main.c midimsg_t.c
	gcc -Wall -O -o axfui main.c
	strip axfui
