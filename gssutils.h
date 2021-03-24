#ifndef GSSUTILS_H_27343A14_658E_44C3_8505_3F0E679229A8
#define GSSUTILS_H_27343A14_658E_44C3_8505_3F0E679229A8

char *gss_primary_error_message(OM_uint32 status_code);
int use_keytab(const char *keytab);
int make_service_name(const char *service, const char *hostname,
		      gss_name_t *output_name);
int get_default_cred(const char *service, const char *hostname,
		     gss_cred_id_t *output_cred);
void gsserror(OM_uint32 status_code, const char *name);
char *gss_name_to_cstr(gss_name_t name);

static inline OM_uint32
GSSCALL(const char *name, OM_uint32 status)
{
    gsserror(status, name);
    return status;
}

#endif	/* GSSUTILS_H_27343A14_658E_44C3_8505_3F0E679229A8 */
