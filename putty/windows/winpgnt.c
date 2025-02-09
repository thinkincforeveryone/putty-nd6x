/*
 * Pageant: the PuTTY Authentication Agent.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <tchar.h>

//#define PUTTY_DO_GLOBALS

#include "putty.h"
#include "ssh.h"
#include "misc.h"
#include "tree234.h"
#include "winsecur.h"
#include "pageant.h"

#include <shellapi.h>

#ifndef NO_SECURITY
#include <aclapi.h>
#ifdef DEBUG_IPC
#define _WIN32_WINNT 0x0500            /* for ConvertSidToStringSid */
#include <sddl.h>
#endif
#endif

#define IDI_MAINICON 200
#define IDI_TRAYICON 201

#define WM_SYSTRAY   (WM_APP + 6)
#define WM_SYSTRAY2  (WM_APP + 7)

#define AGENT_COPYDATA_ID 0x804e50ba   /* random goop */

/* From MSDN: In the WM_SYSCOMMAND message, the four low-order bits of
 * wParam are used by Windows, and should be masked off, so we shouldn't
 * attempt to store information in them. Hence all these identifiers have
 * the low 4 bits clear. Also, identifiers should < 0xF000. */

#define IDM_CLOSE    0x0010
#define IDM_VIEWKEYS 0x0020
#define IDM_ADDKEY   0x0030
#define IDM_HELP     0x0040
#define IDM_ABOUT    0x0050

#define APPNAME "Pageant"

extern char ver[];

static HWND keylist;
static HWND aboutbox;
static HMENU systray_menu, session_menu;
static int already_running;

static char *putty_path;

/* CWD for "add key" file requester. */
static filereq *keypath = NULL;

#define IDM_PUTTY         0x0060
#define IDM_SESSIONS_BASE 0x1000
#define IDM_SESSIONS_MAX  0x2000
#define PUTTY_REGKEY      "Software\\SimonTatham\\PuTTY\\Sessions"
#define PUTTY_DEFAULT     "Default%20Settings"
static int initial_menuitems_count;

/*
 * Print a modal (Really Bad) message box and perform a fatal exit.
 */
void modalfatalbox(const char *fmt, ...)
{
    va_list ap;
    char *buf;

    va_start(ap, fmt);
    buf = dupvprintf(fmt, ap);
    va_end(ap);
    MessageBox(hTopWnd, buf, "Pageant Fatal Error",
               MB_SYSTEMMODAL | MB_ICONERROR | MB_OK | MB_TOPMOST);
    sfree(buf);
    exit(1);
}

/* Un-munge session names out of the registry. */
static void unmungestr(char *in, char *out, int outlen)
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

static int has_security;

struct PassphraseProcStruct
{
    char **passphrase;
    char *comment;
};

/*
 * Dialog-box function for the Licence box.
 */
static INT_PTR CALLBACK LicenceProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        return 1;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, 1);
            return 0;
        }
        return 0;
    case WM_CLOSE:
        EndDialog(hwnd, 1);
        return 0;
    }
    return 0;
}

/*
 * Dialog-box function for the About box.
 */
static INT_PTR CALLBACK AboutProc(HWND hwnd, UINT msg,
                                  WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hwnd, 100, ver);
        return 1;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            aboutbox = NULL;
            DestroyWindow(hwnd);
            return 0;
        case 101:
            EnableWindow(hwnd, 0);
            DialogBox(hinst, MAKEINTRESOURCE(214), hwnd, LicenceProc);
            EnableWindow(hwnd, 1);
            SetActiveWindow(hwnd);
            return 0;
        }
        return 0;
    case WM_CLOSE:
        aboutbox = NULL;
        DestroyWindow(hwnd);
        return 0;
    }
    return 0;
}

static HWND passphrase_box;

/*
 * Dialog-box function for the passphrase box.
 */
