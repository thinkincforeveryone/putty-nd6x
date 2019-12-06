/*
 * uxstore.c: Unix-specific implementation of the interface defined
 * in storage.h.
 */
#ifdef WINNT
typedef long int off_t;
#include <windows.h>

int mkdir(const char* dir, int attr)
{
    return CreateDirectory(dir, NULL) ? 0 : -1;
}

#else
#include <pwd.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "putty.h"
#include "storage.h"
#include "tree234.h"

#ifdef PATH_MAX
#define FNLEN PATH_MAX
#else
#define FNLEN 1024 /* XXX */
#endif

FileStore gFileStore;

enum
{
    INDEX_DIR, INDEX_HOSTKEYS, INDEX_HOSTKEYS_TMP, INDEX_RANDSEED,
    INDEX_SESSIONDIR, INDEX_SESSION,
};

static const char hex[17] = "0123456789ABCDEF";

char *FileStore::mungestr(const char *in)
{
    char *out, *ret;

    if (!in || !*in)
        in = DEFAULT_SESSION_NAME;

    ret = out = snewn(3*strlen(in)+1, char);

    while (*in)
    {
        /*
         * There are remarkably few punctuation characters that
         * aren't shell-special in some way or likely to be used as
         * separators in some file format or another! Hence we use
         * opt-in for safe characters rather than opt-out for
         * specific unsafe ones...
         */
        if (*in!='+' && *in!='-' && *in!='.' && *in!='@' && *in!='_' &&
                !(*in >= '0' && *in <= '9') &&
                !(*in >= 'A' && *in <= 'Z') &&
                !(*in >= 'a' && *in <= 'z'))
        {
            *out++ = '%';
            *out++ = hex[((unsigned char) *in) >> 4];
            *out++ = hex[((unsigned char) *in) & 15];
        }
        else
            *out++ = *in;
        in++;
    }
    *out = '\0';
    return ret;
}

char *FileStore::unmungestr(const char *in)
{
    char *out, *ret;
    out = ret = snewn(strlen(in)+1, char);
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
            in += 3;
        }
        else
        {
            *out++ = *in++;
        }
    }
    *out = '\0';
    return ret;
}

char *FileStore::make_filename(int index, const char *subname)
{
    char *env, *tmp, *ret;

    if (*pathM != 0)
    {
        if (index == INDEX_DIR)
        {
            return dupstr(pathM);
        }
        else if (index == INDEX_SESSIONDIR)
        {
            return dupstr(pathM);
        }
        else if (index == INDEX_SESSION)
        {
            char *munged = mungestr(subname);
            tmp = make_filename(INDEX_SESSIONDIR, NULL);
            ret = dupprintf("%s/%s", tmp, munged);
            sfree(tmp);
            sfree(munged);
            return ret;
        }
        else
        {
            assert(0);
        }
    }
    /*
     * Allow override of the PuTTY configuration location, and of
     * specific subparts of it, by means of environment variables.
     */
    if (index == INDEX_DIR)
    {

        env = getenv("PUTTYDIR");
        if (env)
            return dupstr(env);
        env = getenv("HOME");
        if (env)
            return dupprintf("%s/.putty", env);
#ifndef WINNT
        struct passwd *pwd;
        pwd = getpwuid(getuid());
        if (pwd && pwd->pw_dir)
            return dupprintf("%s/.putty", pwd->pw_dir);
#endif
        return dupstr("/.putty");
    }
    if (index == INDEX_SESSIONDIR)
    {
        env = getenv("PUTTYSESSIONS");
        if (env)
            return dupstr(env);
        tmp = make_filename(INDEX_DIR, NULL);
        ret = dupprintf("%s/sessions", tmp);
        sfree(tmp);
        return ret;
    }
    if (index == INDEX_SESSION)
    {
        char *munged = mungestr(subname);
        tmp = make_filename(INDEX_SESSIONDIR, NULL);
        ret = dupprintf("%s/%s", tmp, munged);
        sfree(tmp);
        sfree(munged);
        return ret;
    }
    if (index == INDEX_HOSTKEYS)
    {
        env = getenv("PUTTYSSHHOSTKEYS");
        if (env)
            return dupstr(env);
        tmp = make_filename(INDEX_DIR, NULL);
        ret = dupprintf("%s/sshhostkeys", tmp);
        sfree(tmp);
        return ret;
    }
    if (index == INDEX_HOSTKEYS_TMP)
    {
        tmp = make_filename(INDEX_HOSTKEYS, NULL);
        ret = dupprintf("%s.tmp", tmp);
        sfree(tmp);
        return ret;
    }
    if (index == INDEX_RANDSEED)
    {
        env = getenv("PUTTYRANDOMSEED");
        if (env)
            return dupstr(env);
        tmp = make_filename(INDEX_DIR, NULL);
        ret = dupprintf("%s/randomseed", tmp);
        sfree(tmp);
        return ret;
    }
    tmp = make_filename(INDEX_DIR, NULL);
    ret = dupprintf("%s/ERROR", tmp);
    sfree(tmp);
    return ret;
}

