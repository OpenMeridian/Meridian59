# Meridian 59 Server - Linux Build Instructions

## Prerequisites (Ubuntu/Debian)

```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install make gcc-multilib g++-multilib flex bison \
    libjansson-dev libjansson-dev:i386 \
    libpq-dev libpq-dev:i386 \
    python3
```

## Build

Build everything (server, compiler, KOD, resources, utilities):

```bash
make -f makefile.linux
```

This builds:
- `blakserv` - the game server (copied to `run/server/`)
- `bc` - the KOD compiler (copied to `bin/`)
- `rscmerge` - resource merge tool (copied to `bin/`)
- All `.bof`, `.rsc`, `.rsb` files (copied to `run/server/rsc/` and `run/server/memmap/`)
- Room files (copied to `run/server/rooms/`)

To clean all build artifacts:

```bash
make -f makefile.linux clean
```

## Configuration

```bash
cd run/server
cp blakserv.cfg-linux blakserv.cfg
```

Edit `blakserv.cfg` as needed. Key settings:

```ini
[Socket]
Port                 5959
MaintenancePort      9998
MaintenanceMask      ::ffff:127.0.0.1;::1

[Resource]
Language             1

[MySQL]
Enabled     No
```

Set `[MySQL] Enabled Yes` if using a database (see Database section below).

## Run

```bash
cd run/server
./blakserv          # foreground (Ctrl+C to stop)
./blakserv &        # background
```

Default ports: **5959** (game), **9998** (admin maintenance)

## Admin Commands

Via the admin script:

```bash
./blakadmin.sh show status
./blakadmin.sh show accounts
./blakadmin.sh who
./blakadmin.sh save game
./blakadmin.sh garbage
./blakadmin.sh "send o 0 updatedatabase"
./blakadmin.sh create account admin username password email
./blakadmin.sh terminate save
```

Interactive mode:

```bash
./blakadmin.sh
blakadm> show status
blakadm> bye
```

Or via telnet/netcat directly:

```bash
telnet 127.0.0.1 9998
echo "show status" | nc 127.0.0.1 9998
```

## Logs

```bash
tail -f run/server/channel/*.txt
```

Files: `debug.txt`, `error.txt`, `log.txt`, `god.txt`, `admin.txt`

## Database (Optional)

The server can optionally log game statistics (player logins, deaths, damage, money,
wiki data, etc.) to a PostgreSQL database. This is not required for the server to run.

To enable, install PostgreSQL and configure:

```ini
[MySQL]
Enabled     Yes
Host        127.0.0.1
Port        5432
Username    blakserv
Password    your_password
Database    meridian59
```

Note: The config section is named `[MySQL]` for compatibility with the Windows version,
but on Linux it connects to PostgreSQL via libpq. The Windows Win32 build uses MySQL
and normally needs port 3306; the Windows x64 build uses PostgreSQL.

The SSL settings are PostgreSQL-only. If unset, libpq normally uses `sslmode=prefer`,
which does not verify the server and can fall back to an unencrypted connection.
For verified TLS, configure the PostgreSQL server for TLS, set `Host` to a name or IP
address that matches its certificate, and add these settings under `[MySQL]`:

```ini
SSLMode     verify-full
SSLRootCert </etc/postgresql/certs/ca.crt>
```

The CA file must exist on the client. If the server requires a client certificate,
also supply the paths to that certificate and its private key:

```ini
SSLCert     </etc/postgresql/certs/client.crt>
SSLKey      </etc/postgresql/certs/client.key>
```

The server creates all required tables automatically on first connect.

## Architecture

Single-threaded epoll main loop with a separate PostgreSQL writer thread
for async database operations.

Key Linux-specific files:
- `blakserv/osd_linux.c/h` - OS-dependent types and stubs
- `blakserv/osd_epoll.c` - Main loop (epoll socket multiplexing + timer wakeup via eventfd)
- `blakserv/database_pg.c/h` - PostgreSQL database layer (replaces MySQL)
- `blakserv/main.c` - Unified Windows/Linux main with `#ifdef`
- `util/rscmerge.c` - Resource merge with deterministic file sorting