static INT_PTR CALLBACK PassphraseProc(HWND hwnd, UINT msg,
                                       WPARAM wParam, LPARAM lParam)
{
    static char **passphrase = NULL;
    struct PassphraseProcStruct *p;

    switch (msg)
    {
    case WM_INITDIALOG:
        passphrase_box = hwnd;
        /*
         * Centre the window.
         */
        {
            /* centre the window */
            RECT rs, rd;
            HWND hw;

            hw = GetDesktopWindow();
            if (GetWindowRect(hw, &rs) && GetWindowRect(hwnd, &rd))
                MoveWindow(hwnd,
                           (rs.right + rs.left + rd.left - rd.right) / 2,
                           (rs.bottom + rs.top + rd.top - rd.bottom) / 2,
                           rd.right - rd.left, rd.bottom - rd.top, TRUE);
        }

        SetForegroundWindow(hwnd);
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        p = (struct PassphraseProcStruct *) lParam;
        passphrase = p->passphrase;
        if (p->comment)
            SetDlgItemText(hwnd, 101, p->comment);
        burnstr(*passphrase);
        *passphrase = dupstr("");
        SetDlgItemText(hwnd, 102, *passphrase);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (*passphrase)
                EndDialog(hwnd, 1);
            else
                MessageBeep(0);
            return 0;
        case IDCANCEL:
            EndDialog(hwnd, 0);
            return 0;
        case 102:		       /* edit box */
            if ((HIWORD(wParam) == EN_CHANGE) && passphrase)
            {
                burnstr(*passphrase);
                *passphrase = GetDlgItemText_alloc(hwnd, 102);
            }
            return 0;
        }
        return 0;
    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return 0;
    }
    return 0;
}

/*
 * Warn about the obsolescent key file format.
 */
//void old_keyfile_warning(void)
//{
//    static const char mbtitle[] = "PuTTY Key File Warning";
//    static const char message[] =
//	"You are loading an SSH-2 private key which has an\n"
//	"old version of the file format. This means your key\n"
//	"file is not fully tamperproof. Future versions of\n"
//	"PuTTY may stop supporting this private key format,\n"
//	"so we recommend you convert your key to the new\n"
//	"format.\n"
//	"\n"
//	"You can perform this conversion by loading the key\n"
//	"into PuTTYgen and then saving it again.";
//
//    MessageBox(NULL, message, mbtitle, MB_OK);
//}

/*
 * Update the visible key list.
 */
void keylist_update(void)
{
    struct RSAKey *rkey;
    struct ssh2_userkey *skey;
    int i;

    if (keylist)
    {
        SendDlgItemMessage(keylist, 100, LB_RESETCONTENT, 0, 0);
        for (i = 0; NULL != (rkey = pageant_nth_ssh1_key(i)); i++)
        {
            char listentry[512], *p;
            /*
             * Replace two spaces in the fingerprint with tabs, for
             * nice alignment in the box.
             */
            strcpy(listentry, "ssh1\t");
            p = listentry + strlen(listentry);
            rsa_fingerprint(p, sizeof(listentry) - (p - listentry), rkey);
            p = strchr(listentry, ' ');
            if (p)
                *p = '\t';
            p = strchr(listentry, ' ');
            if (p)
                *p = '\t';
            SendDlgItemMessage(keylist, 100, LB_ADDSTRING,
                               0, (LPARAM) listentry);
        }
        for (i = 0; NULL != (skey = pageant_nth_ssh2_key(i)); i++)
        {
            char *listentry, *p;
            int pos;
            /*
             * Replace spaces with tabs in the fingerprint prefix, for
             * nice alignment in the list box, until we encounter a :
             * meaning we're into the fingerprint proper.
             */
            p = ssh2_fingerprint(skey->alg, skey->data);
            listentry = dupprintf("%s\t%s", p, skey->comment);
            sfree(p);

            pos = 0;
            while (1)
            {
                pos += strcspn(listentry + pos, " :");
                if (listentry[pos] == ':' || !listentry[pos])
                    break;
                listentry[pos++] = '\t';
            }

            SendDlgItemMessage(keylist, 100, LB_ADDSTRING, 0,
                               (LPARAM) listentry);
            sfree(listentry);
        }
        SendDlgItemMessage(keylist, 100, LB_SETCURSEL, (WPARAM) - 1, 0);
    }
}

