/*
 * winstore.c: Windows-specific implementation of the interface
 * defined in storage.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "putty.h"
#include "storage.h"

#include <shlobj.h>
#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a
#endif
#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x001c
#endif

#ifdef WINNT
WinRegStore winRegStore;
extern IStore* gStorage = &winRegStore;
#endif

static const char *const reg_jumplist_key = PUTTY_REG_POS "\\Jumplist";
static const char *const reg_jumplist_value = "Recent sessions";
static const char *const puttystr = PUTTY_REG_POS "\\Sessions";
static const char *const global_settings_key = PUTTY_REG_POS "\\GlobalSettings";

static const char hex[17] = "0123456789ABCDEF";

static int tried_shgetfolderpath = FALSE;
static HMODULE shell32_module = NULL;
DECL_WINDOWS_FUNCTION(static, HRESULT, SHGetFolderPathA,
                      (HWND, int, HANDLE, DWORD, LPSTR));

void WinRegStore::mungestr(const char *in, char *out)
{
    int candot = 0;

    while (*in)
    {
        if (*in == ' ' || *in == '\\' || *in == '*' || *in == '?' ||
                *in == '%' || *in < ' ' || *in > '~' || (*in == '.'
                        && !candot))
        {
            *out++ = '%';
            *out++ = hex[((unsigned char) *in) >> 4];
            *out++ = hex[((unsigned char) *in) & 15];
        }
        else
            *out++ = *in;
        in++;
        candot = 1;
    }
    *out = '\0';
    return;
}

void WinRegStore::unmungestr(const char *in, char *out, int outlen)
{
    while (*in)
    {
        if (*in == '%' && in[1] && in[2])
        {
            int i, j;

            i = in[1] - '0';
            i -= (i > 9 ? 7 : 0);
            j = in[2] - '0';
            j -= (j > 9 ? 7 : 0);

            *out++ = (i << 4) + j;
            if (!--outlen)
                return;
            in += 3;
        }
        else
        {
            *out++ = *in++;
            if (!--outlen)
                return;
        }
    }
    *out = '\0';
    return;
}

void *WinRegStore::open_global_settings()
{
    HKEY sesskey;
    int ret;

    ret = RegCreateKey(HKEY_CURRENT_USER, global_settings_key, &sesskey);
    if (ret != ERROR_SUCCESS)
    {
        return NULL;
    }

    return (void *)sesskey;
}

void *WinRegStore::open_settings_w(const char *sessionname, char **errmsg)
{
    HKEY subkey1, sesskey;
    int ret;
    char *p;

    *errmsg = NULL;

    if (!sessionname || !*sessionname)
    {
        *errmsg = dupprintf("sessionname is empty\n");
        return NULL;
//	sessionname = DEFAULT_SESSION_NAME;
    }

    p = snewn(3 * strlen(sessionname) + 1, char);
    mungestr(sessionname, p);

    ret = RegCreateKey(HKEY_CURRENT_USER, puttystr, &subkey1);
    if (ret != ERROR_SUCCESS)
    {
        sfree(p);
        *errmsg = dupprintf("Unable to create registry key\n"
                            "HKEY_CURRENT_USER\\%s", puttystr);
        return NULL;
    }
    ret = RegCreateKey(subkey1, p, &sesskey);
    RegCloseKey(subkey1);
    if (ret != ERROR_SUCCESS)
    {
        *errmsg = dupprintf("Unable to create registry key\n"
                            "HKEY_CURRENT_USER\\%s\\%s", puttystr, p);
        sfree(p);
        return NULL;
    }
    sfree(p);
    return (void *) sesskey;
}

void WinRegStore::write_setting_s(void *handle, const char *key, const char *value)
{
    if (handle)
        RegSetValueEx((HKEY) handle, key, 0, REG_SZ, (BYTE*)value,
                      1 + strlen(value));
}

void WinRegStore::write_setting_i(void *handle, const char *key, int value)
{
    if (handle)
        RegSetValueEx((HKEY) handle, key, 0, REG_DWORD,
                      (CONST BYTE *) &value, sizeof(value));
}

void WinRegStore::close_settings_w(void *handle)
{
    RegCloseKey((HKEY) handle);
}

void *WinRegStore::open_settings_r(const char *sessionname)
{
    HKEY subkey1, sesskey;
    char *p;

    if (!sessionname || !*sessionname)
        sessionname = DEFAULT_SESSION_NAME;

    p = snewn(3 * strlen(sessionname) + 1, char);
    mungestr(sessionname, p);

    if (RegOpenKey(HKEY_CURRENT_USER, puttystr, &subkey1) != ERROR_SUCCESS)
    {
        sesskey = NULL;
    }
    else
    {
        if (RegOpenKey(subkey1, p, &sesskey) != ERROR_SUCCESS)
        {
            sesskey = NULL;
        }
        RegCloseKey(subkey1);
    }

    sfree(p);

    return (void *) sesskey;
}

char* winreg_user_read_setting_s(const char* path, const char* key, char*buffer, int buflen)
{
    HKEY subkey1;
    DWORD type, size;
    size = buflen;

    int ret = RegCreateKey(HKEY_CURRENT_USER, path, &subkey1);
    if (ret != ERROR_SUCCESS)
    {
        return NULL;
    }

    if (RegQueryValueEx(subkey1, key, 0,
                        &type, (BYTE*)buffer, &size) != ERROR_SUCCESS)
    {
        RegCloseKey(subkey1);
        return NULL;
    }
    RegCloseKey(subkey1);
    if (type != REG_SZ)
    {
        return NULL;
    }

    return buffer;

}

char *WinRegStore::read_setting_s(void *handle, const char *key, char *buffer, int buflen)
{
    DWORD type, size;
    size = buflen;

    if (!handle ||
            RegQueryValueEx((HKEY) handle, key, 0,
                            &type, (BYTE*)buffer, &size) != ERROR_SUCCESS ||
            type != REG_SZ) return NULL;
    else
        return buffer;
}

int WinRegStore::open_read_settings_s(const char *key, const char *subkey, char *buffer, int buflen)
{
    HKEY hkey;
//    char *p;

    if (RegOpenKey(HKEY_CURRENT_USER, key, &hkey) != ERROR_SUCCESS)
    {
        return -1;
    }

    DWORD type, size;
    size = buflen;

    if (RegQueryValueEx(hkey, subkey, 0,
                        &type, (BYTE*)buffer, &size) != ERROR_SUCCESS ||
            type != REG_SZ)
    {
        RegCloseKey(hkey);
        *buffer = 0;
        return -1;
    }

    RegCloseKey(hkey);

    return 0;
}

int WinRegStore::read_setting_i(void *handle, const char *key, int defvalue)
{
    DWORD type, val, size;
    size = sizeof(val);

    if (!handle ||
            RegQueryValueEx((HKEY) handle, key, 0, &type,
                            (BYTE *) &val, &size) != ERROR_SUCCESS ||
            size != sizeof(val) || type != REG_DWORD)
        return defvalue;
    else
        return val;
}

FontSpec * WinRegStore::read_setting_fontspec(void *handle, const char *name)
{
    char *settingname;
    char* font_name = NULL;
    int isbold = 0, charset = 0, height = 0;

    if (!(font_name = IStore::read_setting_s(handle, name)))
    {
        char *suffname = dupcat(name, "Name", NULL);
        if (font_name = IStore::read_setting_s(handle, suffname))
        {
            sfree(suffname);
        }
        else
        {
            sfree(suffname);
            return 0;
        }
    }
    settingname = dupcat(name, "IsBold", NULL);
    isbold = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (isbold == -1)
    {
        sfree(font_name);
        return 0;
    }

    settingname = dupcat(name, "CharSet", NULL);
    charset = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (charset == -1)
    {
        sfree(font_name);
        return 0;
    }

    settingname = dupcat(name, "Height", NULL);
    height = read_setting_i(handle, settingname, INT_MIN);
    sfree(settingname);
    if (height == INT_MIN)
    {
        sfree(font_name);
        return 0;
    }

    return fontspec_new(font_name, isbold, height, charset);
}

void WinRegStore::write_setting_fontspec(void *handle, const char *name, FontSpec* font)
{
    if (font == NULL) return;
    char *settingname;

    write_setting_s(handle, name, font->name);
    settingname = dupcat(name, "IsBold", NULL);
    write_setting_i(handle, settingname, font->isbold);
    sfree(settingname);
    settingname = dupcat(name, "CharSet", NULL);
    write_setting_i(handle, settingname, font->charset);
    sfree(settingname);
    settingname = dupcat(name, "Height", NULL);
    write_setting_i(handle, settingname, font->height);
    sfree(settingname);
    char *suffname = dupcat(name, "Name", NULL);
    write_setting_s(handle, suffname, font->name);
    sfree(suffname);
}

Filename * WinRegStore::read_setting_filename(void *handle, const char *name)
{
    char* path = IStore::read_setting_s(handle, name);
    if (path)
    {
        Filename* ret = new Filename();
        ret->path = path;
        return ret;
    }
    return NULL;
}

void WinRegStore::write_setting_filename(void *handle, const char *name, Filename* result)
{
    if (result)
    {
        write_setting_s(handle, name, result->path);
    }
}

void WinRegStore::close_settings_r(void *handle)
{
    RegCloseKey((HKEY) handle);
}

void WinRegStore::del_settings(const char *sessionname)
{
    HKEY subkey1;
    char *p;

    if (RegOpenKey(HKEY_CURRENT_USER, puttystr, &subkey1) != ERROR_SUCCESS)
        return;

    p = snewn(3 * strlen(sessionname) + 1, char);
    mungestr(sessionname, p);
    RegDeleteKey(subkey1, p);
    sfree(p);

    RegCloseKey(subkey1);

    extern void remove_from_jumplist_in_bg(const char* sessionname);
    remove_from_jumplist_in_bg(sessionname);
}

void WinRegStore::del_settings_only(const char *sessionname)
{
    HKEY subkey1;
    char *p;

    if (RegOpenKey(HKEY_CURRENT_USER, puttystr, &subkey1) != ERROR_SUCCESS)
        return;

    p = snewn(3 * strlen(sessionname) + 1, char);
    mungestr(sessionname, p);
    RegDeleteKey(subkey1, p);
    sfree(p);

    RegCloseKey(subkey1);
}

struct enumsettings
{
    HKEY key;
    int i;
};

void *WinRegStore::enum_settings_start(void)
{
    struct enumsettings *ret;
    HKEY key;

    if (RegOpenKey(HKEY_CURRENT_USER, puttystr, &key) != ERROR_SUCCESS)
        return NULL;

    ret = snew(struct enumsettings);
    if (ret)
    {
        ret->key = key;
        ret->i = 0;
    }

    return ret;
}

char *WinRegStore::enum_settings_next(void *handle, char *buffer, int buflen)
{
    struct enumsettings *e = (struct enumsettings *) handle;
    char *otherbuf;
    otherbuf = snewn(3 * buflen, char);
    if (RegEnumKey(e->key, e->i++, otherbuf, 3 * buflen) == ERROR_SUCCESS)
    {
        unmungestr(otherbuf, buffer, buflen);
        sfree(otherbuf);
        return buffer;
    }
    else
    {
        sfree(otherbuf);
        return NULL;
    }
}

void WinRegStore::enum_settings_finish(void *handle)
{
    struct enumsettings *e = (struct enumsettings *) handle;
    RegCloseKey(e->key);
    sfree(e);
}

void *WinRegStore::enum_cmd_start(void)
{
    struct enumsettings *ret;
    HKEY key;

    if (RegOpenKey(HKEY_CURRENT_USER, global_settings_key, &key) != ERROR_SUCCESS)
        return NULL;

    ret = snew(struct enumsettings);
    if (ret)
    {
        ret->key = key;
        ret->i = 0;
    }

    return ret;
}

char *WinRegStore::enum_cmd_next(void *handle, char *buffer, int buflen)
{
    struct enumsettings *e = (struct enumsettings *) handle;
    char *otherbuf;
    DWORD  otherbuflen = 3 * buflen;
    otherbuf = snewn(3 * buflen, char);
    if (RegEnumValue(e->key, e->i++, otherbuf, &otherbuflen, 0, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        unmungestr(otherbuf, buffer, buflen);
        sfree(otherbuf);
        int key_prefix_len = strlen(saved_cmd_settings_key);
        if (memcmp(saved_cmd_settings_key, buffer, key_prefix_len) == 0 && key_prefix_len < strlen(buffer))
        {
            memcpy(buffer, buffer + key_prefix_len, strlen(buffer) - key_prefix_len + 1);
            return buffer;
        }
        else
        {
            return enum_cmd_next(handle, buffer, buflen);
        }
    }
    else
    {
        sfree(otherbuf);
        return NULL;
    }
}

void WinRegStore::enum_cmd_finish(void *handle)
{
    struct enumsettings *e = (struct enumsettings *) handle;
    RegCloseKey(e->key);
    sfree(e);
}

void WinRegStore::del_cmd_settings(const char *cmd_name)
{
    HKEY subkey1;
    char *p;
    char fullname[2048] = { 0 };
    if (strlen(cmd_name) > 1024)
    {
        return;
    }
    snprintf(fullname, sizeof(fullname) - 1, "%s%s", saved_cmd_settings_key, cmd_name);

    if (RegOpenKey(HKEY_CURRENT_USER, global_settings_key, &subkey1) != ERROR_SUCCESS)
        return;

    p = snewn(3 * strlen(fullname) + 1, char);
    mungestr(fullname, p);
    RegDeleteValue(subkey1, p);
    sfree(p);

    RegCloseKey(subkey1);
}

static void hostkey_regname(char *buffer, const char *hostname,
                            int port, const char *keytype)
{
    int len;
    strcpy(buffer, keytype);
    strcat(buffer, "@");
    len = strlen(buffer);
    len += sprintf(buffer + len, "%d:", port);
    WinRegStore::mungestr(hostname, buffer + strlen(buffer));
}

int WinRegStore::verify_host_key(const char *hostname, int port,
                                 const char *keytype, const char *key)
{
    char *otherstr, *regname;
    int len;
    HKEY rkey;
    DWORD readlen;
    DWORD type;
    int ret, compare;

    len = 1 + strlen(key);

    /*
     * Now read a saved key in from the registry and see what it
     * says.
     */
    otherstr = snewn(len, char);
    regname = snewn(3 * (strlen(hostname) + strlen(keytype)) + 15, char);

    hostkey_regname(regname, hostname, port, keytype);

    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_POS "\\SshHostKeys",
                   &rkey) != ERROR_SUCCESS)
        return 1;		       /* key does not exist in registry */

    readlen = len;
    ret = RegQueryValueEx(rkey, regname, NULL, &type, (BYTE*)otherstr, &readlen);

    if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA &&
            !strcmp(keytype, "rsa"))
    {
        /*
         * Key didn't exist. If the key type is RSA, we'll try
         * another trick, which is to look up the _old_ key format
         * under just the hostname and translate that.
         */
        char *justhost = regname + 1 + strcspn(regname, ":");
        char *oldstyle = snewn(len + 10, char);	/* safety margin */
        readlen = len;
        ret = RegQueryValueEx(rkey, justhost, NULL, &type,
                              (BYTE*)oldstyle, &readlen);

        if (ret == ERROR_SUCCESS && type == REG_SZ)
        {
            /*
             * The old format is two old-style bignums separated by
             * a slash. An old-style bignum is made of groups of
             * four hex digits: digits are ordered in sensible
             * (most to least significant) order within each group,
             * but groups are ordered in silly (least to most)
             * order within the bignum. The new format is two
             * ordinary C-format hex numbers (0xABCDEFG...XYZ, with
             * A nonzero except in the special case 0x0, which
             * doesn't appear anyway in RSA keys) separated by a
             * comma. All hex digits are lowercase in both formats.
             */
            char *p = otherstr;
            char *q = oldstyle;
            int i, j;

            for (i = 0; i < 2; i++)
            {
                int ndigits, nwords;
                *p++ = '0';
                *p++ = 'x';
                ndigits = strcspn(q, "/");	/* find / or end of string */
                nwords = ndigits / 4;
                /* now trim ndigits to remove leading zeros */
                while (q[(ndigits - 1) ^ 3] == '0' && ndigits > 1)
                    ndigits--;
                /* now move digits over to new string */
                for (j = 0; j < ndigits; j++)
                    p[ndigits - 1 - j] = q[j ^ 3];
                p += ndigits;
                q += nwords * 4;
                if (*q)
                {
                    q++;	       /* eat the slash */
                    *p++ = ',';	       /* add a comma */
                }
                *p = '\0';	       /* terminate the string */
            }

            /*
             * Now _if_ this key matches, we'll enter it in the new
             * format. If not, we'll assume something odd went
             * wrong, and hyper-cautiously do nothing.
             */
            if (!strcmp(otherstr, key))
                RegSetValueEx(rkey, regname, 0, REG_SZ, (BYTE*)otherstr,
                              strlen(otherstr) + 1);
        }
    }

    RegCloseKey(rkey);

    compare = strcmp(otherstr, key);

    sfree(otherstr);
    sfree(regname);

    if (ret == ERROR_MORE_DATA ||
            (ret == ERROR_SUCCESS && type == REG_SZ && compare))
        return 2;		       /* key is different in registry */
    else if (ret != ERROR_SUCCESS || type != REG_SZ)
        return 1;		       /* key does not exist in registry */
    else
        return 0;		       /* key matched OK in registry */
}

