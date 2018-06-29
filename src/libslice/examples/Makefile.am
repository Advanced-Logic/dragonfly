AM_CFLAGS = -DM_GENERIC_INT32 -m64 -fPIC -Og -Wall -gdwarf-2 -I../ 
AM_LDFLAGS =  

bin_PROGRAMS= test_generic_mainloop

test_generic_mainloop_SOURCES= test_generic_mainloop.c 
test_generic_mainloop_LDADD= ../libslice.a -lssl -lcrypto