static void answer_msg(void *msgv)
{
    unsigned char *msg = (unsigned char *)msgv;
    unsigned msglen;
    void *reply;
    int replylen;

    msglen = GET_32BIT(msg);
    if (msglen > AGENT_MAX_MSGLEN)
    {
        reply = pageant_failure_msg(&replylen);
    }
    else
    {
        reply = pageant_handle_msg(msg + 4, msglen, &replylen, NULL, NULL);
        if (replylen > AGENT_MAX_MSGLEN)
        {
            smemclr(reply, replylen);
            sfree(reply);
            reply = pageant_failure_msg(&replylen);
        }
    }

    /*
     * Windows Pageant answers messages in place, by overwriting the
     * input message buffer.
     */
    memcpy(msg, reply, replylen);
    smemclr(reply, replylen);
    sfree(reply);
}

void add_keyfile(Filename *filename)
{
    char *err;
    int ret;
    char *passphrase = NULL;

    /*
     * Try loading the key without a passphrase. (Or rather, without a
     * _new_ passphrase; pageant_add_keyfile will take care of trying
     * all the passphrases we've already stored.)
     */
    ret = pageant_add_keyfile(filename, NULL, &err);
    if (ret == PAGEANT_ACTION_OK)
    {
        goto done;
    }
    else if (ret == PAGEANT_ACTION_FAILURE)
    {
        goto error;
    }

    /*
     * OK, a passphrase is needed, and we've been given the key
     * comment to use in the passphrase prompt.
     */
    while (1)
    {
        INT_PTR dlgret;
        struct PassphraseProcStruct pps;

        pps.passphrase = &passphrase;
        pps.comment = err;
        dlgret = DialogBoxParam(hinst, MAKEINTRESOURCE(114),
                                NULL, PassphraseProc, (LPARAM) &pps);
        passphrase_box = NULL;

        sfree(err);
        err = NULL;

        if (!dlgret)
            goto done;		       /* operation cancelled */

        assert(passphrase != NULL);

        ret = pageant_add_keyfile(filename, passphrase, &err);
        if (ret == PAGEANT_ACTION_OK)
        {
            goto done;
        }
        else if (ret == PAGEANT_ACTION_FAILURE)
        {
            goto error;
        }

        smemclr(passphrase, strlen(passphrase));
        sfree(passphrase);
        passphrase = NULL;
    }

error:
    message_box(err, APPNAME, MB_OK | MB_ICONERROR,
                HELPCTXID(errors_cantloadkey));
done:
    if (passphrase)
    {
        smemclr(passphrase, strlen(passphrase));
        sfree(passphrase);
    }
    sfree(err);
    return;
}

/*
 * Prompt for a key file to add, and add it.
 */
static void prompt_add_keyfile(void)
{
    OPENFILENAME of;
    char *filelist = snewn(8192, char);

    if (!keypath) keypath = filereq_new();
    memset(&of, 0, sizeof(of));
    of.hwndOwner = hTopWnd;
    of.lpstrFilter = FILTER_KEY_FILES;
    of.lpstrCustomFilter = NULL;
    of.nFilterIndex = 1;
    of.lpstrFile = filelist;
    *filelist = '\0';
    of.nMaxFile = 8192;
    of.lpstrFileTitle = NULL;
    of.lpstrTitle = "Select Private Key File";
    of.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    if (request_file(keypath, &of, TRUE, FALSE))
    {
        if(strlen(filelist) > of.nFileOffset)
        {
            /* Only one filename returned? */
            Filename *fn = filename_from_str(filelist);
            add_keyfile(fn);
            filename_free(fn);
        }
        else
        {
            /* we are returned a bunch of strings, end to
             * end. first string is the directory, the
             * rest the filenames. terminated with an
             * empty string.
             */
            char *dir = filelist;
            char *filewalker = filelist + strlen(dir) + 1;
            while (*filewalker != '\0')
            {
                char *filename = dupcat(dir, "\\", filewalker, NULL);
                Filename *fn = filename_from_str(filename);
                add_keyfile(fn);
                filename_free(fn);
                sfree(filename);
                filewalker += strlen(filewalker) + 1;
            }
        }

        keylist_update();
        pageant_forget_passphrases();
    }
    sfree(filelist);
}