void WinRegStore::store_host_key(const char *hostname, int port,
                                 const char *keytype, const char *key)
{
    char *regname;
    HKEY rkey;

    regname = snewn(3 * (strlen(hostname) + strlen(keytype)) + 15, char);

    hostkey_regname(regname, hostname, port, keytype);

    if (RegCreateKey(HKEY_CURRENT_USER, PUTTY_REG_POS "\\SshHostKeys",
                     &rkey) == ERROR_SUCCESS)
    {
        RegSetValueEx(rkey, regname, 0, REG_SZ, (BYTE*)key, strlen(key) + 1);
        RegCloseKey(rkey);
    } /* else key does not exist in registry */

    sfree(regname);
}

/*
 * Open (or delete) the random seed file.
 */
enum { DEL, OPEN_R, OPEN_W };
static int try_random_seed(char const *path, int action, HANDLE *ret)
{
    if (action == DEL)
    {
        remove(path);
        *ret = INVALID_HANDLE_VALUE;
        return FALSE;		       /* so we'll do the next ones too */
    }

    *ret = CreateFile(path,
                      action == OPEN_W ? GENERIC_WRITE : GENERIC_READ,
                      action == OPEN_W ? 0 : (FILE_SHARE_READ |
                              FILE_SHARE_WRITE),
                      NULL,
                      action == OPEN_W ? CREATE_ALWAYS : OPEN_EXISTING,
                      action == OPEN_W ? FILE_ATTRIBUTE_NORMAL : 0,
                      NULL);

    return (*ret != INVALID_HANDLE_VALUE);
}

