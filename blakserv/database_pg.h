// Meridian 59 - PostgreSQL database implementation for Linux
// Replaces MySQL database.h/database.c with libpq-based PostgreSQL support.
// Same public API as the MySQL version for seamless integration.

#ifndef _DATABASE_PG_H
#define _DATABASE_PG_H

#include <libpq-fe.h>

#include "database_common.h"
#include "database_procedures.h"

typedef struct {
	const char *host;
	int port;
	const char *username;
	const char *password;
	const char *database;
	const char *ssl_mode;
	const char *ssl_root_cert;
	const char *ssl_cert;
	const char *ssl_key;
} postgres_connection_options;

void PostgreSQLInit(const postgres_connection_options *options);

#endif
