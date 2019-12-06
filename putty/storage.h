/*
 * storage.h: interface defining functions for storage and recovery
 * of PuTTY's persistent data.
 */

#ifndef PUTTY_STORAGE_H
#define PUTTY_STORAGE_H

const int DATA_VERSION = 2;
/* ----------------------------------------------------------------------
 * Functions to access PuTTY's random number seed file.
 */

typedef void (*noise_consumer_t) (void *data, int len);

class IStore
{
public:
    virtual ~IStore() {};
    /* ----------------------------------------------------------------------
     * Functions to save and restore PuTTY sessions. Note that this is
     * only the low-level code to do the reading and writing. The
     * higher-level code that translates a Config structure into a set
     * of (key,value) pairs is elsewhere, since it doesn't (mostly)
     * change between platforms.
     */

    /*
     * Write a saved session. The caller is expected to call
     * open_setting_w() to get a `void *' handle, then pass that to a
     * number of calls to write_setting_s() and write_setting_i(), and
     * then close it using close_settings_w(). At the end of this call
     * sequence the settings should have been written to the PuTTY
     * persistent storage area.
     *
     * A given key will be written at most once while saving a session.
     * Keys may be up to 255 characters long.  String values have no length
     * limit.
     *
     * Any returned error message must be freed after use.
     */
    virtual void *open_global_settings() = 0;
    virtual void *open_settings_w(const char *sessionname, char **errmsg) = 0;
    virtual void write_setting_s(void *handle, const char *key, const char *value) = 0;
    virtual void write_setting_i(void *handle, const char *key, int value) = 0;
    virtual void write_setting_filename(void *handle, const char *key, Filename* value) = 0;
    virtual void write_setting_fontspec(void *handle, const char *key, FontSpec* font) = 0;
    virtual void close_settings_w(void *handle) = 0;

    /*
     * Read a saved session. The caller is expected to call
     * open_setting_r() to get a `void *' handle, then pass that to a
     * number of calls to read_setting_s() and read_setting_i(), and
     * then close it using close_settings_r().
     *
     * read_setting_s() writes into the provided buffer and returns a
     * pointer to the same buffer.
     *
     * If a particular string setting is not present in the session,
     * read_setting_s() can return NULL, in which case the caller
     * should invent a sensible default. If an integer setting is not
     * present, read_setting_i() returns its provided default.
     *
     * read_setting_filename() and read_setting_fontspec() each read into
     * the provided buffer, and return zero if they failed to.
     */

    virtual void *open_settings_r(const char *sessionname) = 0;
    virtual char *read_setting_s(void *handle, const char *key, char *buffer, int buflen) = 0;
    //virtual int open_read_settings_s(const char *key, const char *subkey, char *buffer, int buflen) = 0;
    virtual int read_setting_i(void *handle, const char *key, int defvalue) = 0;
    virtual Filename* read_setting_filename(void *handle, const char *key) = 0;
    virtual FontSpec *read_setting_fontspec(void *handle, const char *key) = 0;
    virtual void close_settings_r(void *handle) = 0;

    /*
     * Delete a whole saved session.
     */
    virtual void del_settings(const char *sessionname) = 0;
    virtual void del_settings_only(const char *sessionname)
    {
        del_settings(sessionname);
    };

    /*
     * Enumerate all saved sessions.
     */
    virtual void *enum_settings_start(void) = 0;
    virtual char *enum_settings_next(void *handle, char *buffer, int buflen) = 0;
    virtual void enum_settings_finish(void *handle) = 0;

    virtual void *enum_cmd_start(void)
    {
        return NULL;
    };
    virtual char *enum_cmd_next(void *handle, char *buffer, int buflen)
    {
        return NULL;
    };
    virtual void enum_cmd_finish(void *handle)
    {
        return;
    };

    virtual void del_cmd_settings(const char *cmd_name) {};

    /* ----------------------------------------------------------------------
     * Functions to access PuTTY's host key database.
     */

    /*
     * See if a host key matches the database entry. Return values can
     * be 0 (entry matches database), 1 (entry is absent in database),
     * or 2 (entry exists in database and is different).
     */
    virtual int verify_host_key(const char *hostname, int port,
                                const char *keytype, const char *key) = 0;

    /*
     * Write a host key into the database, overwriting any previous
     * entry that might have been there.
     */
    virtual void store_host_key(const char *hostname, int port,
                                const char *keytype, const char *key) = 0;


    /*
     * Read PuTTY's random seed file and pass its contents to a noise
     * consumer function.
     */
    virtual void read_random_seed(noise_consumer_t consumer) = 0;

    /*
     * Write PuTTY's random seed file from a given chunk of noise.
     */
    virtual void write_random_seed(void *data, int len) = 0;

    /* ----------------------------------------------------------------------
     * Cleanup function: remove all of PuTTY's persistent state.
     */
    virtual void cleanup_all(void) = 0;

