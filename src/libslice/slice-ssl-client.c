
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "slice-ssl-client.h"

struct slice_ssl_clnt_context *context_bucket;

SliceReturnType slice_SSL_client_bucket_init(char *err)
{
    context_bucket = malloc(sizeof(struct slice_ssl_clnt_context) * MAX_SSL_CLIENT_CONTEXT_BUCKET);
    if (context_bucket == NULL) {
        if (err) sprintf(err, "Can't allocate memory for SSL Context bucket.");
        return SLICE_RETURN_ERROR;
    }

    memset(context_bucket, 0, sizeof(struct slice_ssl_clnt_context) * MAX_SSL_CLIENT_CONTEXT_BUCKET);

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_SSL_client_connect(int sockfd, SSL_CTX *context, char *err)
{
    if (sockfd >= MAX_SSL_CLIENT_CONTEXT_BUCKET) {
        if (err) sprintf(err, "Sock [%d] : Socket number is over bucket size [%d]", sockfd, MAX_SSL_CLIENT_CONTEXT_BUCKET - 1);
        return SLICE_RETURN_ERROR;
    }

    struct slice_ssl_clnt_context *client_context = &(context_bucket[sockfd]);
    int errnum, skflag;
    X509* server_cert;
    char *x509_str;
    int reterr;

    switch (client_context->state) {
        case SLICE_SSL_STATE_IDLE:
            if ((skflag = fcntl(sockfd, F_GETFL, 0)) < 0) {
                if (err) sprintf(err, "Sock [%d] : fcntl(F_GETFL) return error [%s].", sockfd, strerror(errno));
                return SLICE_RETURN_ERROR;
            }

            if (fcntl(sockfd, F_SETFL, skflag | O_NONBLOCK) < 0) {
                if (err) sprintf(err, "Sock [%d] : fcntl(F_SETFL) return error [%s].", sockfd, strerror(errno));
                return SLICE_RETURN_ERROR;
            }

            memset(client_context, 0, sizeof(*client_context));

            client_context->sock = sockfd;

            if ((client_context->ssl = SSL_new(context)) == NULL) {
                if (err) sprintf(err, "Sock [%d] : Error while create new SSL.", sockfd);
                memset(client_context, 0, sizeof(*client_context));
                return SLICE_RETURN_ERROR;
            }

            SSL_set_fd(client_context->ssl, sockfd);
            client_context->state = SLICE_SSL_STATE_CONNECTING;

        case SLICE_SSL_STATE_CONNECTING:
            if ((errnum = SSL_connect(client_context->ssl)) <= 0) {
                reterr = SSL_get_error(client_context->ssl, errnum);
                if (reterr == SSL_ERROR_WANT_READ || reterr == SSL_ERROR_WANT_WRITE) {
                    // connect in progress
                    if (err) sprintf(err, "IN_PROGRESS");
                    return SLICE_RETURN_INFO;
                } else {
                    if (err) sprintf(err, "Sock [%d] : SSL connect error [%s].", sockfd, SliceSSLGetErrorString(reterr));
                    SSL_free(client_context->ssl);
                    memset(client_context, 0, sizeof(*client_context));
                    return SLICE_RETURN_ERROR;
                }
            }

            if ((server_cert = SSL_get_peer_certificate(client_context->ssl)) == NULL) {
                if (err) sprintf(err, "Sock [%d] : Error while get server certificate.", sockfd);
                SSL_free(client_context->ssl);
                memset(client_context, 0, sizeof(*client_context));
                return SLICE_RETURN_ERROR;
            }

            if ((x509_str = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0)) == NULL) {
                if (err) sprintf(err, "Sock [%d] : Error while get X509 subject name.", sockfd);
                SSL_free(client_context->ssl);
                memset(client_context, 0, sizeof(*client_context));
                X509_free(server_cert);
                return SLICE_RETURN_ERROR;
            }
            free(x509_str);

            if ((x509_str = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0)) == NULL) {
                if (err) sprintf(err, "Sock [%d] : Error while get X509 issuer name.", sockfd);
                SSL_free(client_context->ssl);
                memset(client_context, 0, sizeof(*client_context));
                X509_free(server_cert);
                return SLICE_RETURN_ERROR;
            }
            free(x509_str);

            X509_free(server_cert);

            client_context->state = SLICE_SSL_STATE_CONNECTED;
            return SLICE_RETURN_NORMAL;

        case SLICE_SSL_STATE_CONNECTED:
        default:
            if (err) sprintf(err, "Sock [%d] : Connect invalid state [%d].", sockfd, (int)client_context->state);
            return SLICE_RETURN_ERROR;
    }
}