static HANDLE access_random_seed(int action)
{
    HKEY rkey;
    DWORD type, size;
    HANDLE rethandle;
    char seedpath[2 * MAX_PATH + 10] = "\0";

    /*
     * Iterate over a selection of possible random seed paths until
     * we find one that works.
     *
     * We do this iteration separately for reading and writing,
     * meaning that we will automatically migrate random seed files
     * if a better location becomes available (by reading from the
     * best location in which we actually find one, and then
     * writing to the best location in which we can _create_ one).
     */

    /*
     * First, try the location specified by the user in the
     * Registry, if any.
     */
    size = sizeof(seedpath);
    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_POS, &rkey) ==
            ERROR_SUCCESS)
    {
        int ret = RegQueryValueEx(rkey, "RandSeedFile",
                                  0, &type, (BYTE*)seedpath, &size);
        if (ret != ERROR_SUCCESS || type != REG_SZ)
            seedpath[0] = '\0';
        RegCloseKey(rkey);

        if (*seedpath && try_random_seed(seedpath, action, &rethandle))
            return rethandle;
    }

    /*
     * Next, try the user's local Application Data directory,
     * followed by their non-local one. This is found using the
     * SHGetFolderPath function, which won't be present on all
     * versions of Windows.
     */
    if (!tried_shgetfolderpath)
    {
        /* This is likely only to bear fruit on systems with IE5+
         * installed, or WinMe/2K+. There is some faffing with
         * SHFOLDER.DLL we could do to try to find an equivalent
         * on older versions of Windows if we cared enough.
         * However, the invocation below requires IE5+ anyway,
         * so stuff that. */
        shell32_module = load_system32_dll("shell32.dll");
        GET_WINDOWS_FUNCTION(shell32_module, SHGetFolderPathA);
        tried_shgetfolderpath = TRUE;
    }
    if (p_SHGetFolderPathA)
    {
        if (SUCCEEDED(p_SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA,
                                         NULL, SHGFP_TYPE_CURRENT, seedpath)))
        {
            strcat(seedpath, "\\PUTTY.RND");
            if (try_random_seed(seedpath, action, &rethandle))
                return rethandle;
        }

        if (SUCCEEDED(p_SHGetFolderPathA(NULL, CSIDL_APPDATA,
                                         NULL, SHGFP_TYPE_CURRENT, seedpath)))
        {
            strcat(seedpath, "\\PUTTY.RND");
            if (try_random_seed(seedpath, action, &rethandle))
                return rethandle;
        }
    }

    /*
     * Failing that, try %HOMEDRIVE%%HOMEPATH% as a guess at the
     * user's home directory.
     */
    {
        int len, ret;

        len =
            GetEnvironmentVariable("HOMEDRIVE", seedpath,
                                   sizeof(seedpath));
        ret =
            GetEnvironmentVariable("HOMEPATH", seedpath + len,
                                   sizeof(seedpath) - len);
        if (ret != 0)
        {
            strcat(seedpath, "\\PUTTY.RND");
            if (try_random_seed(seedpath, action, &rethandle))
                return rethandle;
        }
    }

    /*
     * And finally, fall back to C:\WINDOWS.
     */
    GetWindowsDirectory(seedpath, sizeof(seedpath));
    strcat(seedpath, "\\PUTTY.RND");
    if (try_random_seed(seedpath, action, &rethandle))
        return rethandle;

    /*
     * If even that failed, give up.
     */
    return INVALID_HANDLE_VALUE;
}

