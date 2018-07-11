
#include <fcntl.h>
#include <string.h>

#include "slice-ssl-server.h"

#define MAX_SSL_SESSION_CONTEXT_BUCKET              (64 * 1024)        // max fd number

struct slice_ssl_sess_context
{
    int sock;
    SliceSSLState state;
    SSL *ssl;
};

struct slice_ssl_sess_context *context_bucket;

SliceReturnType slice_SSL_session_bucket_init(char *err)
{
    context_bucket = malloc(sizeof(struct slice_ssl_sess_context) * MAX_SSL_SESSION_CONTEXT_BUCKET);
    if (context_bucket == NULL) {
        if (err) sprintf(err, "Can't allocate memory for SSL Context bucket");
        return SLICE_RETURN_ERROR;
    }

    memset(context_bucket, 0, sizeof(struct slice_ssl_sess_context) * MAX_SSL_SESSION_CONTEXT_BUCKET);

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_SSL_session_accept(int sockfd, SliceSSLContext *context, char *err)
{
    if (sockfd >= MAX_SSL_SESSION_CONTEXT_BUCKET) {
        if (err) sprintf(err, "Session [%d] : Socket number is over bucket size [%d]", sockfd, MAX_SSL_SESSION_CONTEXT_BUCKET - 1);
        return SLICE_RETURN_ERROR;
    }
    if (!context) {
        if (err) sprintf(err, "Session [%d] : SSL Context is NULL", sockfd);
        return SLICE_RETURN_ERROR;
    }

    struct slice_ssl_sess_context *session_context = &(context_bucket[sockfd]);
    int reterr, errnum, skflag;
    //X509* client_cert;
    //char *x509_str;

    switch (session_context->state) {
        case SLICE_SSL_STATE_IDLE:
            if ((skflag = fcntl(sockfd, F_GETFL, 0)) < 0) {
                if (err) sprintf(err, "Session [%d] : fcntl(F_GETFL) return error [%s]", sockfd, strerror(errno));
                return SLICE_RETURN_ERROR;
            }

            if (fcntl(sockfd, F_SETFL, skflag | O_NONBLOCK) < 0) {
                if (err) sprintf(err, "Session [%d] : fcntl(F_SETFL) return error [%s]", sockfd, strerror(errno));
                return SLICE_RETURN_ERROR;
            }

            memset(session_context, 0, sizeof(*session_context));

            session_context->sock = sockfd;

            if ((session_context->ssl = SSL_new(context)) == NULL) {
                if (err) sprintf(err, "Session [%d] : Error while create new SSL", sockfd);
                memset(session_context, 0, sizeof(*session_context));
                return SLICE_RETURN_ERROR;
            }

            SSL_set_fd(session_context->ssl, sockfd);
            session_context->state = SLICE_SSL_STATE_CONNECTING;

        case SLICE_SSL_STATE_CONNECTING:
            if ((errnum = SSL_accept(session_context->ssl)) <= 0) {
                reterr = SSL_get_error(session_context->ssl, errnum);
                if (reterr == SSL_ERROR_WANT_READ || reterr == SSL_ERROR_WANT_WRITE) {
                    // connect in progress
                    if (err) sprintf(err, "IN_PROGRESS");
                    return SLICE_RETURN_INFO;
                } else {
                    if (err) sprintf(err, "Session [%d] : SSL accept error [%s]", sockfd, SliceSSLGetErrorString(reterr));
                    SSL_free(session_context->ssl);
                    memset(session_context, 0, sizeof(*session_context));
                    return SLICE_RETURN_ERROR;
                }
            }

            /*
            if ((client_cert = SSL_get_peer_certificate(session_context->ssl)) == NULL) {
                if (err) sprintf(err, "Session [%d] : Error while get server certificate", sockfd);
                SSL_free(session_context->ssl);
                memset(session_context, 0, sizeof(*session_context));
                return SLICE_RETURN_ERROR;
            }

            if ((x509_str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0)) == NULL) {
                if (err) sprintf(err, "Session [%d] : Error while get X509 subject name", sockfd);
                SSL_free(session_context->ssl);
                memset(session_context, 0, sizeof(*session_context));
                X509_free(client_cert);
                return SLICE_RETURN_ERROR;
            }
            free(x509_str);

            if ((x509_str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0)) == NULL) {
                if (err) sprintf(err, "Session [%d] : Error while get X509 issuer name", sockfd);
                SSL_free(session_context->ssl);
                memset(session_context, 0, sizeof(*session_context));
                X509_free(client_cert);
                return SLICE_RETURN_ERROR;
            }
            free(x509_str);

            X509_free(client_cert);
            */

            session_context->state = SLICE_SSL_STATE_CONNECTED;

            return SLICE_RETURN_NORMAL;

        case SLICE_SSL_STATE_CONNECTED:
        default:
            if (err) sprintf(err, "Sock [%d] : Connect invalid state [%d]", sockfd, (int)session_context->state);
            return SLICE_RETURN_ERROR;
    }

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_SSL_session_close(int sockfd, char *err)
{
    struct slice_ssl_sess_context *session_context = &(context_bucket[sockfd]);
    int ret = SLICE_RETURN_NORMAL;
    int reterr;

    if (session_context->ssl) {
        if ((ret = SSL_shutdown(session_context->ssl)) < 0) {
            if (err) {
                reterr = SSL_get_error(session_context->ssl, ret);
                sprintf(err, "Session [%d] : SSL shutdown error [%s]", sockfd, SliceSSLGetErrorString(reterr));
            }
        }
        SSL_free(session_context->ssl);
    }
    memset(session_context, 0, sizeof(*session_context));

    return ret;
}

SliceSSLState slice_SSL_session_get_state(int sockfd)
{
    return context_bucket[sockfd].state;
}

SliceReturnType slice_SSL_session_read(int sockfd, void *read_buff, size_t buff_len, int *read_len, int *err_num, char *err)
{
    struct slice_ssl_sess_context *session_context = &(context_bucket[sockfd]);
    int ret;
    int reterr;

    *err_num = 0;
    *read_len = 0;

    if ((ret = SSL_read(session_context->ssl, read_buff, buff_len)) <= 0) {
        reterr = SSL_get_error(session_context->ssl, ret);
        if (reterr == SSL_ERROR_WANT_READ || reterr == SSL_ERROR_WANT_WRITE) {
            // ssl continue read
            return SLICE_RETURN_INFO;
        } else if (reterr == SSL_ERROR_SYSCALL) {
            if (errno == 0) {
                if (err) sprintf(err, "Connection Closed");
                *err_num = ECONNABORTED;
            } else {
                if (err) sprintf(err, "%d:%s", errno, strerror(errno));
                *err_num = errno;
            }
            return SLICE_RETURN_ERROR;
        } else if (reterr == SSL_ERROR_ZERO_RETURN) {
            if (err) sprintf(err, "Sock [%d] : Connection Closed", sockfd);
            return SLICE_RETURN_ERROR;
        } else {
            if (err) sprintf(err, "Session [%d] : SSL read error [%s]", sockfd, SliceSSLGetErrorString(reterr));
            return SLICE_RETURN_ERROR;
        }
    }

    *read_len = ret;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_SSL_session_write(int sockfd, void *write_buff, size_t write_size, int *write_len, int *err_num, char *err)
{
    struct slice_ssl_sess_context *session_context = &(context_bucket[sockfd]);
    int ret;
    int reterr;

    *err_num = 0;
    *write_len = 0;

    if ((ret = SSL_write(session_context->ssl, write_buff, write_size)) <= 0) {
        reterr = SSL_get_error(session_context->ssl, ret);
        if (reterr == SSL_ERROR_WANT_READ || reterr == SSL_ERROR_WANT_WRITE) {
            // ssl continue write
            return SLICE_RETURN_INFO;
        } else if (reterr == SSL_ERROR_SYSCALL) {
            if (err) sprintf(err, "%s", strerror(errno));
            *err_num = errno;
            return SLICE_RETURN_ERROR;
        } else if (reterr == SSL_ERROR_ZERO_RETURN) {
            if (err) sprintf(err, "Sock [%d] : Connection Closed", sockfd);
            return SLICE_RETURN_ERROR;
        } else {
            if (err) sprintf(err, "Session [%d] : SSL write error [%s]", sockfd, SliceSSLGetErrorString(reterr));
            return SLICE_RETURN_ERROR;
        }
    }

    *write_len = ret;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_SSL_session_bucket_destroy(char *err)
{
    struct slice_ssl_sess_context *sess_context;
    int i;

    if (!context_bucket) {
        if (err) sprintf(err, "Session context bucket not found");
        return SLICE_RETURN_INFO;
    }

    for (i = 0; i < MAX_SSL_SESSION_CONTEXT_BUCKET; i++) {
        sess_context = &(context_bucket[i]);
        if (sess_context->sock > 0 && sess_context->ssl) {
            slice_SSL_session_close(sess_context->sock, NULL);
        }
    }

    free(context_bucket);
    context_bucket = NULL;

    return SLICE_RETURN_NORMAL;
}