/*
 * Dialog-box function for the key list box.
 */
static INT_PTR CALLBACK KeyListProc(HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam)
{
    struct RSAKey *rkey;
    struct ssh2_userkey *skey;

    switch (msg)
    {
    case WM_INITDIALOG:
        /*
         * Centre the window.
         */
    {
        /* centre the window */
        RECT rs, rd;
        HWND hw;

        hw = GetDesktopWindow();
        if (GetWindowRect(hw, &rs) && GetWindowRect(hwnd, &rd))
            MoveWindow(hwnd,
                       (rs.right + rs.left + rd.left - rd.right) / 2,
                       (rs.bottom + rs.top + rd.top - rd.bottom) / 2,
                       rd.right - rd.left, rd.bottom - rd.top, TRUE);
    }

    if (has_help())
        SetWindowLongPtr(hwnd, GWL_EXSTYLE,
                         GetWindowLongPtr(hwnd, GWL_EXSTYLE) |
                         WS_EX_CONTEXTHELP);
    else
    {
        HWND item = GetDlgItem(hwnd, 103);   /* the Help button */
        if (item)
            DestroyWindow(item);
    }

    keylist = hwnd;
    {
        static int tabs[] = { 35, 75, 250 };
        SendDlgItemMessage(hwnd, 100, LB_SETTABSTOPS,
                           sizeof(tabs) / sizeof(*tabs),
                           (LPARAM) tabs);
    }
    keylist_update();
    return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            keylist = NULL;
            DestroyWindow(hwnd);
            return 0;
        case 101:		       /* add key */
            if (HIWORD(wParam) == BN_CLICKED ||
                    HIWORD(wParam) == BN_DOUBLECLICKED)
            {
                if (passphrase_box)
                {
                    MessageBeep(MB_ICONERROR);
                    SetForegroundWindow(passphrase_box);
                    break;
                }
                prompt_add_keyfile();
            }
            return 0;
        case 102:		       /* remove key */
            if (HIWORD(wParam) == BN_CLICKED ||
                    HIWORD(wParam) == BN_DOUBLECLICKED)
            {
                int i;
                int rCount, sCount;
                int *selectedArray;

                /* our counter within the array of selected items */
                int itemNum;

                /* get the number of items selected in the list */
                int numSelected =
                    SendDlgItemMessage(hwnd, 100, LB_GETSELCOUNT, 0, 0);

                /* none selected? that was silly */
                if (numSelected == 0)
                {
                    MessageBeep(0);
                    break;
                }

                /* get item indices in an array */
                selectedArray = snewn(numSelected, int);
                SendDlgItemMessage(hwnd, 100, LB_GETSELITEMS,
                                   numSelected, (WPARAM)selectedArray);

                itemNum = numSelected - 1;
                rCount = pageant_count_ssh1_keys();
                sCount = pageant_count_ssh2_keys();

                /* go through the non-rsakeys until we've covered them all,
                 * and/or we're out of selected items to check. note that
                 * we go *backwards*, to avoid complications from deleting
                 * things hence altering the offset of subsequent items
                 */
                for (i = sCount - 1; (itemNum >= 0) && (i >= 0); i--)
                {
                    skey = pageant_nth_ssh2_key(i);

                    if (selectedArray[itemNum] == rCount + i)
                    {
                        pageant_delete_ssh2_key(skey);
                        skey->alg->freekey(skey->data);
                        sfree(skey);
                        itemNum--;
                    }
                }

                /* do the same for the rsa keys */
                for (i = rCount - 1; (itemNum >= 0) && (i >= 0); i--)
                {
                    rkey = pageant_nth_ssh1_key(i);

                    if(selectedArray[itemNum] == i)
                    {
                        pageant_delete_ssh1_key(rkey);
                        freersakey(rkey);
                        sfree(rkey);
                        itemNum--;
                    }
                }

                sfree(selectedArray);
                keylist_update();
            }
            return 0;
        case 103:		       /* help */
            if (HIWORD(wParam) == BN_CLICKED ||
                    HIWORD(wParam) == BN_DOUBLECLICKED)
            {
                launch_help(hwnd, WINHELP_CTX_pageant_general);
            }
            return 0;
        }
        return 0;
    case WM_HELP:
    {
        int id = ((LPHELPINFO)lParam)->iCtrlId;
        const char *topic = NULL;
        switch (id)
        {
        case 100:
            topic = WINHELP_CTX_pageant_keylist;
            break;
        case 101:
            topic = WINHELP_CTX_pageant_addkey;
            break;
        case 102:
            topic = WINHELP_CTX_pageant_remkey;
            break;
        }
        if (topic)
        {
            launch_help(hwnd, topic);
        }
        else
        {
            MessageBeep(0);
        }
    }
    break;
    case WM_CLOSE:
        keylist = NULL;
        DestroyWindow(hwnd);
        return 0;
    }
    return 0;
}

