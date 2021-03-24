FROM httpd:2.4

RUN apt-get -y update && apt-get -y install heimdal-clients heimdal-dev
COPY http.keytab.tmp /http.keytab
RUN chmod 444 /http.keytab
COPY krb5.conf.tmp /etc/krb5.conf
COPY spnegocgi nph-spnego.cgi /usr/local/apache2/cgi-bin/
COPY index.html /usr/local/apache2/htdocs/index.html
COPY httpd.conf /usr/local/apache2/conf/httpd.conf
