/*
 * auth.h - Handle SiriDB authentication.
 *
 * author       : Jeroen van der Heijden
 * email        : jeroen@transceptor.technology
 * copyright    : 2016, Transceptor Technology
 *
 * changes
 *  - initial version, 10-03-2016
 *
 */
#pragma once

#include <stddef.h>
#include <msgpack.h>
#include <siri/net/clserver.h>
#include <qpack/qpack.h>

enum sirinet_msg_tp;

enum sirinet_msg_tp siridb_auth_request(
        uv_handle_t * client,
        qp_obj_t * qp_username,
        qp_obj_t * qp_password,
        qp_obj_t * qp_dbname);