void WinRegStore::read_random_seed(noise_consumer_t consumer)
{
    HANDLE seedf = access_random_seed(OPEN_R);

    if (seedf != INVALID_HANDLE_VALUE)
    {
        while (1)
        {
            char buf[1024];
            DWORD len;

            if (ReadFile(seedf, buf, sizeof(buf), &len, NULL) && len)
                consumer(buf, len);
            else
                break;
        }
        CloseHandle(seedf);
    }
}

void WinRegStore::write_random_seed(void *data, int len)
{
    HANDLE seedf = access_random_seed(OPEN_W);

    if (seedf != INVALID_HANDLE_VALUE)
    {
        DWORD lenwritten;

        WriteFile(seedf, data, len, &lenwritten, NULL);
        CloseHandle(seedf);
    }
}

/*
 * Internal function supporting the jump list registry code. All the
 * functions to add, remove and read the list have substantially
 * similar content, so this is a generalisation of all of them which
 * transforms the list in the registry by prepending 'add' (if
 * non-null), removing 'rem' from what's left (if non-null), and
 * returning the resulting concatenated list of strings in 'out' (if
 * non-null).
 */
static int transform_jumplist_registry
(const char *add, const char *rem, char **out)
{
    int ret;
    HKEY pjumplist_key, psettings_tmp;
    DWORD type;
    int value_length;
    char *old_value, *new_value;
    char *piterator_old, *piterator_new, *piterator_tmp;

    ret = RegCreateKeyEx(HKEY_CURRENT_USER, reg_jumplist_key, 0, NULL,
                         REG_OPTION_NON_VOLATILE, (KEY_READ | KEY_WRITE), NULL,
                         &pjumplist_key, NULL);
    if (ret != ERROR_SUCCESS)
    {
        return JUMPLISTREG_ERROR_KEYOPENCREATE_FAILURE;
    }

    /* Get current list of saved sessions in the registry. */
    value_length = 200;
    old_value = snewn(value_length, char);
    ret = RegQueryValueEx(pjumplist_key, reg_jumplist_value, NULL, &type,
                          (BYTE*)old_value, (DWORD*)&value_length);
    /* When the passed buffer is too small, ERROR_MORE_DATA is
     * returned and the required size is returned in the length
     * argument. */
    if (ret == ERROR_MORE_DATA)
    {
        sfree(old_value);
        old_value = snewn(value_length, char);
        ret = RegQueryValueEx(pjumplist_key, reg_jumplist_value, NULL, &type,
                              (BYTE*)old_value, (DWORD*)&value_length);
    }

    if (ret == ERROR_FILE_NOT_FOUND)
    {
        /* Value doesn't exist yet. Start from an empty value. */
        *old_value = '\0';
        *(old_value + 1) = '\0';
    }
    else if (ret != ERROR_SUCCESS)
    {
        /* Some non-recoverable error occurred. */
        sfree(old_value);
        RegCloseKey(pjumplist_key);
        return JUMPLISTREG_ERROR_VALUEREAD_FAILURE;
    }
    else if (type != REG_MULTI_SZ)
    {
        /* The value present in the registry has the wrong type: we
         * try to delete it and start from an empty value. */
        ret = RegDeleteValue(pjumplist_key, reg_jumplist_value);
        if (ret != ERROR_SUCCESS)
        {
            sfree(old_value);
            RegCloseKey(pjumplist_key);
            return JUMPLISTREG_ERROR_VALUEREAD_FAILURE;
        }

        *old_value = '\0';
        *(old_value + 1) = '\0';
    }

    /* Check validity of registry data: REG_MULTI_SZ value must end
     * with \0\0. */
    piterator_tmp = old_value;
    while (((piterator_tmp - old_value) < (value_length - 1)) &&
            !(*piterator_tmp == '\0' && *(piterator_tmp+1) == '\0'))
    {
        ++piterator_tmp;
    }

    if ((piterator_tmp - old_value) >= (value_length-1))
    {
        /* Invalid value. Start from an empty value. */
        *old_value = '\0';
        *(old_value + 1) = '\0';
    }

    /*
     * Modify the list, if we're modifying.
     */
    if (add || rem)
    {
        /* Walk through the existing list and construct the new list of
         * saved sessions. */
        new_value = snewn(value_length + (add ? strlen(add) + 1 : 0), char);
        piterator_new = new_value;
        piterator_old = old_value;

        /* First add the new item to the beginning of the list. */
        if (add)
        {
            strcpy(piterator_new, add);
            piterator_new += strlen(piterator_new) + 1;
        }
        /* Now add the existing list, taking care to leave out the removed
         * item, if it was already in the existing list. */
        while (*piterator_old != '\0')
        {
            if (!rem || strcmp(piterator_old, rem) != 0)
            {
                /* Check if this is a valid session, otherwise don't add. */
                psettings_tmp = (HKEY__*)gStorage->open_settings_r(piterator_old);
                if (psettings_tmp != NULL)
                {
                    gStorage->close_settings_r(psettings_tmp);
                    strcpy(piterator_new, piterator_old);
                    piterator_new += strlen(piterator_new) + 1;
                }
            }
            piterator_old += strlen(piterator_old) + 1;
        }
        *piterator_new = '\0';
        ++piterator_new;

        /* Save the new list to the registry. */
        ret = RegSetValueEx(pjumplist_key, reg_jumplist_value, 0, REG_MULTI_SZ,
                            (BYTE*)new_value, piterator_new - new_value);

        sfree(old_value);
        old_value = new_value;
    }
    else
        ret = ERROR_SUCCESS;

    /*
     * Either return or free the result.
     */
    if (out)
        *out = old_value;
    else
        sfree(old_value);

    /* Clean up and return. */
    RegCloseKey(pjumplist_key);

    if (ret != ERROR_SUCCESS)
    {
        return JUMPLISTREG_ERROR_VALUEWRITE_FAILURE;
    }
    else
    {
        return JUMPLISTREG_OK;
    }
}

