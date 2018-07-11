#ifndef _SLICE_SSL_SERVER_H_
#define _SLICE_SSL_SERVER_H_

#include "slice-ssl.h"

#ifdef __cplusplus
extern "C" {
#endif

SliceReturnType slice_SSL_session_bucket_init(char *err);
SliceReturnType slice_SSL_session_bucket_destroy(char *err);
SliceReturnType slice_SSL_session_accept(int sockfd, SliceSSLContext *context, char *err);
SliceReturnType slice_SSL_session_close(int sockfd, char *err);
SliceSSLState slice_SSL_session_get_state(int sockfd);
SliceReturnType slice_SSL_session_read(int sockfd, void *read_buff, size_t buff_len, int *read_len, int *err_num, char *err);
SliceReturnType slice_SSL_session_write(int sockfd, void *write_buff, size_t write_size, int *write_len, int *err_num, char *err);

#ifdef __cplusplus
}
#endif

#define SliceSSLSessionBucketInit(_err) slice_SSL_session_bucket_init(_err)
#define SliceSSLSessionBucketDestroy(_err) slice_SSL_session_bucket_destroy(_err)
#define SliceSSLSessionAccept(_sockfd, _context, _err) slice_SSL_session_accept(_sockfd, _context, _err)
#define SliceSSLSessionClose(_sockfd, _err) slice_SSL_session_close(_sockfd, _err)
#define SliceSSLSessionGetState(_sockfd) slice_SSL_session_get_state(_sockfd)
#define SliceSSLSessionRead(_sockfd, _read_buff, _buff_len, _read_len, _err_num, _err) slice_SSL_session_read(_sockfd, _read_buff, _buff_len, _read_len, _err_num, _err)
#define SliceSSLSessionWrite(_sockfd, _write_buff, _write_size, _write_len, _err_num, _err) slice_SSL_session_write(_sockfd, _write_buff, _write_size, _write_len, _err_num, _err)

#endif // !_SLICE_SSL_SERVER_H_