void *FileStore::open_settings_w(const char *sessionname, char **errmsg)
{
    char *filename;
    FILE *fp;

    *errmsg = NULL;

    /*
     * Start by making sure the .putty directory and its sessions
     * subdir actually exist. Ignore error returns from mkdir since
     * they're perfectly likely to be `already exists', and any
     * other error will trip us up later on so there's no real need
     * to catch it now.
     */
    filename = make_filename(INDEX_SESSIONDIR, NULL);
    if (mkdir(filename, 0700) != 0)
    {
        char *filename2 = make_filename(INDEX_DIR, NULL);
        mkdir(filename2, 0700);
        sfree(filename2);
        mkdir(filename, 0700);
    }
    sfree(filename);

    filename = make_filename(INDEX_SESSION, sessionname);
    fp = fopen(filename, "w");
    if (!fp)
    {
        *errmsg = dupprintf("Unable to create %s: %s",
                            filename, strerror(errno));
        sfree(filename);
        return NULL;                   /* can't open */
    }
    sfree(filename);
    return fp;
}

void FileStore::write_setting_s(void *handle, const char *key, const char *value)
{
    FILE *fp = (FILE *)handle;
    fprintf(fp, "%s=%s\n", key, value);
}

void FileStore::write_setting_i(void *handle, const char *key, int value)
{
    FILE *fp = (FILE *)handle;
    fprintf(fp, "%s=%d\n", key, value);
}

void FileStore::close_settings_w(void *handle)
{
    FILE *fp = (FILE *)handle;
    fclose(fp);
}

/*
 * Reading settings, for the moment, is done by retrieving X
 * resources from the X display. When we introduce disk files, I
 * think what will happen is that the X resources will override
 * PuTTY's inbuilt defaults, but that the disk files will then
 * override those. This isn't optimal, but it's the best I can
 * immediately work out.
 * FIXME: the above comment is a bit out of date. Did it happen?
 */

static tree234 *xrmtree = NULL;

void provide_xrm_string(char *string)
{
    char *p, *q, *key;
    struct skeyval *xrms, *ret;

    p = q = strchr(string, ':');
    if (!q)
    {
        fprintf(stderr, "pterm: expected a colon in resource string"
                " \"%s\"\n", string);
        return;
    }
    q++;
    while (p > string && p[-1] != '.' && p[-1] != '*')
        p--;
    xrms = snew(struct skeyval);
    key = snewn(q-p, char);
    memcpy(key, p, q-p);
    key[q-p-1] = '\0';
    xrms->key = key;
    while (*q && isspace((unsigned char)*q))
        q++;
    xrms->value = dupstr(q);

    if (!xrmtree)
        xrmtree = newtree234(keycmp);

    ret = (struct skeyval*)add234(xrmtree, xrms);
    if (ret)
    {
        /* Override an existing string. */
        del234(xrmtree, ret);
        add234(xrmtree, xrms);
    }
}