/* Adds a new entry to the jumplist entries in the registry. */
int add_to_jumplist_registry(const char *item)
{
    return transform_jumplist_registry(item, item, NULL);
}

/* Removes an item from the jumplist entries in the registry. */
int remove_from_jumplist_registry(const char *item)
{
    return transform_jumplist_registry(NULL, item, NULL);
}

/* Returns the jumplist entries from the registry. Caller must free
 * the returned pointer. */
char *get_jumplist_registry_entries (void)
{
    char *list_value;

    if (transform_jumplist_registry(NULL,NULL,&list_value) != ERROR_SUCCESS)
    {
        list_value = snewn(2, char);
        *list_value = '\0';
        *(list_value + 1) = '\0';
    }
    return list_value;
}

/*
 * Recursively delete a registry key and everything under it.
 */
static void registry_recursive_remove(HKEY key)
{
    DWORD i;
    char name[MAX_PATH + 1];
    HKEY subkey;

    i = 0;
    while (RegEnumKey(key, i, name, sizeof(name)) == ERROR_SUCCESS)
    {
        if (RegOpenKey(key, name, &subkey) == ERROR_SUCCESS)
        {
            registry_recursive_remove(subkey);
            RegCloseKey(subkey);
        }
        RegDeleteKey(key, name);
    }
}

