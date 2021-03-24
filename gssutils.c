#include <stdlib.h>
#include <string.h>

#include <gssapi.h>
#include <gssapi/gssapi_krb5.h>

#include "log.h"
#include "gssutils.h"

char *
gss_primary_error_message(OM_uint32 status_code)
{
    OM_uint32 message_context = 0;
    OM_uint32 major_status, minor_status;
    gss_buffer_desc status_string;
    char *error_message = NULL;

    do {
	major_status = gss_display_status(
	    &minor_status,
	    status_code,
	    GSS_C_GSS_CODE,
	    GSS_C_NO_OID,
	    &message_context,
	    &status_string);

	if (!GSS_ERROR(major_status)) {
	    error_message = strdup(status_string.value);
	    gss_release_buffer(&minor_status, &status_string);
	    break;
	}
    } while (message_context != 0 && error_message == NULL);

    return error_message;
}

void
gsserror(OM_uint32 status_code, const char *msg)
{
    if (! GSS_ERROR(status_code))
	return;
    
    char *error = gss_primary_error_message(status_code);
    gsscgi_error("%s: %s", msg, error);
    if (error != NULL)
	free(error);
}

/**
 * 参照するkeytabファイルを設定する。
 * 成功すれば0を返す。
 */
int
use_keytab(const char *keytab)
{
    OM_uint32 major_status = gsskrb5_register_acceptor_identity(keytab);
    if (GSS_ERROR(major_status)) {
	gsserror(major_status, "gsskrb5_register_acceptor_identity");
	return -1;
    }
    
    return 0;
}

int
make_service_name(const char *service, const char *hostname,
		  gss_name_t *output_name)
{
    char *namestr = NULL;
    if (hostname == NULL) {
	namestr = strdup(service);
	if (namestr == NULL) {
	    gsscgi_perror("strdup");
	    return -1;
	}
    } else {
	namestr = malloc(strlen(service) + 1 + strlen(hostname) + 1);
	if (namestr == NULL) {
	    gsscgi_perror("malloc");
	    return -1;
	}
	strcpy(namestr, service);
	strcat(namestr, "@");
	strcat(namestr, hostname);
    }

    gss_buffer_desc buf;
    buf.value = namestr;
    buf.length = strlen(namestr) + 1;

    OM_uint32 major_status, minor_status;
    major_status = gss_import_name(&minor_status,
				   &buf,
				   GSS_C_NT_HOSTBASED_SERVICE,
				   output_name);
    gsserror(major_status, "gss_import_name");
    
    gss_release_buffer(&minor_status, &buf);
    return GSS_ERROR(major_status) ? -1 : 0;
}

int
get_default_cred(const char *service, const char *hostname,
		 gss_cred_id_t *output_cred)
{
    gss_name_t server_name;
    int rc = 0;

    if (make_service_name(service, hostname, &server_name) != 0)
	return -1;

    OM_uint32 major_status, minor_status;
    major_status = gss_acquire_cred(&minor_status,
				    server_name,
				    GSS_C_INDEFINITE,
				    GSS_C_NO_OID_SET,
				    GSS_C_ACCEPT,
				    output_cred,
				    NULL, NULL);
    if (GSS_ERROR(major_status)) {
	gsserror(major_status, "gss_acquire_cred");
	rc = -1;
    }

    major_status = gss_release_name(&minor_status, &server_name);
    if (GSS_ERROR(major_status))
	gsserror(major_status, "gss_release_name");

    return rc;
}

char *
gss_name_to_cstr(gss_name_t name)
{
    OM_uint32 major_status, minor_status;
    gss_buffer_desc buf;

    major_status = GSSCALL("gss_display_name",
			   gss_display_name(&minor_status,
					    name,
					    &buf,
					    NULL));
    if (GSS_ERROR(major_status))
	return NULL;

    char *name_cstr = strdup(buf.value);
    GSSCALL("gss_release_buffer", 
	    gss_release_buffer(&minor_status, &buf));
    
    return name_cstr;
}