const char *get_setting(const char *key)
{
    struct skeyval tmp, *ret;
    tmp.key = key;
    if (xrmtree)
    {
        ret = (skeyval*)find234(xrmtree, &tmp, NULL);
        if (ret)
            return ret->value;
    }
#ifdef WINNT
    return NULL;
#else
    return x_get_default(key);
#endif
}

void *FileStore::open_settings_r(const char *sessionname)
{
    char *filename;
    FILE *fp;
    char *line;
    tree234 *ret;

    filename = make_filename(INDEX_SESSION, sessionname);
    fp = fopen(filename, "r");
    sfree(filename);
    if (!fp)
        return NULL;		       /* can't open */

    ret = newtree234(keycmp);

    while ( (line = fgetline(fp)) )
    {
        char *value = strchr(line, '=');
        struct skeyval *kv;

        if (!value)
            continue;
        *value++ = '\0';
        value[strcspn(value, "\r\n")] = '\0';   /* trim trailing NL */

        kv = snew(struct skeyval);
        kv->key = dupstr(line);
        kv->value = dupstr(value);
        add234(ret, kv);

        sfree(line);
    }

    fclose(fp);

    return ret;
}

char *FileStore::read_setting_s(void *handle, const char *key, char *buffer, int buflen)
{
    tree234 *tree = (tree234 *)handle;
    const char *val = NULL;
    struct skeyval tmp, *kv;

    tmp.key = key;
    if (tree != NULL &&
            (kv = (struct skeyval*)find234(tree, &tmp, NULL)) != NULL)
    {
        val = kv->value;
        assert(val != NULL);
    }
    else
        val = get_setting(key);

    if (!val)
        return NULL;
    else
    {
        strncpy(buffer, val, buflen);
        buffer[buflen-1] = '\0';
        return buffer;
    }
}

int FileStore::read_setting_i(void *handle, const char *key, int defvalue)
{
    tree234 *tree = (tree234 *)handle;
    const char *val = NULL;
    struct skeyval tmp, *kv;

    tmp.key = key;
    if (tree != NULL &&
            (kv = (struct skeyval*)find234(tree, &tmp, NULL)) != NULL)
    {
        val = kv->value;
        assert(val != NULL);
    }
    else
        val = get_setting(key);

    if (!val)
        return defvalue;
    else
        return atoi(val);
}

FontSpec * FileStore::read_setting_fontspec(void *handle, const char *name)
{
    /*
     * In GTK1-only PuTTY, we used to store font names simply as a
     * valid X font description string (logical or alias), under a
     * bare key such as "Font".
     *
     * In GTK2 PuTTY, we have a prefix system where "client:"
     * indicates a Pango font and "server:" an X one; existing
     * configuration needs to be reinterpreted as having the
     * "server:" prefix, so we change the storage key from the
     * provided name string (e.g. "Font") to a suffixed one
     * ("FontName").
     */
    char *settingname;
    char* font_name = NULL;
    int isbold = 0, charset = 0, height = 0;

    if (!(font_name = IStore::read_setting_s(handle, name)))
    {
        char *suffname = dupcat(name, "Name", NULL);
        if (font_name = IStore::read_setting_s(handle, suffname))
        {
            /* got new-style name */
            sfree(suffname);
        }
        else
        {
            sfree(suffname);
            return NULL;
        }
    }
    settingname = dupcat(name, "IsBold", NULL);
    isbold = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (isbold == -1) return 0;

    settingname = dupcat(name, "CharSet", NULL);
    charset = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (charset == -1) return 0;

    settingname = dupcat(name, "Height", NULL);
    height = read_setting_i(handle, settingname, INT_MIN);
    sfree(settingname);
    if (height == INT_MIN) return 0;

    return fontspec_new(font_name, isbold, height, charset);
}

Filename *FileStore::read_setting_filename(void *handle, const char *name)
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