void WinRegStore::cleanup_all(void)
{
    HKEY key;
    int ret;
    char name[MAX_PATH + 1];

    /* ------------------------------------------------------------
     * Wipe out the random seed file, in all of its possible
     * locations.
     */
    access_random_seed(DEL);

    /* ------------------------------------------------------------
     * Ask Windows to delete any jump list information associated
     * with this installation of PuTTY.
     */
    clear_jumplist();

    /* ------------------------------------------------------------
     * Destroy all registry information associated with PuTTY.
     */

    /*
     * Open the main PuTTY registry key and remove everything in it.
     */
    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_POS, &key) ==
            ERROR_SUCCESS)
    {
        registry_recursive_remove(key);
        RegCloseKey(key);
    }
    /*
     * Now open the parent key and remove the PuTTY main key. Once
     * we've done that, see if the parent key has any other
     * children.
     */
    if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_PARENT,
                   &key) == ERROR_SUCCESS)
    {
        RegDeleteKey(key, PUTTY_REG_PARENT_CHILD);
        ret = RegEnumKey(key, 0, name, sizeof(name));
        RegCloseKey(key);
        /*
         * If the parent key had no other children, we must delete
         * it in its turn. That means opening the _grandparent_
         * key.
         */
        if (ret != ERROR_SUCCESS)
        {
            if (RegOpenKey(HKEY_CURRENT_USER, PUTTY_REG_GPARENT,
                           &key) == ERROR_SUCCESS)
            {
                RegDeleteKey(key, PUTTY_REG_GPARENT_CHILD);
                RegCloseKey(key);
            }
        }
    }
    /*
     * Now we're done.
     */
}


