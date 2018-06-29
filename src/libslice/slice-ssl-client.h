#ifndef _SLICE_SSL_CLIENT_H_
#define _SLICE_SSL_CLIENT_H_

#include "slice-ssl.h"

#define MAX_SSL_CLIENT_CONTEXT_BUCKET           (64 * 1024)

struct slice_ssl_clnt_context
{
    int sock;
    SSL *ssl;
    SliceSSLState state;
};

#ifdef __cplusplus
extern "C" {
#endif

SliceReturnType slice_SSL_client_bucket_init(char *err);
SliceReturnType slice_SSL_client_bucket_destroy(char *err);
SliceReturnType slice_SSL_client_connect(int sockfd, SSL_CTX *context, char *err);
SliceReturnType slice_SSL_client_shutdown(int sockfd, char *err);
SliceReturnType slice_SSL_client_read(int sockfd, void *read_buff, size_t buff_len, int *read_len, int *err_num, char *err);
SliceReturnType slice_SSL_client_write(int sockfd, void *write_buff, size_t write_size, int *write_len, int *err_num, char *err);
SliceSSLState slice_SSL_client_get_state(int sockfd);

#ifdef __cplusplus
}
#endif

#define SliceSSLClientBucketInit(_err) slice_SSL_client_bucket_init(_err)
#define SliceSSLClientBucketDestroy(_err) slice_SSL_client_bucket_destroy(_err)
#define SliceSSLClientConnect(_sockfd, _context, _err) slice_SSL_client_connect(_sockfd, _context, _err)
#define SliceSSLClientShutdown(_sockfd, _err) slice_SSL_client_shutdown(_sockfd, _err)
#define SliceSSLClientRead(_sockfd, _read_buff, _buff_len, _read_len, _err_num, _err) slice_SSL_client_read(_sockfd, _read_buff, _buff_len, _read_len, _err_num, _err)
#define SliceSSLClientWrite(_sockfd, _write_buff, _write_size, _write_len, _err_num, _err) slice_SSL_client_write(_sockfd, _write_buff, _write_size, _write_len, _err_num, _err)
#define SliceSSLClientGetState(_sockfd) slice_SSL_client_get_state(_sockfd)

#endif