void FileStore::write_setting_fontspec(void *handle, const char *name, FontSpec* result)
{
    if (result == NULL)return;
    /*
     * read_setting_fontspec had to handle two cases, but when
     * writing our settings back out we simply always generate the
     * new-style name.
     */
    char *suffname = dupcat(name, "Name", NULL);
    write_setting_s(handle, suffname, result->name);
    sfree(suffname);

    char *settingname;
    write_setting_s(handle, name, result->name);
    settingname = dupcat(name, "IsBold", NULL);
    write_setting_i(handle, settingname, result->isbold);
    sfree(settingname);
    settingname = dupcat(name, "CharSet", NULL);
    write_setting_i(handle, settingname, result->charset);
    sfree(settingname);
    settingname = dupcat(name, "Height", NULL);
    write_setting_i(handle, settingname, result->height);
    sfree(settingname);
}
void FileStore::write_setting_filename(void *handle, const char *name, Filename* result)
{
    if (result)
    {
        write_setting_s(handle, name, result->path);
    }
}

void FileStore::close_settings_r(void *handle)
{
    tree234 *tree = (tree234 *)handle;
    struct skeyval *kv;

    if (!tree)
        return;

    while ( (kv = (struct skeyval*)index234(tree, 0)) != NULL)
    {
        del234(tree, kv);
        sfree((char *)kv->key);
        sfree((char *)kv->value);
        sfree(kv);
    }

    freetree234(tree);
}

void FileStore::del_settings(const char *sessionname)
{
    char *filename;
    filename = make_filename(INDEX_SESSION, sessionname);
    unlink(filename);
    sfree(filename);
}

void *FileStore::enum_settings_start(void)
{
    DIR *dp;
    char *filename;

    filename = make_filename(INDEX_SESSIONDIR, NULL);
    dp = opendir(filename);
    sfree(filename);

    return dp;
}

char *FileStore::enum_settings_next(void *handle, char *buffer, int buflen)
{
    DIR *dp = (DIR *)handle;
    struct dirent *de;
    char *fullpath;
    int maxlen, thislen, len;
    char *unmunged;

    fullpath = make_filename(INDEX_SESSIONDIR, NULL);
    maxlen = len = strlen(fullpath);

    while ( (de = readdir(dp)) != NULL )
    {
        thislen = len + 1 + strlen(de->d_name);
        if (maxlen < thislen)
        {
            maxlen = thislen;
            fullpath = sresize(fullpath, maxlen+1, char);
        }
        fullpath[len] = '/';
        strncpy(fullpath+len+1, de->d_name, thislen - (len+1));
        fullpath[thislen] = '\0';
#ifndef WINNT
        struct stat st;
        if (stat(fullpath, &st) < 0 || !S_ISREG(st.st_mode))
            continue;                  /* try another one */
#else
        WIN32_FIND_DATA fd;
        HANDLE hFind = ::FindFirstFile(fullpath,&fd);
        ::FindClose(hFind);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            continue;
        }
        else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue;
        }
        //else file

#endif

        unmunged = unmungestr(de->d_name);
        strncpy(buffer, unmunged, buflen);
        buffer[buflen-1] = '\0';
        sfree(unmunged);
        sfree(fullpath);
        return buffer;
    }

    sfree(fullpath);
    return NULL;
}

void FileStore::enum_settings_finish(void *handle)
{
    DIR *dp = (DIR *)handle;
    closedir(dp);
}

/*
 * Lines in the host keys file are of the form
 *
 *   type@port:hostname keydata
 *
 * e.g.
 *
 *   rsa@22:foovax.example.org 0x23,0x293487364395345345....2343
 */
