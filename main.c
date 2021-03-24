#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#include <gssapi.h>

#include "log.h"
#include "gssutils.h"
#include "base64.h"

extern char **environ;

const char *g_progname = "spnegocgi";
const char *g_keytab = NULL;
const char *g_hostname = NULL;
const char *g_service = "HTTP";

noreturn void
fatal(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    gsscgi_vlog("ERROR", format, ap);
    va_end(ap);

    printf("HTTP/1.0 500 Internal Server Error\r\n");
    printf("\r\n");
    exit(1);
}

void
get_server_cred(gss_cred_id_t *output_cred)
{
    if (g_keytab != NULL) {
	if (use_keytab(g_keytab) != 0) {
	    fatal("cannot read %s", g_keytab);
	}
    }
    if (get_default_cred(g_service, g_hostname, output_cred) != 0) {
	fatal("cannot get credential");
    }
}

char *
escape_html(const char *s)
{
    char *buf = malloc(strlen(s) * 6 + 1);
    char *q=buf;
    for (const char *p=s; *p != '\0'; ++p) {
	switch (*p) {
	case '&':
	    strcat(q, "&amp;"); q += strlen("&amp;"); break;
	case '<':
	    strcat(q, "&lt;"); q += strlen("&lt;"); break;
	case '>':
	    strcat(q, "&gt;"); q += strlen("&gt;"); break;
	case '"':
	    strcat(q, "&quot;"); q += strlen("&quot;"); break;
	default:
	    *q = *p; q++; break;
	}
    }
    *q = '\0';
    return buf;
}

static size_t
strarray_len(char const * const *strarray)
{
    size_t len = 0;
    while (*(strarray + len) != NULL)
	len++;
    return len;
}

static int
compare_strs(const char **p, const char **q)
{
    return strcmp(*p, *q);
}

typedef int (*qsort_comp)(const void *, const void *);

static char **
sort_environ()
{
    size_t len = strarray_len((const char **)environ);
    char **environ2 = malloc(sizeof(environ[0]) * (len + 1));
    memcpy(environ2, environ, sizeof(environ[0]) * (len + 1));
    qsort(environ2, len, sizeof(environ2[0]), (qsort_comp)compare_strs);
    return environ2;
}

void
print_content(const char *user)
{
    char *user_esc = escape_html(user);
    printf("<!DOCTYPE html><html><head>"
	   "<meta charset='UTF-8'/>"
	   "<title>Authentication Succeeded!</title>"
	   "</head><body>"
	   "<h1>SPNEGO authentication succeeded!</h1>"
	   "<p>Hello %s!</p>"
	   "<hr>",
	   user_esc);
    free(user_esc);

    printf("<h2>Environment Variables</h2><pre>");
    char **envs = sort_environ();
    for (char **p = envs; *p != NULL; p++) {
	char *esc = escape_html(*p);
	printf("%s\n", esc);
	free(esc);
    }
    free(envs);
    printf("</pre></body></html>");
}

void
http_status_nph(int status)
{
    char *msg = NULL;
    switch (status) {
    case 200: msg = "OK"; break;
    case 401: msg = "Unauthorized"; break;
    }
    printf("HTTP/1.0 %03d%s%s\r\n",
	   status,
	   (msg == NULL ? "" : " "),
	   (msg == NULL ? "" : msg));
}

void
http_status_cgi(int status)
{
    printf("Status: %03d\r\n", status);
}

#define http_status http_status_nph

void
response_ok(const char *token, const char *user)
{
    http_status(200);
    printf("Content-Type: text/html; charset=UTF-8\r\n");
    printf("Connection: Keep-Alive\r\n");
    if (token != NULL) {
	gsscgi_debug("return token %s", token);
	printf("WWW-Authenticate: Negotiate %s\r\n", token);
    }
    printf("\r\n");
    print_content(user);
}

void
http_401(const char *token, const char *msg)
{
    http_status(401);
    printf("Content-Type: text/plain\r\n"
	   "WWW-Authenticate: Negotiate%s%s\r\n"
	   "\r\n"
	   "%s",
	   (token == NULL ? "" : " "),
	   (token == NULL ? "" : token),
	   (msg == NULL ? "" : msg));
}

