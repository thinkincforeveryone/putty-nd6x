/*
 * windefs.c: default settings that are specific to Windows.
 */

#include "putty.h"

#include <commctrl.h>

FontSpec *platform_default_fontspec(const char *name)
{
    if (!strcmp(name, "Font"))
        return fontspec_new("Courier New", 0, 14, ANSI_CHARSET);
    else
        return fontspec_new("", 0, 0, 0);
}

Filename *platform_default_filename(const char *name)
{
    if (!strcmp(name, "LogFileName"))
        return filename_from_str("putty.log");
    else
        return filename_from_str("");
}

char *platform_default_s(const char *name)
{
    if (!strcmp(name, "SerialLine"))
        return dupstr("COM1");
    return NULL;
}

int platform_default_i(const char *name, int def)
{
    return def;
}