void WinRegStore::load_settings_to_tree234(const char *sessionname, tree234 *storage_tree)
{
    HKEY subkey1, sesskey;
    int ret;
    char munge_buffer[4096] = { 0 };

    if (!sessionname || !*sessionname)
    {
        return;
    }
    WinRegStore::mungestr(sessionname, munge_buffer);

    ret = RegOpenKey(HKEY_CURRENT_USER, puttystr, &subkey1);
    if (ret != ERROR_SUCCESS)
    {
        return;
    }

    ret = RegOpenKey(subkey1, munge_buffer, &sesskey);
    RegCloseKey(subkey1);
    if (ret != ERROR_SUCCESS)
    {
        return;
    }

    for (int i = 0; i < 10000; i++)
    {
        char key_buf[1024] = { 0 };
        char val_buf[4096] = { 0 };
        DWORD key_size = sizeof(key_buf)-1;
        DWORD type, size;
        size = sizeof(val_buf);
        ret = RegEnumValue(sesskey, i, key_buf, &key_size, NULL, &type, (BYTE *)&val_buf, &size);
        if (ret != ERROR_SUCCESS)
        {
            break;
        }
        if(type != REG_DWORD && type != REG_SZ)
        {
            int err = GetLastError();
            const char* str = win_strerror(err);
            char errstr[1024] = { 0 };
            snprintf(errstr, sizeof(errstr)-1, "errno:%d, %s\n", err, str);
            continue;
        }

        if (type == REG_DWORD)
        {
            DWORD int_value = *((DWORD*)val_buf);
            itoa(int_value, val_buf, 10);
        }

        struct skeyval find_key, *kv;
        find_key.key = key_buf;
        if ((kv = (struct skeyval*)find234(storage_tree, &find_key, NULL)) != NULL)
        {
            continue;
        }

        kv = snew(struct skeyval);
        kv->key = dupstr(key_buf);
        kv->value = dupstr(val_buf);
        struct skeyval* old_kv = (struct skeyval*)add234(storage_tree, kv);
        assert(old_kv == kv);
    }
    RegCloseKey(sesskey);
    return;
}
