#
# Test spnegocgi using Docker
# ===========================

# Admin principal to fetch the keys of the service principal
_USER != whoami
USER ?= ${_USER}
ADMIN_PRINCIPAL ?= ${USER}/admin

# Service principal (default: HTTP/host.name)
SERVICE ?= HTTP
HOSTNAME ?= ${_HOSTNAME}
_HOSTNAME != hostname -f
SERVICE_PRINCIPAL ?= ${SERVICE}/${HOSTNAME}

# Path of keytab in a container
KEYTAB_PATH ?= /http.keytab

# krb5.conf for a container
# (if it is not found, use /etc/krb5.conf instead)
KRB5CONF ?= krb5.conf

# keytab for a container
# (if it is not found, create using kadmin)
KEYTAB ?= http.keytab

# Parameters to build and run docker.
CONTAINER_NAME ?= spnegocgi1
IMAGE_NAME ?= spnegocgi
PORT ?= 8089

CLEAN_FILES += nph-spnego.cgi .build-image.done krb5.conf.tmp
.if exists(${KEYTAB})
CLEAN_FILES += http.keytab.tmp
.else
ALL_CLEAN_FILES += http.keytab.tmp
.endif

rm-docker:
	-docker kill ${CONTAINER_NAME:Q}
	docker rm ${CONTAINER_NAME:Q}

run-docker: build-image
	-${MAKE} rm-docker
	docker run -d --name ${CONTAINER_NAME:Q} -p ${PORT}:80 ${IMAGE_NAME:Q}

build-image: .build-image.done
.build-image.done: nph-spnego.cgi spnegocgi Dockerfile krb5.conf.tmp http.keytab.tmp httpd.conf
	docker build -t ${IMAGE_NAME:Q} .
	touch $@

.if exists(${KRB5CONF})
krb5.conf.tmp: ${KRB5CONF}
	cp $> $@
.else
krb5.conf.tmp: /etc/krb5.conf
	cp $> $@
.endif

nph-spnego.cgi: nph-spnego.cgi.in
	sed 's/%%SERVICE%%/${SERVICE}/g; s/%%HOST%%/${HOSTNAME}/g; s!%%KEYTAB%%!${KEYTAB_PATH}!g' $> > $@
	chmod 755 $@

# Keytab file that contains service credentials
.if exists(${KEYTAB})
http.keytab.tmp: ${KEYTAB}
	cp $> $@
.else
http.keytab.tmp:
	kadmin -p ${ADMIN_PRINCIPAL} ext -k $@ ${SERVICE_PRINCIPAL}
.endif