void
handle_request(gss_cred_id_t server_cred)
{
    const char *auth_header = getenv("HTTP_AUTHORIZATION");
    while (auth_header != NULL && *auth_header == ' ')
	auth_header++;
    if (auth_header == NULL || *auth_header == '\0') {
	gsscgi_debug("No Authorization header in the request");
	
	/*
	 * No Authorization: header.
	 * Return 401 error to initiate authentication.
	 */
	http_401(NULL, "SPNEGO Authentication is required.");
	return;
    }

    gsscgi_debug("Authorization: %s", auth_header);
    if (strncasecmp(auth_header, "Negotiate ", strlen("Negotiate ")) != 0) {
	gsscgi_info("unexpected authentication type");

	/*
	 * Authorization: header with unxpected authenticatino type
	 */
	http_401(NULL, "SPNEGO Authentication is required.");
	return;
    }

    size_t tokensize = 0;
    char *token = base64Decode(auth_header + strlen("Negotiate "),
			       &tokensize,
			       BASE64_TYPE_STANDARD);
    if (token == NULL) {
	gsscgi_info("malformed token; decoding base64 failed");

	http_401(NULL, "Malformed token");
	return;
    }

    OM_uint32 major_status, minor_status;
    gss_buffer_desc tokenbuf = {
	.value = token,	/* token will be freed by gss_release_buffer */
	.length = tokensize
    };
    gss_buffer_desc send_token = GSS_C_EMPTY_BUFFER;
    gss_name_t client_name = GSS_C_NO_NAME;
    gss_ctx_id_t context = GSS_C_NO_CONTEXT;
    int flags;
    major_status = gss_accept_sec_context(&minor_status,
					  &context,
					  server_cred,
					  &tokenbuf,
					  GSS_C_NO_CHANNEL_BINDINGS,
					  &client_name,
					  NULL, /* mech_type */
					  &send_token,
					  &flags,
					  NULL, NULL);
    if (GSS_ERROR(major_status)) {
	gsserror(major_status, "gss_accept_sec_context");

	http_401(NULL, NULL);
    } else {
	char *send_token_b64 = NULL;
	if (send_token.length != 0) {
	    send_token_b64 = base64Encode(send_token.value,
					  send_token.length,
					  BASE64_TYPE_STANDARD);

	    GSSCALL("gss_release_buffer",
		    gss_release_buffer(&minor_status, &send_token));
	}
	
	if (major_status & GSS_S_CONTINUE_NEEDED) {
	    gsscgi_info("gss_accept_sec_context returned GSS_S_CONTINUE_NEEDED");

	    http_401(send_token_b64, NULL);
	} else {
	    gsscgi_debug("gss_accept_sec_context completed");

	    char *user = gss_name_to_cstr(client_name);
	    gsscgi_info("authentication completed; user=%s", user);
	    response_ok(send_token_b64, user);
	    free(user);
	}

	if (send_token_b64 != NULL)
	    free(send_token_b64);

	GSSCALL("gss_release_name",
		gss_release_name(&minor_status, &client_name));

	GSSCALL("gss_delete_sec_context",
		gss_delete_sec_context(&minor_status,
				       &context,
				       GSS_C_NO_BUFFER));
    }

    GSSCALL("gss_release_buffer",
	    gss_release_buffer(&minor_status, &tokenbuf));
}

noreturn void
usage(int status)
{
    printf("Usage: %s [--help] [-t keytab] [-s service] [-h hostname]\n", g_progname);
    exit(status);
}

void
parse_options(int argc, char **argv)
{
    const char *lastslash = strrchr(argv[0], '/');
    const char *progname = (lastslash == NULL ? argv[0] : lastslash + 1);
    if (*progname != '\0')
	g_progname = progname;

    struct option options[] = {
	{ "help", 0, NULL, 'H' },
	{ "keytab", 0, NULL, 't' },
	{ "hostname", 0, NULL, 'h' },
	{ "service", 0, NULL, 's' }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "h:s:t:", options, NULL)) != EOF) {
	switch (opt) {
	case 'H': usage(0); break;
	case 't': g_keytab = optarg; break;
	case 'h': g_hostname = optarg; break;
	case 's': g_service = optarg; break;
	default:  usage(1); break;
	}
    }

    argc -= optind;
    argv += optind;
    if (argc > 0)
	fatal("excessive command line arguments");
}

int
main(int argc, char **argv)
{
    parse_options(argc, argv);

    gss_cred_id_t server_cred;
    get_server_cred(&server_cred);

    handle_request(server_cred);

    OM_uint32 minor_status;
    GSSCALL("gss_release_cred",
	    gss_release_cred(&minor_status, &server_cred));
  
    return 0;
}