/* Set up a system tray icon */
static BOOL AddTrayIcon(HWND hwnd)
{
    BOOL res;
    NOTIFYICONDATA tnid;
    HICON hicon;

#ifdef NIM_SETVERSION
    tnid.uVersion = 0;
    res = Shell_NotifyIcon(NIM_SETVERSION, &tnid);
#endif

    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = hwnd;
    tnid.uID = 1;	       /* unique within this systray use */
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = WM_SYSTRAY;
    tnid.hIcon = hicon = LoadIcon(hinst, MAKEINTRESOURCE(201));
    strcpy(tnid.szTip, "Pageant (PuTTY authentication agent)");

    res = Shell_NotifyIcon(NIM_ADD, &tnid);

    if (hicon) DestroyIcon(hicon);

    return res;
}

/* Update the saved-sessions menu. */
static void update_sessions(void)
{
    int num_entries;
    HKEY hkey;
    TCHAR buf[MAX_PATH + 1];
    MENUITEMINFO mii;

    int index_key, index_menu;

    if (!putty_path)
        return;

    if(ERROR_SUCCESS != RegOpenKey(HKEY_CURRENT_USER, PUTTY_REGKEY, &hkey))
        return;

    for(num_entries = GetMenuItemCount(session_menu);
            num_entries > initial_menuitems_count;
            num_entries--)
        RemoveMenu(session_menu, 0, MF_BYPOSITION);

    index_key = 0;
    index_menu = 0;

    while(ERROR_SUCCESS == RegEnumKey(hkey, index_key, buf, MAX_PATH))
    {
        TCHAR session_name[MAX_PATH + 1];
        unmungestr(buf, session_name, MAX_PATH);
        if(strcmp(buf, PUTTY_DEFAULT) != 0)
        {
            memset(&mii, 0, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
            mii.fType = MFT_STRING;
            mii.fState = MFS_ENABLED;
            mii.wID = (index_menu * 16) + IDM_SESSIONS_BASE;
            mii.dwTypeData = session_name;
            InsertMenuItem(session_menu, index_menu, TRUE, &mii);
            index_menu++;
        }
        index_key++;
    }

    RegCloseKey(hkey);

    if(index_menu == 0)
    {
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE | MIIM_STATE;
        mii.fType = MFT_STRING;
        mii.fState = MFS_GRAYED;
        mii.dwTypeData = _T("(No sessions)");
        InsertMenuItem(session_menu, index_menu, TRUE, &mii);
    }
}

#ifndef NO_SECURITY
/*
 * Versions of Pageant prior to 0.61 expected this SID on incoming
 * communications. For backwards compatibility, and more particularly
 * for compatibility with derived works of PuTTY still using the old
 * Pageant client code, we accept it as an alternative to the one
 * returned from get_user_sid() in winpgntc.c.
 */
//PSID get_default_sid(void)
//{
//    HANDLE proc = NULL;
//    DWORD sidlen;
//    PSECURITY_DESCRIPTOR psd = NULL;
//    PSID sid = NULL, copy = NULL, ret = NULL;
//
//    if ((proc = OpenProcess(MAXIMUM_ALLOWED, FALSE,
//                            GetCurrentProcessId())) == NULL)
//        goto cleanup;
//
//    if (p_GetSecurityInfo(proc, SE_KERNEL_OBJECT, OWNER_SECURITY_INFORMATION,
//                          &sid, NULL, NULL, NULL, &psd) != ERROR_SUCCESS)
//        goto cleanup;
//
//    sidlen = GetLengthSid(sid);
//
//    copy = (PSID)smalloc(sidlen);
//
//    if (!CopySid(sidlen, copy, sid))
//        goto cleanup;
//
//    /* Success. Move sid into the return value slot, and null it out
//     * to stop the cleanup code freeing it. */
//    ret = copy;
//    copy = NULL;
//
//  cleanup:
//    if (proc != NULL)
//        CloseHandle(proc);
//    if (psd != NULL)
//        LocalFree(psd);
//    if (copy != NULL)
//        sfree(copy);
//
//    return ret;
//}
#endif
//
//static LRESULT CALLBACK WndProc(HWND hwnd, UINT message,
//				WPARAM wParam, LPARAM lParam)
//{
//    static int menuinprogress;
//    static UINT msgTaskbarCreated = 0;
//
//    switch (message) {
//      case WM_CREATE:
//        msgTaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
//        break;
//      default:
//        if (message==msgTaskbarCreated) {
//            /*
//	     * Explorer has been restarted, so the tray icon will
//	     * have been lost.
//	     */
//	    AddTrayIcon(hwnd);
//        }
//        break;
//
//      case WM_SYSTRAY:
//	if (lParam == WM_RBUTTONUP) {
//	    POINT cursorpos;
//	    GetCursorPos(&cursorpos);
//	    PostMessage(hwnd, WM_SYSTRAY2, cursorpos.x, cursorpos.y);
//	} else if (lParam == WM_LBUTTONDBLCLK) {
//	    /* Run the default menu item. */
//	    UINT menuitem = GetMenuDefaultItem(systray_menu, FALSE, 0);
//	    if (menuitem != -1)
//		PostMessage(hwnd, WM_COMMAND, menuitem, 0);
//	}
//	break;
//      case WM_SYSTRAY2:
//	if (!menuinprogress) {
//	    menuinprogress = 1;
//	    update_sessions();
//	    SetForegroundWindow(hwnd);
//	    TrackPopupMenu(systray_menu,
//			   TPM_RIGHTALIGN | TPM_BOTTOMALIGN |
//			   TPM_RIGHTBUTTON,
//			   wParam, lParam, 0, hwnd, NULL);
//	    menuinprogress = 0;
//	}
//	break;
//      case WM_COMMAND:
//      case WM_SYSCOMMAND:
//	switch (wParam & ~0xF) {       /* low 4 bits reserved to Windows */
//	  case IDM_PUTTY:
//	    if((INT_PTR)ShellExecute(hwnd, NULL, putty_path, _T(""), _T(""),
//				 SW_SHOW) <= 32) {
//		MessageBox(NULL, "Unable to execute PuTTY!",
//			   "Error", MB_OK | MB_ICONERROR);
//	    }
//	    break;
//	  case IDM_CLOSE:
//	    if (passphrase_box)
//		SendMessage(passphrase_box, WM_CLOSE, 0, 0);
//	    SendMessage(hwnd, WM_CLOSE, 0, 0);
//	    break;
//	  case IDM_VIEWKEYS:
//	    if (!keylist) {
//		keylist = CreateDialog(hinst, MAKEINTRESOURCE(211),
//				       NULL, KeyListProc);
//		ShowWindow(keylist, SW_SHOWNORMAL);
//	    }
//	    /*
//	     * Sometimes the window comes up minimised / hidden for
//	     * no obvious reason. Prevent this. This also brings it
//	     * to the front if it's already present (the user
//	     * selected View Keys because they wanted to _see_ the
//	     * thing).
//	     */
//	    SetForegroundWindow(keylist);
//	    SetWindowPos(keylist, HWND_TOP, 0, 0, 0, 0,
//			 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
//	    break;
//	  case IDM_ADDKEY:
//	    if (passphrase_box) {
//		MessageBeep(MB_ICONERROR);
//		SetForegroundWindow(passphrase_box);
//		break;
//	    }
//	    prompt_add_keyfile();
//	    break;
//	  case IDM_ABOUT:
//	    if (!aboutbox) {
//		aboutbox = CreateDialog(hinst, MAKEINTRESOURCE(213),
//					NULL, AboutProc);
//		ShowWindow(aboutbox, SW_SHOWNORMAL);
//		/*
//		 * Sometimes the window comes up minimised / hidden
//		 * for no obvious reason. Prevent this.
//		 */
//		SetForegroundWindow(aboutbox);
//		SetWindowPos(aboutbox, HWND_TOP, 0, 0, 0, 0,
//			     SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
//	    }
//	    break;
//	  case IDM_HELP:
//	    launch_help(hwnd, WINHELP_CTX_pageant_general);
//	    break;
//	  default:
//	    {
//		if(wParam >= IDM_SESSIONS_BASE && wParam <= IDM_SESSIONS_MAX) {
//		    MENUITEMINFO mii;
//		    TCHAR buf[MAX_PATH + 1];
//		    TCHAR param[MAX_PATH + 1];
//		    memset(&mii, 0, sizeof(mii));
//		    mii.cbSize = sizeof(mii);
//		    mii.fMask = MIIM_TYPE;
//		    mii.cch = MAX_PATH;
//		    mii.dwTypeData = buf;
//		    GetMenuItemInfo(session_menu, wParam, FALSE, &mii);
//		    strcpy(param, "@");
//		    strcat(param, mii.dwTypeData);
//		    if((INT_PTR)ShellExecute(hwnd, NULL, putty_path, param,
//					 _T(""), SW_SHOW) <= 32) {
//			MessageBox(NULL, "Unable to execute PuTTY!", "Error",
//				   MB_OK | MB_ICONERROR);
//		    }
//		}
//	    }
//	    break;
//	}
//	break;
//      case WM_DESTROY:
//	quit_help(hwnd);
//	PostQuitMessage(0);
//	return 0;
//      case WM_COPYDATA:
//	{
//	    COPYDATASTRUCT *cds;
//	    char *mapname;
//	    void *p;
//	    HANDLE filemap;
//#ifndef NO_SECURITY
//	    PSID mapowner, ourself, ourself2;
//#endif
//            PSECURITY_DESCRIPTOR psd = NULL;
//	    int ret = 0;
//
//	    cds = (COPYDATASTRUCT *) lParam;
//	    if (cds->dwData != AGENT_COPYDATA_ID)
//		return 0;	       /* not our message, mate */
//	    mapname = (char *) cds->lpData;
//	    if (mapname[cds->cbData - 1] != '\0')
//		return 0;	       /* failure to be ASCIZ! */
//#ifdef DEBUG_IPC
//	    debug(("mapname is :%s:\n", mapname));
//#endif
//	    filemap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mapname);
//#ifdef DEBUG_IPC
//	    debug(("filemap is %p\n", filemap));
//#endif
//	    if (filemap != NULL && filemap != INVALID_HANDLE_VALUE) {
//#ifndef NO_SECURITY
//		int rc;
//		if (has_security) {
//                    if ((ourself = get_user_sid()) == NULL) {
//#ifdef DEBUG_IPC
//			debug(("couldn't get user SID\n"));
//#endif
//                        CloseHandle(filemap);
//			return 0;
//                    }
//
//                    if ((ourself2 = get_default_sid()) == NULL) {
//#ifdef DEBUG_IPC
//			debug(("couldn't get default SID\n"));
//#endif
//                        CloseHandle(filemap);
//                        sfree(ourself);
//			return 0;
//                    }
//
//		    if ((rc = p_GetSecurityInfo(filemap, SE_KERNEL_OBJECT,
//						OWNER_SECURITY_INFORMATION,
//						&mapowner, NULL, NULL, NULL,
//						&psd) != ERROR_SUCCESS)) {
//#ifdef DEBUG_IPC
//			debug(("couldn't get owner info for filemap: %d\n",
//                               rc));
//#endif
//                        CloseHandle(filemap);
//                        sfree(ourself);
//                        sfree(ourself2);
//			return 0;
//		    }
//#ifdef DEBUG_IPC
//                    {
//                        LPTSTR ours, ours2, theirs;
//                        ConvertSidToStringSid(mapowner, &theirs);
//                        ConvertSidToStringSid(ourself, &ours);
//                        ConvertSidToStringSid(ourself2, &ours2);
//                        debug(("got sids:\n  oursnew=%s\n  oursold=%s\n"
//                               "  theirs=%s\n", ours, ours2, theirs));
//                        LocalFree(ours);
//                        LocalFree(ours2);
//                        LocalFree(theirs);
//                    }
//#endif
//		    if (!EqualSid(mapowner, ourself) &&
//                        !EqualSid(mapowner, ourself2)) {
//                        CloseHandle(filemap);
//                        LocalFree(psd);
//                        sfree(ourself);
//                        sfree(ourself2);
//			return 0;      /* security ID mismatch! */
//                    }
//#ifdef DEBUG_IPC
//		    debug(("security stuff matched\n"));
//#endif
//                    LocalFree(psd);
//                    sfree(ourself);
//                    sfree(ourself2);
//		} else {
//#ifdef DEBUG_IPC
//		    debug(("security APIs not present\n"));
//#endif
//		}
//#endif
//		p = MapViewOfFile(filemap, FILE_MAP_WRITE, 0, 0, 0);
//#ifdef DEBUG_IPC
//		debug(("p is %p\n", p));
//		{
//		    int i;
//		    for (i = 0; i < 5; i++)
//			debug(("p[%d]=%02x\n", i,
//			       ((unsigned char *) p)[i]));
//                }
//#endif
//		answer_msg(p);
//		ret = 1;
//		UnmapViewOfFile(p);
//	    }
//	    CloseHandle(filemap);
//	    return ret;
//	}
//    }
//
//    return DefWindowProc(hwnd, message, wParam, lParam);
//}

/*
 * Fork and Exec the command in cmdline. [DBW]
 */
void spawn_cmd(const char *cmdline, const char *args, int show)
{
    if (ShellExecute(NULL, _T("open"), cmdline,
                     args, NULL, show) <= (HINSTANCE) 32)
    {
        char *msg;
        msg = dupprintf("Failed to run \"%.100s\", Error: %d", cmdline,
                        (int)GetLastError());
        MessageBox(NULL, msg, APPNAME, MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
        sfree(msg);
    }
}

/*
 * This is a can't-happen stub, since Pageant never makes
 * asynchronous agent requests.
 */
void agent_schedule_callback(void (*callback)(void *, void *, int),
                             void *callback_ctx, void *data, int len)
{
    assert(!"We shouldn't get here");
}

//void cleanup_exit(int code)
//{
//    shutdown_help();
//    exit(code);
//}