    virtual void load_settings_to_tree234(const char *sessionname, tree234 *storage_tree) {}

    char* read_setting_s(void* handle, const char* key)
    {
        char buffer[1024] = {0};
        char* ret = read_setting_s(handle, key, buffer, sizeof(buffer));
        return ret ? dupstr(buffer) : NULL;
    }

};
extern IStore* gStorage;

class WinRegStore: public IStore
{
public:
    static int open_read_settings_s(const char *key, const char *subkey, char *buffer, int buflen);

    virtual void *open_global_settings();
    virtual void *open_settings_w(const char *sessionname, char **errmsg) ;
    virtual void write_setting_s(void *handle, const char *key, const char *value) ;
    virtual void write_setting_i(void *handle, const char *key, int value) ;
    virtual void write_setting_filename(void *handle, const char *key, Filename* value) ;
    virtual void write_setting_fontspec(void *handle, const char *key, FontSpec* font) ;
    virtual void close_settings_w(void *handle) ;

    virtual void *open_settings_r(const char *sessionname) ;
    virtual char *read_setting_s(void *handle, const char *key, char *buffer, int buflen) ;
    //virtual int open_read_settings_s(const char *key, const char *subkey, char *buffer, int buflen) ;
    virtual int read_setting_i(void *handle, const char *key, int defvalue) ;
    virtual Filename *read_setting_filename(void *handle, const char *key) ;
    virtual FontSpec *read_setting_fontspec(void *handle, const char *key) ;
    virtual void close_settings_r(void *handle) ;

    virtual void del_settings(const char *sessionname);
    virtual void del_settings_only(const char *sessionname);

    virtual void *enum_settings_start(void) ;
    virtual char *enum_settings_next(void *handle, char *buffer, int buflen) ;
    virtual void enum_settings_finish(void *handle) ;

    virtual void *enum_cmd_start(void);
    virtual char *enum_cmd_next(void *handle, char *buffer, int buflen);
    virtual void enum_cmd_finish(void *handle);
    virtual void del_cmd_settings(const char *cmd_name);

    virtual int verify_host_key(const char *hostname, int port,
                                const char *keytype, const char *key) ;

    virtual void store_host_key(const char *hostname, int port,
                                const char *keytype, const char *key) ;

    virtual void read_random_seed(noise_consumer_t consumer) ;

    virtual void write_random_seed(void *data, int len) ;

    virtual void cleanup_all(void) ;

    virtual void load_settings_to_tree234(const char *sessionname, tree234 *storage_tree);

    static void mungestr(const char *in, char *out);
    static void unmungestr(const char *in, char *out, int outlen);
};

class FileStore: public IStore
{
public:
    FileStore()
    {
        *pathM = 0;
    }
    FileStore(const char* thePath)
    {
        strncpy(pathM, thePath, sizeof(pathM));
    }

    virtual void *open_global_settings()
    {
        return NULL;
    };
    virtual void *open_settings_w(const char *sessionname, char **errmsg) ;
    virtual void write_setting_s(void *handle, const char *key, const char *value) ;
    virtual void write_setting_i(void *handle, const char *key, int value) ;
    virtual void write_setting_filename(void *handle, const char *key, Filename* value) ;
    virtual void write_setting_fontspec(void *handle, const char *key, FontSpec* font) ;
    virtual void close_settings_w(void *handle) ;

    virtual void *open_settings_r(const char *sessionname) ;
    virtual char *read_setting_s(void *handle, const char *key, char *buffer, int buflen) ;
    //virtual int open_read_settings_s(const char *key, const char *subkey, char *buffer, int buflen) ;
    virtual int read_setting_i(void *handle, const char *key, int defvalue) ;
    virtual Filename *read_setting_filename(void *handle, const char *key) ;
    virtual FontSpec *read_setting_fontspec(void *handle, const char *key) ;
    virtual void close_settings_r(void *handle) ;

    virtual void del_settings(const char *sessionname) ;

    virtual void *enum_settings_start(void) ;
    virtual char *enum_settings_next(void *handle, char *buffer, int buflen) ;
    virtual void enum_settings_finish(void *handle) ;

    virtual int verify_host_key(const char *hostname, int port,
                                const char *keytype, const char *key) ;

    virtual void store_host_key(const char *hostname, int port,
                                const char *keytype, const char *key) ;

    virtual void read_random_seed(noise_consumer_t consumer) ;

    virtual void write_random_seed(void *data, int len) ;

    virtual void cleanup_all(void) ;

    static char *mungestr(const char *in);
    static char *unmungestr(const char *in);

private:
    char *make_filename(int index, const char *subname);

    char pathM[256];
};

class MemStore : public IStore
{
public:
    MemStore() { }
    void input(const char* input);