int FileStore::verify_host_key(const char *hostname, int port,
                               const char *keytype, const char *key)
{
    FILE *fp;
    char *filename;
    char *line;
    int ret;

    filename = make_filename(INDEX_HOSTKEYS, NULL);
    fp = fopen(filename, "r");
    sfree(filename);
    if (!fp)
        return 1;		       /* key does not exist */

    ret = 1;
    while ( (line = fgetline(fp)) )
    {
        int i;
        char *p = line;
        char porttext[20];

        line[strcspn(line, "\n")] = '\0';   /* strip trailing newline */

        i = strlen(keytype);
        if (strncmp(p, keytype, i))
            goto done;
        p += i;

        if (*p != '@')
            goto done;
        p++;

        sprintf(porttext, "%d", port);
        i = strlen(porttext);
        if (strncmp(p, porttext, i))
            goto done;
        p += i;

        if (*p != ':')
            goto done;
        p++;

        i = strlen(hostname);
        if (strncmp(p, hostname, i))
            goto done;
        p += i;

        if (*p != ' ')
            goto done;
        p++;

        /*
         * Found the key. Now just work out whether it's the right
         * one or not.
         */
        if (!strcmp(p, key))
            ret = 0;		       /* key matched OK */
        else
            ret = 2;		       /* key mismatch */

done:
        sfree(line);
        if (ret != 1)
            break;
    }

    fclose(fp);
    return ret;
}

void FileStore::store_host_key(const char *hostname, int port,
                               const char *keytype, const char *key)
{
    FILE *rfp, *wfp;
    char *newtext, *line;
    int headerlen;
    char *filename, *tmpfilename;

    newtext = dupprintf("%s@%d:%s %s\n", keytype, port, hostname, key);
    headerlen = 1 + strcspn(newtext, " ");   /* count the space too */

    /*
     * Open both the old file and a new file.
     */
    tmpfilename = make_filename(INDEX_HOSTKEYS_TMP, NULL);
    wfp = fopen(tmpfilename, "w");
    if (!wfp)
    {
        char *dir;

        dir = make_filename(INDEX_DIR, NULL);
        mkdir(dir, 0700);
        sfree(dir);

        wfp = fopen(tmpfilename, "w");
    }
    if (!wfp)
    {
        sfree(tmpfilename);
        return;
    }
    filename = make_filename(INDEX_HOSTKEYS, NULL);
    rfp = fopen(filename, "r");

    /*
     * Copy all lines from the old file to the new one that _don't_
     * involve the same host key identifier as the one we're adding.
     */
    if (rfp)
    {
        while ( (line = fgetline(rfp)) )
        {
            if (strncmp(line, newtext, headerlen))
                fputs(line, wfp);
        }
        fclose(rfp);
    }

    /*
     * Now add the new line at the end.
     */
    fputs(newtext, wfp);

    fclose(wfp);

    rename(tmpfilename, filename);

    sfree(tmpfilename);
    sfree(filename);
    sfree(newtext);
}

void FileStore::read_random_seed(noise_consumer_t consumer)
{
#ifndef WINNT
    int fd;
    char *fname;

    fname = make_filename(INDEX_RANDSEED, NULL);
    fd = open(fname, O_RDONLY);
    sfree(fname);
    if (fd >= 0)
    {
        char buf[512];
        int ret;
        while ( (ret = read(fd, buf, sizeof(buf))) > 0)
            consumer(buf, ret);
        close(fd);
    }
#endif
}

void FileStore::write_random_seed(void *data, int len)
{
#ifndef WINNT
    int fd;
    char *fname;

    fname = make_filename(INDEX_RANDSEED, NULL);
    /*
     * Don't truncate the random seed file if it already exists; if
     * something goes wrong half way through writing it, it would
     * be better to leave the old data there than to leave it empty.
     */
    fd = open(fname, O_CREAT | O_WRONLY, 0600);
    if (fd < 0)
    {
        char *dir;

        dir = make_filename(INDEX_DIR, NULL);
        mkdir(dir, 0700);
        sfree(dir);

        fd = open(fname, O_CREAT | O_WRONLY, 0600);
    }

    while (len > 0)
    {
        int ret = write(fd, data, len);
        if (ret <= 0) break;
        len -= ret;
        data = (char *)data + len;
    }

    close(fd);
    sfree(fname);
#endif
}

void FileStore::cleanup_all(void)
{
}
