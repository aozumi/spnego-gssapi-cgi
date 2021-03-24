CFLAGS += ${KRB5_CFLAGS}
LIBS += ${KRB5_LIBS}

KRB5_CFLAGS != krb5-config --cflags gssapi krb5
KRB5_LIBS != krb5-config --libs gssapi krb5

CLEAN_FILES += spnegocgi *.o
ALL_CLEAN_FILES += .depend

# CGI executable
spnegocgi: main.o base64.o log.o gssutils.o
	${CC} -o $@ $> ${LIBS}

.c.o:
	${CC} -c -o $@ ${CFLAGS} $<

depend: .depend
.depend: *.c *.h
	mkdep ${CFLAGS} *.c

clean: .PHONY
	-rm -f ${CLEAN_FILES}

allclean: clean
	-rm -f ${ALL_CLEAN_FILES}

.include "docker.mk"
