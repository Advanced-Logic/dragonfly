
#include <stdio.h>

#include "slice.h"
#include "slice-ssl.h"

static int ssl_library_loaded = 0;

void slice_ssl_load_library()
{
    if (!ssl_library_loaded) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();

        ssl_library_loaded = 1;
    }
}

char *slice_ssl_get_error_string(int err_num)
{
    switch (err_num) {
        case SSL_ERROR_NONE:
            return "SSL_ERROR_NONE";
        case SSL_ERROR_ZERO_RETURN:
            return "SSL_ERROR_ZERO_RETURN";
        case SSL_ERROR_WANT_READ:
            return "SSL_ERROR_WANT_READ";
        case SSL_ERROR_WANT_WRITE:
            return "SSL_ERROR_WANT_WRITE";
        case SSL_ERROR_WANT_CONNECT:
            return "SSL_ERROR_WANT_CONNECT";
        case SSL_ERROR_WANT_ACCEPT:
            return "SSL_ERROR_WANT_ACCEPT";
        case SSL_ERROR_WANT_X509_LOOKUP:
            return "SSL_ERROR_WANT_X509_LOOKUP";
        case SSL_ERROR_SYSCALL:
            return "SSL_ERROR_SYSCALL";
        case SSL_ERROR_SSL:
            return "SSL_ERROR_SSL";
        default:
            return "SSL_ERROR_UNKNOWN";
    }
}

