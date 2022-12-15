/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/config.h"

#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"

#include <inttypes.h>
#include <string.h>
#include <time.h>

/**
 * Private declarations
 */

static const char *mympd_getenv(const char *env_var, bool first_startup);
static sds mympd_getenv_string(const char *env_var, const char *default_value, validate_callback vcb, bool first_startup);
static int mympd_getenv_int(const char *env_var, int default_value, int min, int max, bool first_startup);
static bool mympd_getenv_bool(const char *env_var, bool default_value, bool first_startup);

/**
 * Public functions
 */

/**
 * Frees the config struct
 * @param config pointer to config struct
 */
void *mympd_free_config(struct t_config *config) {
    FREE_SDS(config->http_host);
    FREE_SDS(config->ssl_cert);
    FREE_SDS(config->ssl_key);
    FREE_SDS(config->ssl_san);
    FREE_SDS(config->acl);
    FREE_SDS(config->scriptacl);
    FREE_SDS(config->lualibs);
    FREE_SDS(config->pin_hash);
    FREE_SDS(config->user);
    FREE_SDS(config->workdir);
    FREE_SDS(config->cachedir);
    FREE_PTR(config);
    return NULL;
}

/**
 * Sets the initial default values for config struct
 * This function is used before reading command line arguments
 * @param config pointer to config struct
 */
void mympd_config_defaults_initial(struct t_config *config) {
    //command line options
    config->user = sdsnew(CFG_MYMPD_USER);
    config->workdir = sdsnew(MYMPD_WORK_DIR);
    config->cachedir = sdsnew(MYMPD_CACHE_DIR);
    config->log_to_syslog = CFG_LOG_TO_SYSLOG;
    //not configurable
    config->startup_time = time(NULL);
    config->first_startup = false;
    config->bootstrap = false;
    //set all other sds strings to NULL
    config->http_host = NULL;
    config->http_port = CFG_MYMPD_HTTP_PORT;
    config->ssl = CFG_MYMPD_SSL;
    config->ssl_port = CFG_MYMPD_SSL_PORT;
    config->ssl_san = NULL;
    config->ssl_cert = NULL;
    config->ssl_key = NULL;
    config->acl = NULL;
    config->scriptacl = NULL;
    config->lualibs = NULL;
    config->pin_hash = NULL;
    config->covercache_keep_days = CFG_COVERCACHE_KEEP_DAYS;
    config->save_caches = true;
}

/**
 * Sets the default values for config struct
 * This function is used after reading command line arguments and
 * reads the environment variables
 * @param config pointer to config struct
 */
void mympd_config_defaults(struct t_config *config) {
    if (config->first_startup == true) {
        MYMPD_LOG_INFO("Reading environment variables");
    }
    //configurable with environment variables at first startup
    config->http_host = mympd_getenv_string("MYMPD_HTTP_HOST", CFG_MYMPD_HTTP_HOST, vcb_isname, config->first_startup);
    config->http_port = mympd_getenv_int("MYMPD_HTTP_PORT", CFG_MYMPD_HTTP_PORT, 0, MPD_PORT_MAX, config->first_startup);
    #ifdef MYMPD_ENABLE_SSL
        config->ssl = mympd_getenv_bool("MYMPD_SSL", CFG_MYMPD_SSL, config->first_startup);
        config->ssl_port = mympd_getenv_int("MYMPD_SSL_PORT", CFG_MYMPD_SSL_PORT, 0, MPD_PORT_MAX, config->first_startup);
        config->ssl_san = mympd_getenv_string("MYMPD_SSL_SAN", CFG_MYMPD_SSL_SAN, vcb_isname, config->first_startup);
        config->custom_cert = mympd_getenv_bool("MYMPD_CUSTOM_CERT", CFG_MYMPD_CUSTOM_CERT, config->first_startup);
        sds default_cert = sdscatfmt(sdsempty(), "%S/ssl/server.pem", config->workdir);
        sds default_key = sdscatfmt(sdsempty(), "%S/ssl/server.key", config->workdir);
        if (config->custom_cert == true) {
            config->ssl_cert = mympd_getenv_string("MYMPD_SSL_CERT", default_cert, vcb_isfilepath, config->first_startup);
            config->ssl_key = mympd_getenv_string("MYMPD_SSL_KEY", default_key, vcb_isfilepath, config->first_startup);
            FREE_SDS(default_cert);
            FREE_SDS(default_key);
        }
        else {
            config->ssl_cert = default_cert;
            config->ssl_key = default_key;
        }
    #else
        config->ssl = false;
        config->ssl_port = 0;
        config->ssl_san = sdsempty();
        config->custom_cert = sdsempty();
        config->ssl_cert = sdsempty();
        config->ssl_key = sdsempty();
    #endif
    config->acl = mympd_getenv_string("MYMPD_ACL", CFG_MYMPD_ACL, vcb_isname, config->first_startup);
    config->scriptacl = mympd_getenv_string("MYMPD_SCRIPTACL", CFG_MYMPD_SCRIPTACL, vcb_isname, config->first_startup);
    #ifdef MYMPD_ENABLE_LUA
        config->lualibs = mympd_getenv_string("MYMPD_LUALIBS", CFG_MYMPD_LUALIBS, vcb_isalnum, config->first_startup);
    #else
        config->lualibs = sdsempty();
    #endif
    config->loglevel = CFG_MYMPD_LOGLEVEL;
    config->pin_hash = sdsnew(CFG_MYMPD_PIN_HASH);
    config->covercache_keep_days = mympd_getenv_int("MYMPD_COVERCACHE_KEEP_DAYS", CFG_COVERCACHE_KEEP_DAYS, COVERCACHE_AGE_MIN, COVERCACHE_AGE_MAX, config->first_startup);
    config->save_caches = mympd_getenv_bool("MYMPD_SAVE_CACHES", CFG_MYMPD_SSL, config->save_caches);
}

