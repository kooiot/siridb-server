/*
 * tee.c - To tee the data for a SiriDB database.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <siri/db/tee.h>
#include <siri/siri.h>
#include <siri/net/pipe.h>
#include <logger/logger.h>

#define TEE__BUF_SZ 512
static char tee__buf[TEE__BUF_SZ];

static void tee__runtime_init(uv_pipe_t * pipe);
static void tee__write_cb(uv_write_t * req, int status);
static void tee__on_connect(uv_connect_t * req, int status);
static void tee__close_cb(uv_pipe_t * pipe);
static void tee__alloc_buffer(
    uv_handle_t * handle,
    size_t suggsz,
    uv_buf_t * buf);
static void tee__on_data(
    uv_stream_t * client,
    ssize_t nread,
    const uv_buf_t * buf);

siridb_tee_t * siridb_tee_new(void)
{
    siridb_tee_t * tee = malloc(sizeof(siridb_tee_t));
    if (tee == NULL)
    {
        return NULL;
    }
    tee->pipe_name_ = NULL;
    tee->err_msg_ = NULL;
    tee->pipe.data = tee;
    tee->flags = SIRIDB_TEE_FLAG;
    return tee;
}

void siridb_tee_free(siridb_tee_t * tee)
{
    free(tee->err_msg_);
    free(tee->pipe_name_);
    free(tee);
}

int siridb_tee_connect(siridb_tee_t * tee)
{
    if (tee->flags & SIRIDB_TEE_FLAG_CONNECTING)
    {
        return 0;
    }
    tee->flags |= SIRIDB_TEE_FLAG_CONNECTING;
    uv_connect_t * req = malloc(sizeof(uv_connect_t));
    if (req == NULL)
    {
        return -1;
    }

    req->data = tee;

    if (uv_pipe_init(siri.loop, &tee->pipe, 0))
    {
        tee->flags &= ~SIRIDB_TEE_FLAG_CONNECTING;
        free(req);
        return -1;
    }
    tee->flags |= SIRIDB_TEE_FLAG_INIT;
    tee->pipe.data = tee;

    uv_pipe_connect(req, &tee->pipe, tee->pipe_name_, tee__on_connect);
    return 0;
}

int siridb_tee_set_pipe_name(siridb_tee_t * tee, const char * pipe_name)
{
    free(tee->pipe_name_);
    free(tee->err_msg_);
    tee->err_msg_ = NULL;

    if (pipe_name == NULL)
    {
        tee->pipe_name_ = NULL;

        if (tee->flags & SIRIDB_TEE_FLAG_CONNECTED)
        {
            uv_close((uv_handle_t *) &tee->pipe, (uv_close_cb) tee__close_cb);
        }
        return 0;
    }

    tee->pipe_name_ = strdup(pipe_name);
    if (!tee->pipe_name_)
    {
        return -1;
    }
    if (tee->flags & SIRIDB_TEE_FLAG_INIT)
    {
        uv_close((uv_handle_t *) &tee->pipe, (uv_close_cb) tee__runtime_init);
    }
    else
    {
        tee__runtime_init(&tee->pipe);
    }
    return 0;
}

void siridb_tee_write(siridb_tee_t * tee, sirinet_promise_t * promise)
{
    uv_write_t * req = malloc(sizeof(uv_write_t));
    if (!req)
    {
        log_error("Cannot allocate memory for tee request");
        return;
    }

    req->data = promise;
    sirinet_promise_incref(promise);

    uv_buf_t wrbuf = uv_buf_init(
            (char *) promise->pkg,
            sizeof(sirinet_pkg_t) + promise->pkg->len);

    if (uv_write(req, (uv_stream_t *) &tee->pipe, &wrbuf, 1, tee__write_cb))
    {
        log_error("Cannot write to tee");
        sirinet_promise_decref(promise);
    }
}

const char * tee_str(siridb_tee_t * tee)
{
    if (tee->err_msg_)
    {
        return tee->err_msg_;
    }
    if (tee->pipe_name_)
    {
        return tee->pipe_name_;
    }
    return "disabled";
}


static void tee__runtime_init(uv_pipe_t * pipe)
{
    siridb_tee_t * tee = pipe->data;

    tee->flags &= ~SIRIDB_TEE_FLAG_INIT;
    tee->flags &= ~SIRIDB_TEE_FLAG_CONNECTING;
    tee->flags &= ~SIRIDB_TEE_FLAG_CONNECTED;

     if (siridb_tee_connect(tee))
     {
         log_error("Could not connect to tee at runtime");
     }
}

static void tee__close_cb(uv_pipe_t * pipe)
{
    siridb_tee_t * tee = pipe->data;

    tee->flags &= ~SIRIDB_TEE_FLAG_INIT;
    tee->flags &= ~SIRIDB_TEE_FLAG_CONNECTING;
    tee->flags &= ~SIRIDB_TEE_FLAG_CONNECTED;
}

static void tee__write_cb(uv_write_t * req, int status)
{
    sirinet_promise_t * promise = req->data;
    sirinet_promise_decref(promise);
    if (status)
    {
        log_error("Socket (tee) write error: %s", uv_strerror(status));
    }
    free(req);
}

static void tee__on_connect(uv_connect_t * req, int status)
{
    siridb_tee_t * tee = req->data;

    if (status == 0)
    {
        log_info("Connection created to pipe: '%s'", tee->pipe_name_);
        if (uv_read_start(req->handle, tee__alloc_buffer, tee__on_data))
        {
            free(tee->err_msg_);
            if (asprintf(&tee->err_msg_,
                    "Cannot open pipe '%s' for reading",
                    tee->pipe_name_) >= 0)
            {
                log_warning(tee->err_msg_);
            }
            goto fail;
        }
        tee->flags |= SIRIDB_TEE_FLAG_CONNECTED;
        goto done;
    }

    free(tee->err_msg_);
    tee->err_msg_ = NULL;

    if (asprintf(
            &tee->err_msg_,
            "Cannot connect to pipe '%s' (%s)",
            tee->pipe_name_,
            uv_strerror(status)) >= 0)
    {
        log_warning(tee->err_msg_);
    }

fail:
    uv_close((uv_handle_t *) req->handle, (uv_close_cb) tee__close_cb);
done:
    free(req);
}

static void tee__alloc_buffer(
    uv_handle_t * handle __attribute__((unused)),
    size_t suggsz __attribute__((unused)),
    uv_buf_t * buf)
{
    buf->base = tee__buf;
    buf->len = TEE__BUF_SZ;
}



static void tee__on_data(
    uv_stream_t * client,
    ssize_t nread,
    const uv_buf_t * buf __attribute__((unused)))
{
    if (nread < 0)
    {
        if (nread != UV_EOF)
        {
            log_error("Read error on pipe '%s' : '%s'",
                sirinet_pipe_name((uv_pipe_t * ) client),
                uv_err_name(nread));
        }
        log_info("Disconnected from tee");
        uv_close((uv_handle_t *) client, (uv_close_cb) tee__close_cb);
    }

    if (nread > 0)
    {
        log_debug("Got %zd bytes on tee which will be ignored", nread);
    }
}