SliceSSLContext *slice_ssl_context_create(SliceSSLConnectionType type, SliceSSLMode mode, SliceSSLFileType file_type, char *cert_file_path, char *ssh_key_file_path, char *rsa_key_file_path, char *err)
{
    const SSL_METHOD *method;
    SliceSSLContext *context;
    int err_num;

    if (type == SLICE_SSL_CONNECTION_TYPE_CLIENT) {
        switch (mode) {
            case SLICE_SSL_MODE_SSL_2:
                if (err) sprintf(err, "SSLv2 is obsolete");
                return NULL;

            case SLICE_SSL_MODE_SSL_3:
                method = SSLv3_client_method();
                break;

            case SLICE_SSL_MODE_SSL_23:
                method = SSLv23_client_method();
                break;

            case SLICE_SSL_MODE_TLS_1_0:
                method = TLSv1_client_method();
                break;

            case SLICE_SSL_MODE_TLS_1_1:
                method = TLSv1_1_client_method();
                break;

            case SLICE_SSL_MODE_TLS_1_2:
                method = TLSv1_2_client_method();
                break;

            default:
                if (err) sprintf(err, "Unknown SSL mode [%d]", (int)mode);
                return NULL;
        }
    } else {
        switch (mode) {
            case SLICE_SSL_MODE_SSL_2:
                if (err) sprintf(err, "SSLv2 is obsolete");
                return NULL;

            case SLICE_SSL_MODE_SSL_3:
                method = SSLv3_server_method();
                break;

            case SLICE_SSL_MODE_SSL_23:
                method = SSLv23_server_method();
                break;

            case SLICE_SSL_MODE_TLS_1_0:
                method = TLSv1_server_method();
                break;

            case SLICE_SSL_MODE_TLS_1_1:
                method = TLSv1_1_server_method();
                break;

            case SLICE_SSL_MODE_TLS_1_2:
                method = TLSv1_2_server_method();
                break;

            default:
                if (err) sprintf(err, "Unknown SSL mode [%d]", (int)mode);
                return NULL;
        }
    }

    if (!(context = SSL_CTX_new(method))) {
        if (err) sprintf(err, "Error while create new SSL Context.");
        return NULL;
    }

    SSL_CTX_set_cipher_list(context, "ALL");

    if (file_type) {
        if (type == SLICE_SSL_CONNECTION_TYPE_CLIENT) {
            if (ssh_key_file_path && ssh_key_file_path[0] != 0) {
                if (file_type == SLICE_SSL_FILE_TYPE_PEM) {
                    if ((err_num = SSL_CTX_use_PrivateKey_file(context, ssh_key_file_path, SSL_FILETYPE_PEM)) <= 0) {
                        if (err) sprintf(err, "Use private key file [%s] error.", ssh_key_file_path);
                        SSL_CTX_free(context);
                        return NULL;
                    }
                } else {
                    if ((err_num = SSL_CTX_use_PrivateKey_file(context, ssh_key_file_path, SSL_FILETYPE_ASN1)) <= 0) {
                        if (err) sprintf(err, "Use private key file [%s] error.", ssh_key_file_path);
                        SSL_CTX_free(context);
                        return NULL;
                    }
                }
            } else if (rsa_key_file_path && rsa_key_file_path[0] != 0) {
                if (file_type == SLICE_SSL_FILE_TYPE_PEM) {
                    if ((err_num = SSL_CTX_use_RSAPrivateKey_file(context, rsa_key_file_path, SSL_FILETYPE_PEM)) <= 0) {
                        if (err) sprintf(err, "Use RSA private key file [%s] error.", rsa_key_file_path);
                        SSL_CTX_free(context);
                        return NULL;
                    }
                } else {
                    if ((err_num = SSL_CTX_use_RSAPrivateKey_file(context, rsa_key_file_path, SSL_FILETYPE_ASN1)) <= 0) {
                        if (err) sprintf(err, "Use RSA private key file [%s] error.", rsa_key_file_path);
                        SSL_CTX_free(context);
                        return NULL;
                    }
                }
            }
        } else {
            if (cert_file_path && cert_file_path[0] != 0) {
                if (file_type == SLICE_SSL_FILE_TYPE_PEM) {
                    if ((err_num = SSL_CTX_use_certificate_file(context, cert_file_path, SSL_FILETYPE_PEM)) <= 0) {
                        if (err) sprintf(err, "Use certificate file [%s] error.", cert_file_path);
                        SSL_CTX_free(context);
                        return NULL;
                    }
                } else {
                    if ((err_num = SSL_CTX_use_certificate_file(context, cert_file_path, SSL_FILETYPE_ASN1)) <= 0) {
                        if (err) sprintf(err, "Use certificate file [%s] error.", cert_file_path);
                        SSL_CTX_free(context);
                        return NULL;
                    }
                }
            } else {
                if (err) sprintf(err, "SSL server context need certificate file");
                SSL_CTX_free(context);
                return NULL;
            }
        }

        if (ssh_key_file_path && ssh_key_file_path[0] != 0) {
            if (file_type == SLICE_SSL_FILE_TYPE_PEM) {
                if ((err_num = SSL_CTX_use_PrivateKey_file(context, ssh_key_file_path, SSL_FILETYPE_PEM)) <= 0) {
                    if (err) sprintf(err, "Use private key file [%s] error.", ssh_key_file_path);
                    SSL_CTX_free(context);
                    return NULL;
                }
            } else {
                if ((err_num = SSL_CTX_use_PrivateKey_file(context, ssh_key_file_path, SSL_FILETYPE_ASN1)) <= 0) {
                    if (err) sprintf(err, "Use private key file [%s] error.", ssh_key_file_path);
                    SSL_CTX_free(context);
                    return NULL;
                }
            }

            if (SSL_CTX_check_private_key(context) == 0) {
                if (err) sprintf(err, "Check private key file [%s] error.", ssh_key_file_path);
                SSL_CTX_free(context);
                return NULL;
            }
        } else if (rsa_key_file_path && rsa_key_file_path[0] != 0) {
            if (file_type == SLICE_SSL_FILE_TYPE_PEM) {
                if ((err_num = SSL_CTX_use_RSAPrivateKey_file(context, rsa_key_file_path, SSL_FILETYPE_PEM)) <= 0) {
                    if (err) sprintf(err, "Use RSA private key file [%s] error.", rsa_key_file_path);
                    SSL_CTX_free(context);
                    return NULL;
                }
            } else {
                if ((err_num = SSL_CTX_use_RSAPrivateKey_file(context, rsa_key_file_path, SSL_FILETYPE_ASN1)) <= 0) {
                    if (err) sprintf(err, "Use RSA private key file [%s] error.", rsa_key_file_path);
                    SSL_CTX_free(context);
                    return NULL;
                }
            }

            if (SSL_CTX_check_private_key(context) == 0) {
                if (err) sprintf(err, "Check private key file [%s] error.", rsa_key_file_path);
                SSL_CTX_free(context);
                return NULL;
            }
        } else {
            if (err) sprintf(err, "SSL server context need private key file");
            SSL_CTX_free(context);
            return NULL;
        }
    }
    
    SSL_CTX_set_options(context, SSL_OP_ALL);

    return context;
}

SliceReturnType slice_ssl_context_destroy(SliceSSLContext *context, char *err)
{
    if (!context) {
        if (err) sprintf(err, "Invalid parameter");
        return SLICE_RETURN_ERROR;
    }

    SSL_CTX_free(context);

    return SLICE_RETURN_NORMAL;
}