SliceReturnType slice_SSL_client_shutdown(int sockfd, char *err)
{
    struct slice_ssl_clnt_context *client_context = &(context_bucket[sockfd]);
    int ret = SLICE_RETURN_NORMAL;
    int reterr;

    if (client_context->ssl != NULL) {
        if ((ret = SSL_shutdown(client_context->ssl)) < 0) {
            if (err) {
                reterr = SSL_get_error(client_context->ssl, ret);
                sprintf(err, "Sock [%d] : SSL shutdown error [%s].", sockfd, SliceSSLGetErrorString(reterr));
            }
        }
        SSL_free(client_context->ssl);
    }
    memset(client_context, 0, sizeof(*client_context));

    return ret;
}

SliceSSLState slice_SSL_client_get_state(int sockfd)
{
    return context_bucket[sockfd].state;
}

SliceReturnType slice_SSL_client_read(int sockfd, void *read_buff, size_t buff_len, int *read_len, int *err_num, char *err)
{
    struct slice_ssl_clnt_context *client_context = &(context_bucket[sockfd]);
    int ret;
    int reterr;

    *err_num = 0;
    *read_len = 0;

    if ((ret = SSL_read(client_context->ssl, read_buff, buff_len)) <= 0) {
        reterr = SSL_get_error(client_context->ssl, ret);
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
        } else {
            if (err) sprintf(err, "Sock [%d] : SSL read error [%s].", sockfd, SliceSSLGetErrorString(reterr));
            return SLICE_RETURN_ERROR;
        }
    }

    *read_len = ret;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_SSL_client_write(int sockfd, void *write_buff, size_t write_size, int *write_len, int *err_num, char *err)
{
    struct slice_ssl_clnt_context *client_context = &(context_bucket[sockfd]);
    int ret;
    int reterr;

    *err_num = 0;
    *write_len = 0;

    if ((ret = SSL_write(client_context->ssl, write_buff, write_size)) <= 0) {
        reterr = SSL_get_error(client_context->ssl, ret);
        if (reterr == SSL_ERROR_WANT_READ || reterr == SSL_ERROR_WANT_WRITE) {
            // ssl continue read
            return SLICE_RETURN_INFO;
        } else if (reterr == SSL_ERROR_SYSCALL) {
            if (err) sprintf(err, "%s", strerror(errno));
            *err_num = errno;
            return SLICE_RETURN_ERROR;
        } else {
            if (err) sprintf(err, "Sock [%d] : SSL write error [%s].", sockfd, SliceSSLGetErrorString(reterr));
            return SLICE_RETURN_ERROR;
        }
    }

    *write_len = ret;

    return SLICE_RETURN_NORMAL;
}

SliceReturnType slice_SSL_client_bucket_destroy(char *err)
{
    struct slice_ssl_clnt_context *client_context;
    int i;

    if (!context_bucket) {
        if (err) sprintf(err, "Client context bucket not found");
        return SLICE_RETURN_INFO;
    }

    for (i = 0; i <MAX_SSL_CLIENT_CONTEXT_BUCKET; i++) {
        client_context = &(context_bucket[i]);
        if (client_context->sock > 0 && client_context->ssl) {
            slice_SSL_client_shutdown(client_context->sock, NULL);
        }
    }

    return SLICE_RETURN_NORMAL;
}