/**
 * Reads or writes the config from the /var/lib/mympd/config directory
 * @param config pointer to config struct
 */
bool mympd_read_config(struct t_config *config) {
    config->http_host = state_file_rw_string_sds(config->workdir, "config", "http_host", config->http_host, vcb_isname, false);
    config->http_port = state_file_rw_int(config->workdir, "config", "http_port", config->http_port, 0, MPD_PORT_MAX, false);

    #ifdef MYMPD_ENABLE_SSL
        config->ssl = state_file_rw_bool(config->workdir, "config", "ssl", config->ssl, false);
        config->ssl_port = state_file_rw_int(config->workdir, "config", "ssl_port", config->ssl_port, 0, MPD_PORT_MAX, false);
        config->ssl_san = state_file_rw_string_sds(config->workdir, "config", "ssl_san", config->ssl_san, vcb_isname, false);
        config->custom_cert = state_file_rw_bool(config->workdir, "config", "custom_cert", config->custom_cert, false);
        if (config->custom_cert == true) {
            config->ssl_cert = state_file_rw_string_sds(config->workdir, "config", "ssl_cert", config->ssl_cert, vcb_isname, false);
            config->ssl_key = state_file_rw_string_sds(config->workdir, "config", "ssl_key", config->ssl_key, vcb_isname, false);
        }
        config->pin_hash = state_file_rw_string_sds(config->workdir, "config", "pin_hash", config->pin_hash, vcb_isname, false);
    #else
        MYMPD_LOG_NOTICE("OpenSSL is disabled, ignoring ssl and pin settings");
    #endif
    config->acl = state_file_rw_string_sds(config->workdir, "config", "acl", config->acl, vcb_isname, false);
    config->scriptacl = state_file_rw_string_sds(config->workdir, "config", "scriptacl", config->scriptacl, vcb_isname, false);
    #ifdef MYMPD_ENABLE_LUA
        config->lualibs = state_file_rw_string_sds(config->workdir, "config", "lualibs", config->lualibs, vcb_isname, false);
    #else
        MYMPD_LOG_NOTICE("Lua is disabled, ignoring lua settings");
    #endif
    config->covercache_keep_days = state_file_rw_int(config->workdir, "config", "covercache_keep_days", config->covercache_keep_days, COVERCACHE_AGE_MIN, COVERCACHE_AGE_MAX, false);
    config->loglevel = state_file_rw_int(config->workdir, "config", "loglevel", config->loglevel, LOGLEVEL_MIN, LOGLEVEL_MAX, false);
    config->save_caches = state_file_rw_bool(config->workdir, "config", "save_caches", config->save_caches, false);
    //overwrite configured loglevel
    config->loglevel = mympd_getenv_int("MYMPD_LOGLEVEL", config->loglevel, LOGLEVEL_MIN, LOGLEVEL_MAX, true);
    return true;
}

/**
 * Private functions
 */

/**
 * Reads environment variables on first startup
 * @param env_var variable name to read
 * @param first_startup true for first startup else false
 * @return environment value or NULL
 */
static const char *mympd_getenv(const char *env_var, bool first_startup) {
    const char *env_value = getenv_check(env_var, 100);
    if (first_startup == true) {
        return env_value;
    }
    if (env_value != NULL) {
        MYMPD_LOG_INFO("Ignoring environment variable \"%s\" with value \"%s\"", env_var, env_value);
    }
    return NULL;
}

/**
 * Gets an environment variable as sds string
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param vcb validation callback
 * @param first_startup true for first startup else false
 * @return environment variable as sds string
 */
static sds mympd_getenv_string(const char *env_var, const char *default_value, validate_callback vcb, bool first_startup) {
    const char *env_value = mympd_getenv(env_var, first_startup);
    if (env_value == NULL) {
        return sdsnew(default_value);
    }
    sds value = sdsnew(env_value);
    if (vcb == NULL ||
        vcb(value) == true)
    {
        return value;
    }
    FREE_SDS(value);
    return sdsnew(default_value);
}

/**
 * Gets an environment variable as int
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param first_startup true for first startup else false
 * @return environment variable as integer
 */
static int mympd_getenv_int(const char *env_var, int default_value, int min, int max, bool first_startup) {
    const char *env_value = mympd_getenv(env_var, first_startup);
    if (env_value == NULL) {
        return default_value;
    }
    int value = (int)strtoimax(env_value, NULL, 10);
    if (value >= min && value <= max) {
        return value;
    }
    MYMPD_LOG_WARN("Invalid value for \"%s\" using default", env_var);
    return default_value;
}

/**
 * Gets an environment variable as bool
 * @param env_var variable name to read
 * @param default_value default value if variable is not set
 * @param vcb validation callback
 * @param first_startup true for first startup else false
 * @return environment variable as bool
 */
static bool mympd_getenv_bool(const char *env_var, bool default_value, bool first_startup) {
    const char *env_value = mympd_getenv(env_var, first_startup);
    return env_value != NULL ? strcmp(env_value, "true") == 0 ? true : false
                             : default_value;
}
