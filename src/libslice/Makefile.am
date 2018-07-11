AM_CFLAGS= -DM_GENERIC_INT32 -m64 -fPIC -Og -Wall -gdwarf-2 
AM_LDFLAGS= `pkg-config --libs openssl` 

noinst_LIBRARIES= libslice.a

libslice_a_SOURCES= slice-buffer.c slice-client.c slice-connection.c slice-io.c slice-mainloop.c slice-object.c slice-server.c slice-session.c slice-ssl.c slice-ssl-client.c slice-ssl-server.c 
