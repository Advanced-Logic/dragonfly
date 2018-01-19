#ifndef _SLICE_SSL_H_
#define _SLICE_SSL_H_

#include <openssl/ssl.h>
#include <openssl/err.h>

enum slice_ssl_connection_type
{
    SLICE_SSL_CONNECTION_TYPE_CLIENT,
    SLICE_SSL_CONNECTION_TYPE_SERVER
};

enum slice_ssl_file_type
{
    SLICE_SSL_FILE_TYPE_NONE = 0,
    SLICE_SSL_FILE_TYPE_PEM,
    SLICE_SSL_FILE_TYPE_ASN1
};

enum slice_ssl_mode
{
    SLICE_SSL_MODE_NONE,
    SLICE_SSL_MODE_SSL_2,
    SLICE_SSL_MODE_SSL_3,
    SLICE_SSL_MODE_SSL_23,
    SLICE_SSL_MODE_TLS_1_0,
    SLICE_SSL_MODE_TLS_1_1,
    SLICE_SSL_MODE_TLS_1_2
};

typedef enum slice_ssl_connection_type SliceSSLConnectionType;
typedef enum slice_ssl_file_type SliceSSLFileType;
typedef enum slice_ssl_mode SliceSSLMode;

typedef SSL_CTX SliceSSLContext;

#ifdef __cplusplus
extern "C" {
#endif

void slice_ssl_load_library();
char *slice_ssl_get_error_string(int err_num);
SliceSSLContext *slice_ssl_context_create(SliceSSLConnectionType type, SliceSSLMode mode, SliceSSLFileType file_type, char *cert_file_path, char *ssh_key_file_path, char *rsa_key_file_path, char *err);
SliceReturnType slice_ssl_context_destroy(SliceSSLContext *context, char *err);

#ifdef __cplusplus
}
#endif

#define SliceSSLLoadLibrary() slice_ssl_load_library()
#define SliceSSLGetErrorString(_err_num) slice_ssl_get_error_string(_err_num)
#define SliceSSLContextCreate(_type, _mode, _file_type, _cert_file, _ssh_key_file, _rsa_key_file, _err) slice_ssl_context_create(_type, _mode, _file_type, _cert_file, _ssh_key_file, _rsa_key_file, _err)
#define SliceSSLContextDestroy(_context, _err) slice_ssl_context_destroy(_context, _err)

#endif