    virtual void *open_global_settings()
    {
        return NULL;
    };
    virtual void *open_settings_w(const char *sessionname, char **errmsg);
    virtual void write_setting_s(void *handle, const char *key, const char *value);
    virtual void write_setting_i(void *handle, const char *key, int value);
    virtual void write_setting_filename(void *handle, const char *key, Filename* value);
    virtual void write_setting_fontspec(void *handle, const char *key, FontSpec* font);
    virtual void close_settings_w(void *handle);

    virtual void *open_settings_r(const char *sessionname);
    virtual char *read_setting_s(void *handle, const char *key, char *buffer, int buflen);
    //virtual int open_read_settings_s(const char *key, const char *subkey, char *buffer, int buflen) ;
    virtual int read_setting_i(void *handle, const char *key, int defvalue);
    virtual Filename *read_setting_filename(void *handle, const char *key);
    virtual FontSpec *read_setting_fontspec(void *handle, const char *key);
    virtual void close_settings_r(void *handle);

    virtual void del_settings(const char *sessionname);

    virtual void *enum_settings_start(void);
    virtual char *enum_settings_next(void *handle, char *buffer, int buflen);
    virtual void enum_settings_finish(void *handle);

    virtual int verify_host_key(const char *hostname, int port,
                                const char *keytype, const char *key);

    virtual void store_host_key(const char *hostname, int port,
                                const char *keytype, const char *key);

    virtual void read_random_seed(noise_consumer_t consumer);

    virtual void write_random_seed(void *data, int len);

    virtual void cleanup_all(void);

    static char *mungestr(const char *in);
    static char *unmungestr(const char *in);

private:
    const char* inputM;
};

class TmplStore : public IStore
{
public:
    TmplStore(IStore* impl)
        : implStorageM(impl)
    { }

    virtual void *open_global_settings()
    {
        return NULL;
    };
    virtual void *open_settings_w(const char *sessionname, char **errmsg);
    virtual void write_setting_s(void *handle, const char *key, const char *value);
    virtual void write_setting_i(void *handle, const char *key, int value);
    virtual void write_setting_filename(void *handle, const char *key, Filename* value);
    virtual void write_setting_fontspec(void *handle, const char *key, FontSpec* font);
    virtual void close_settings_w(void *handle);

    virtual void *open_settings_r(const char *sessionname);
    virtual char *read_setting_s(void *handle, const char *key, char *buffer, int buflen);
    //virtual int open_read_settings_s(const char *key, const char *subkey, char *buffer, int buflen) ;
    virtual int read_setting_i(void *handle, const char *key, int defvalue);
    virtual Filename *read_setting_filename(void *handle, const char *key);
    virtual FontSpec *read_setting_fontspec(void *handle, const char *key);
    virtual void close_settings_r(void *handle);

    virtual void del_settings(const char *sessionname);
    virtual void del_settings_only(const char *sessionname)
    {
        implStorageM->del_settings_only(sessionname);
    }

    virtual void *enum_settings_start(void);
    virtual char *enum_settings_next(void *handle, char *buffer, int buflen);
    virtual void enum_settings_finish(void *handle);

    virtual void *enum_cmd_start(void)
    {
        return implStorageM->enum_cmd_start();
    }
    virtual char *enum_cmd_next(void *handle, char *buffer, int buflen)
    {
        return implStorageM->enum_cmd_next(handle, buffer, buflen);
    }
    virtual void enum_cmd_finish(void *handle)
    {
        return implStorageM->enum_cmd_finish(handle);
    }
    virtual void del_cmd_settings(const char *cmd_name)
    {
        return implStorageM->del_cmd_settings(cmd_name);
    };

    virtual int verify_host_key(const char *hostname, int port,
                                const char *keytype, const char *key);

    virtual void store_host_key(const char *hostname, int port,
                                const char *keytype, const char *key);

    virtual void read_random_seed(noise_consumer_t consumer);

    virtual void write_random_seed(void *data, int len);

    virtual void cleanup_all(void);

    char* load_ssetting(const char *section, char* setting, const char* def);

private:
    IStore* implStorageM;
};

struct skeyval
{
    const char *key;
    const char *value;
};

static inline int keycmp(void *av, void *bv)
{
    struct skeyval *a = (struct skeyval *)av;
    struct skeyval *b = (struct skeyval *)bv;
    return strcmp(a->key, b->key);
}

#include <map>
extern std::map<std::string, int> DEFAULT_INT_VALUE;
extern std::map<std::string, std::string> DEFAULT_STR_VALUE;
static inline bool is_default_value(const char* key, const char* value)
{
    std::map<std::string, std::string>::iterator it = DEFAULT_STR_VALUE.find(key);
    if (it == DEFAULT_STR_VALUE.end())
    {
        return false;
    }
    return strcmp(value, it->second.c_str()) == 0;
}

static inline bool is_default_value(const char* key, const int value)
{
    std::map<std::string, int>::iterator it = DEFAULT_INT_VALUE.find(key);
    if (it == DEFAULT_INT_VALUE.end())
    {
        return false;
    }
    return value == it->second;
}

#endif
