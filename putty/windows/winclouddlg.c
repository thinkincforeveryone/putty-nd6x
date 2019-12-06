/*
 * windlg.c - dialogs for PuTTY(tel), including the configuration dialog.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>

#include "putty.h"
#include "ssh.h"
#include "win_res.h"
#include "storage.h"
#include "dialog.h"

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#include <Shlobj.h>
#include "native_putty_common.h"
#include <vector>
#include <list>
#include <set>
#include <algorithm>
using namespace std;
struct winctrl *dlg_findbyctrl(struct dlgparam *dp, union control *ctrl);

void get_cmdlist(std::vector<std::string>& cmdlist);
/*
 * These are the various bits of data required to handle the
 * portable-dialog stuff in the config box. Having them at file
 * scope in here isn't too bad a place to put them; if we were ever
 * to need more than one config box per process we could always
 * shift them to a per-config-box structure stored in GWL_USERDATA.
 */
static struct controlbox *ctrlbox;
/*
 * ctrls_base holds the OK and Cancel buttons: the controls which
 * are present in all dialog panels. ctrls_panel holds the ones
 * which change from panel to panel.
 */
// multi-thread is not supported
static struct winctrls ctrls_base;
static struct dlgparam dp;

controlset* middle_btn_controlset = NULL;
controlset* bottom_btn_controlset = NULL;

static bool isFreshingSessionTreeView = false;
static HMENU st_popup_menus[4];
enum
{
    SESSION_GROUP = 0 ,
    SESSION_ITEM  = 1,
    SESSION_NONE  = 2,
    SESSION_DEFAULT  = 3
};

enum
{
    DRAG_BEGIN = 0 ,
    DRAG_MOVE  = 1,
    DRAG_CTRL_DOWN = 2,
    DRAG_CTRL_UP   = 3,
    DRAG_END   = 4,
    DRAG_CANCEL = 5
};

static HWND hEdit = NULL;
enum
{
    EDIT_BEGIN = 0 ,
    EDIT_END  = 1,
    EDIT_OK = 2,
    EDIT_CANCEL   = 3,
    EDIT_INIT   = 4
};


enum
{
    IDCX_CLOUD = IDC_CLOUD,
    IDCX_TVSTATIC,
    IDCX_SEARCHBAR,
    IDCX_SESSIONTREEVIEW,
    IDCX_CLOUDTREEVIEW,
    IDCX_PROGRESS_STATIC,
    IDCX_PROGRESS_BAR,
    IDCX_PROGRESS_CANCEL_BTN,
    IDCX_STDBASE,
    IDCX_PANELBASE = IDCX_STDBASE + 32
};

struct treeview_faff
{
    HWND treeview;
    HTREEITEM lastat[128];
};

extern const BYTE ANDmaskCursor[] ;
extern const BYTE XORmaskCursor[];

static HCURSOR hCopyCurs = NULL;
static const int SESSION_TREEVIEW_WIDTH = 160;
static const int TREEVIEW_HEIGHT = 269;
static RECT dlgMonitorRect;

extern Conf* cfg;		       /* defined in window.c */

#define GRP_COLLAPSE_SETTING "GroupCollapse"

static void refresh_session_treeview(const char* select_session);
static void refresh_cloud_treeview(const char* select_session);
RECT getMaxWorkArea();
LPARAM get_selected_session(HWND hwndSess, char* const sess_name, const int name_len);

void upload_cloud_session(const string& session, const string& local_session);
int delete_cloud_session(const string& session);
void download_cloud_session(const string& session, const string& local_session);
void get_cloud_all_change_list(map<string, string>*& download_list, set<string>*& delete_list, map<string, string>*& upload_list);
std::map<std::string, std::string>& get_cloud_session_id_map();
static int edit_cloudsession_treeview(HWND hwndSess, int eflag);

static void SaneEndDialog(HWND hwnd, int ret)
{
    SetWindowLongPtr(hwnd, BOXRESULT, ret);
    SetWindowLongPtr(hwnd, BOXFLAGS, DF_END);
}

void process_in_ui_jobs();
void ErrorExit(char * str) ;
static int SaneDialogBox(HINSTANCE hinst,
                         LPCTSTR tmpl,
                         HWND hwndparent,
                         DLGPROC lpDialogFunc)
{
    WNDCLASS wc;
    HWND hwnd;
    MSG msg;
    int flags;
    int ret;
    int gm;

    wc.style = CS_DBLCLKS | CS_SAVEBITS | CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc = DefDlgProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = DLGWINDOWEXTRA + 2*sizeof(LONG_PTR);
    wc.hInstance = hinst;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND +1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "PuTTY-ND2_CloudSessions";
    RegisterClassA(&wc);

    hwnd = CreateDialog(hinst, tmpl, hwndparent, lpDialogFunc);
    if (hwnd == NULL)
    {
        ErrorExit("PuTTY-ND2_CloudSessions");
        return -1;
    }
    extern HWND hCloudWnd;
    hCloudWnd = hwnd;

    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(hwnd, SW_SHOW);

    bringToForeground(hwnd);
    SetWindowLongPtr(hwnd, BOXFLAGS, 0); /* flags */
    SetWindowLongPtr(hwnd, BOXRESULT, 0); /* result from SaneEndDialog */

    while ((gm=GetMessage(&msg, NULL, 0, 0)) > 0)
    {
        if(msg.message == WM_KEYUP)
        {
        }
        else if (msg.message == WM_KEYDOWN)
        {
            if (msg.wParam == VK_CONTROL)
            {
            }
            else if (msg.wParam == VK_F2)
            {
                HWND hwndSess = GetDlgItem(hwnd, IDCX_SESSIONTREEVIEW);
                TreeView_EditLabel(hwndSess, TreeView_GetSelection(hwndSess));
                continue;
            }
            if (msg.wParam == VK_RETURN)
            {
                if (edit_cloudsession_treeview(GetDlgItem(hwnd, IDCX_CLOUDTREEVIEW), EDIT_OK))
                {
                    continue;
                }
            }
            if (msg.wParam == VK_ESCAPE)
            {
                if (edit_cloudsession_treeview(GetDlgItem(hwnd, IDCX_CLOUDTREEVIEW), EDIT_CANCEL))
                {
                    continue;

                }
                else if (ctrlbox->cancelbutton != NULL)
                {
                    struct winctrl *wc = dlg_findbyctrl(&dp, ctrlbox->cancelbutton);
                    winctrl_handle_command(&dp,
                                           WM_COMMAND,
                                           MAKEWPARAM(wc->base_id, BN_CLICKED),
                                           0);
                    if (dp.ended)
                    {
                        SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
                        break;
                    }
                    continue;
                }
            }
            if (msg.wParam == VK_DOWN)
            {
                SetFocus(GetDlgItem(hwnd,IDCX_SESSIONTREEVIEW));
            }
        }
        process_in_ui_jobs();

        flags=GetWindowLongPtr(hwnd, BOXFLAGS);
        if (!(flags & DF_END) && !IsDialogMessage(hwnd, &msg))
            DispatchMessage(&msg);
        if (flags & DF_END)
            break;
    }

    if (gm == 0)
        PostQuitMessage(msg.wParam); /* We got a WM_QUIT, pass it on */

    HMONITOR mon;
    MONITORINFO mi;
    mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(mon, &mi);
    dlgMonitorRect = mi.rcWork;

    ret=GetWindowLongPtr(hwnd, BOXRESULT);
    DestroyWindow(hwnd);
    hCloudWnd = NULL;
    SetActiveWindow(hwndparent);
    return ret;
}

extern HTREEITEM session_treeview_insert(struct treeview_faff *faff,
        int level, char *text, LPARAM flags, bool changed = false, bool isSavedCmd = false);
extern const char* extract_last_part(const char* session);
extern void extract_group(const char* session,
                          char* group, const int glen);
extern LPARAM conv_tv_to_sess(
    HWND hwndSess, HTREEITEM hitem,
    char* const sess_name, const int name_len);
extern LPARAM get_selected_session(HWND hwndSess, char* const sess_name, const int name_len);


/*
 * Create the session tree view.
 */
static HWND create_session_treeview(HWND hwnd, struct treeview_faff* tvfaff)
{
    RECT r;
    WPARAM font;
    HWND tvstatic;
    HWND sessionview;
    HIMAGELIST hImageList;
    HBITMAP hBitMap;

    r.left = 3;
    r.right = r.left + SESSION_TREEVIEW_WIDTH - 6;
    r.top = 3;
    r.bottom = r.top + 10;
    MapDialogRect(hwnd, &r);
    const int SEARCH_TEXT_LEN = SESSION_TREEVIEW_WIDTH;
    tvstatic = CreateWindowEx(0, "STATIC", "Local Sessions",
                              WS_CHILD | WS_VISIBLE,
                              r.left, r.top,
                              SEARCH_TEXT_LEN, r.bottom - r.top,
                              hwnd, (HMENU) IDCX_TVSTATIC, hinst,
                              NULL);
    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
    SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

    r.left = 3;
    r.right = r.left + SESSION_TREEVIEW_WIDTH - 6;
    r.top = 13;
    r.bottom = r.top + TREEVIEW_HEIGHT;
    MapDialogRect(hwnd, &r);
    sessionview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
                                 WS_CHILD | WS_VISIBLE |
                                 WS_TABSTOP | TVS_HASLINES |
                                 TVS_HASBUTTONS | TVS_LINESATROOT |
                                 TVS_SHOWSELALWAYS, r.left, r.top,
                                 r.right - r.left, r.bottom - r.top,
                                 hwnd, (HMENU) IDCX_SESSIONTREEVIEW, hinst,
                                 NULL);
    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
    SendMessage(sessionview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
    tvfaff->treeview = sessionview;
    memset(tvfaff->lastat, 0, sizeof(tvfaff->lastat));

    hImageList = ImageList_Create(16,16,ILC_COLOR16,8,10);
    hBitMap = LoadBitmap(hinst,MAKEINTRESOURCE(IDB_TREE));
    ImageList_Add(hImageList,hBitMap,NULL);
    DeleteObject(hBitMap);
    SendDlgItemMessage(hwnd, IDCX_SESSIONTREEVIEW, TVM_SETIMAGELIST,0,(LPARAM)hImageList);

    return sessionview;
}

static HWND create_cloud_treeview(HWND hwnd, struct treeview_faff* tvfaff)
{
    RECT r;
    WPARAM font;
    HWND tvstatic;
    HWND sessionview;
    HIMAGELIST hImageList;
    HBITMAP hBitMap;
    RECT rd;
    GetWindowRect(hwnd, &rd);

    r.left = SESSION_TREEVIEW_WIDTH + 50 + 3;
    r.right = r.left + SESSION_TREEVIEW_WIDTH - 6;
    r.top = 3;
    r.bottom = r.top + 10;
    MapDialogRect(hwnd, &r);
    const int SEARCH_TEXT_LEN = SESSION_TREEVIEW_WIDTH;
    tvstatic = CreateWindowEx(0, "STATIC", "Remote Sessions",
                              WS_CHILD | WS_VISIBLE,
                              r.left, r.top,
                              SEARCH_TEXT_LEN, r.bottom - r.top,
                              hwnd, (HMENU)IDCX_TVSTATIC, hinst,
                              NULL);
    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
    SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

    r.left = SESSION_TREEVIEW_WIDTH + 50 + 3;
    r.right = r.left + SESSION_TREEVIEW_WIDTH - 6;
    r.top = 13;
    r.bottom = r.top + TREEVIEW_HEIGHT;
    MapDialogRect(hwnd, &r);
    sessionview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
                                 WS_CHILD | WS_VISIBLE |
                                 WS_TABSTOP | TVS_HASLINES |
                                 TVS_HASBUTTONS | TVS_LINESATROOT | TVS_EDITLABELS |
                                 TVS_SHOWSELALWAYS, r.left, r.top,
                                 r.right - r.left, r.bottom - r.top,
                                 hwnd, (HMENU)IDCX_CLOUDTREEVIEW, hinst,
                                 NULL);
    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
    SendMessage(sessionview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
    tvfaff->treeview = sessionview;
    memset(tvfaff->lastat, 0, sizeof(tvfaff->lastat));

    hImageList = ImageList_Create(16, 16, ILC_COLOR16, 8, 10);
    hBitMap = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_TREE));
    ImageList_Add(hImageList, hBitMap, NULL);
    DeleteObject(hBitMap);
    SendDlgItemMessage(hwnd, IDCX_CLOUDTREEVIEW, TVM_SETIMAGELIST, 0, (LPARAM)hImageList);

    return sessionview;
}

void dlg_show_controlset(struct controlset *ctrlset, void *dlg, const int show);
void show_cloud_progress_bar(bool is_show)
{
    dlg_show_controlset(middle_btn_controlset, &dp, !is_show);
    dlg_show_controlset(bottom_btn_controlset, &dp, !is_show);

    ShowWindow(GetDlgItem(dp.hwnd, IDCX_PROGRESS_STATIC), is_show ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(dp.hwnd, IDCX_PROGRESS_BAR), is_show ? SW_SHOW : SW_HIDE);
}

void on_progress_bar_done(void *ctx)
{
    if (hCloudWnd == NULL)
    {
        return;
    }
    show_cloud_progress_bar(false);
    refresh_cloud_treeview("");
    refresh_session_treeview("");
}

void set_progress_bar(const std::string& msg, int progress)
{
    show_cloud_progress_bar(true);
    SetDlgItemText(dp.hwnd, IDCX_PROGRESS_STATIC, msg.c_str());
    SendMessage(GetDlgItem(dp.hwnd, IDCX_PROGRESS_BAR), PBM_SETPOS, progress, (LPARAM)0);
    //refresh_cloud_treeview("");

    if (progress == 100 && hCloudWnd != NULL)
    {
        extern void* schedule_ui_timer(int ms, void(*callback)(void*), void* arg);
        schedule_ui_timer(100, on_progress_bar_done, NULL);
    }
}

static HWND create_progress_bar(HWND hwnd)
{
    RECT r;
    WPARAM font;
    HWND tvstatic;
    HWND sessionview;
    RECT rd;
    GetWindowRect(hwnd, &rd);

    r.left = 3;
    r.right = r.left + SESSION_TREEVIEW_WIDTH - 6;
    r.top = TREEVIEW_HEIGHT + 16;
    r.bottom = r.top + 10;
    MapDialogRect(hwnd, &r);
    const int TEXT_LEN = SESSION_TREEVIEW_WIDTH * 3;
    tvstatic = CreateWindowEx(0, "STATIC", "",
                              WS_CHILD | WS_VISIBLE,
                              r.left, r.top,
                              TEXT_LEN, r.bottom - r.top,
                              hwnd, (HMENU)IDCX_PROGRESS_STATIC, hinst,
                              NULL);
    font = SendMessage(hwnd, WM_GETFONT, 0, 0);
    SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));

    r.left = 3;
    r.right = r.left + 366;
    r.top = TREEVIEW_HEIGHT + 26;
    r.bottom = r.top + 4;
    MapDialogRect(hwnd, &r);
    sessionview = CreateWindowEx(WS_EX_CLIENTEDGE, PROGRESS_CLASS, "",
                                 WS_CHILD | WS_VISIBLE , r.left, r.top,
                                 r.right - r.left, r.bottom - r.top,
                                 hwnd, (HMENU)IDCX_PROGRESS_BAR, hinst,
                                 NULL);
    SendMessage(sessionview, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); //���ý������ķ�Χ
    SendMessage(sessionview, PBS_MARQUEE, 1, 0); //����PBS_MARQUEE �ǹ���Ч��
    SendMessage(sessionview, PBM_SETPOS, 1, (LPARAM)0);   //���ý���
    //SendMessage(hwnd, PBM_GETRANGE, TRUE, (LPARAM)&range);  //��ȡ�������ķ�Χ

    r.left = r.right + 3;
    r.right = r.left + 15;
    r.top = TREEVIEW_HEIGHT + 10;
    r.bottom = r.top + 15;
    MapDialogRect(hwnd, &r);
    sessionview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_BUTTON, "",
                                 WS_CHILD | WS_VISIBLE, r.left, r.top,
                                 r.right - r.left, r.bottom - r.top,
                                 hwnd, (HMENU)IDCX_PROGRESS_BAR, hinst,
                                 NULL);

    show_cloud_progress_bar(false);
    return sessionview;
}

extern int sessioncmp(const void *av, const void *bv);
bool session_buf_cmp(const char* a, const char* b)
{
    return sessioncmp(&a, &b) < 0;
}

bool session_buf_cmp_eq(const char* a, const char* b)
{
    return sessioncmp(&a, &b) == 0;
}
/*
 * Set up the session view contents.
 */

static void refresh_session_treeview(const char* select_session)
{
    HWND sessionview = GetDlgItem(dp.hwnd, IDCX_SESSIONTREEVIEW);
    struct treeview_faff tv_faff_struct;
    struct treeview_faff* tvfaff = &tv_faff_struct;

    HTREEITEM hfirst = NULL;
    HTREEITEM item = NULL;
    int i, j, k;               //index to iterator all the characters of the sessions
    int level;              //tree item's level
    int b;                  //index of the tree item's first character
    char itemstr[64];
    char selected_session_name[256] = {0};
    char lower_session_name[256] = { 0 };
    char pre_show_session_name[256] = { 0 };
    struct sesslist sesslist;
    int is_select;
    char session[256] = {0};

    isFreshingSessionTreeView = true;
    tvfaff->treeview = sessionview;
    memset(tvfaff->lastat, 0, sizeof(tvfaff->lastat));
    TreeView_DeleteAllItems(tvfaff->treeview);

    get_sesslist(&sesslist, TRUE);
    extern bool not_to_upload(const char* session_name);
    std::vector<const char*> all_cloud_session;
    for (i = 0; i < sesslist.nsessions; i++)
    {
        if (!sesslist.sessions[i][0])
            continue;

        if (not_to_upload(sesslist.sessions[i]))
        {
            continue;
        }
        all_cloud_session.push_back(sesslist.sessions[i]);
    }

    vector<string> cmdlist;
    get_cmdlist(cmdlist);
    for (int i = 0; i < cmdlist.size(); i++)
    {
        if (cannot_save_cmd(cmdlist[i].c_str()))
        {
            continue;
        }

        cmdlist[i] = saved_cmd_settings_folder + cmdlist[i];
        all_cloud_session.push_back(cmdlist[i].c_str());
    }

    map<string, string>* download_list = NULL;
    set<string>* delete_list = NULL;
    map<string, string>* upload_list = NULL;
    get_cloud_all_change_list(download_list, delete_list, upload_list);
    std::map<std::string, std::string>::iterator itDownload = download_list->begin();
    set<string> changed_sessions;
    for (; itDownload != download_list->end(); itDownload++)
    {
        all_cloud_session.push_back(itDownload->second.c_str());
        changed_sessions.insert(itDownload->second);
    }
    std::sort(all_cloud_session.begin(), all_cloud_session.end(), session_buf_cmp);
    std::vector<const char*>::iterator sortLast = std::unique(all_cloud_session.begin(), all_cloud_session.end(), session_buf_cmp_eq);
    all_cloud_session.resize(std::distance(all_cloud_session.begin(), sortLast));

    std::vector<const char*>::iterator it = all_cloud_session.begin();
    for (; it != all_cloud_session.end(); it++)
    {
        const char* session_name = *it;

        is_select = !strcmp(session_name, select_session);
        strncpy(session, session_name, sizeof(session));
        bool is_session_changed = changed_sessions.find(session_name) != changed_sessions.end();

        level = 0;
        b = 0;
        for (j = 0, k = 0; session_name[j] && level < 10; j++, k++)
        {
            if (session_name[j] == '#')
            {
                if (b == j)
                {
                    b = j + 1;
                    continue;
                }

                level++;
                if (i == 0 || strncmp(pre_show_session_name, session_name, j + 1))
                {
                    int len = (j - b) > 63 ? 63 : (j-b);
                    strncpy(itemstr, session_name + b, len);
                    itemstr[len] = '\0';
                    item = session_treeview_insert(tvfaff, level - 1, itemstr, SESSION_GROUP, is_session_changed);

                    TreeView_Expand(tvfaff->treeview, item,  TVE_COLLAPSE);

                }
                b = j + 1;
            }
        }
        strncpy(pre_show_session_name, session_name, sizeof(pre_show_session_name));
        if (is_session_changed)
        {
            for (int level_index = 0; level_index < level - 1; level_index++)
            {
                TreeView_Expand(tvfaff->treeview, tvfaff->lastat[level_index], TVE_EXPAND);
            }
        }

        if (b == j)
        {
            if (is_select)
            {
                hfirst = item;
                strncpy(selected_session_name, session_name, sizeof(selected_session_name));
            }
            if (!hfirst)
            {
                hfirst = item;
                strncpy(selected_session_name, session_name, sizeof(selected_session_name));
            }
            continue;
        }
        item = session_treeview_insert(tvfaff, level, const_cast<char*>(session_name + b), SESSION_ITEM, is_session_changed, is_cmd_session(session_name));

        if (is_select)
        {
            hfirst = item;
            strncpy(selected_session_name, session_name, sizeof(selected_session_name));
        }

        if (!hfirst)
        {
            hfirst = item;
            strncpy(selected_session_name, session_name, sizeof(selected_session_name));
        }
    }
    isFreshingSessionTreeView = false;
    InvalidateRect(sessionview, NULL, TRUE);
    if (hfirst)
    {
        TreeView_SelectItem(sessionview, hfirst);
    }
    else
    {
        dlg_refresh(NULL, &dp);
    }
    get_sesslist(&sesslist, FALSE);
}

/*
* Set up the session view contents.
*/
struct StrCompare
{
    // functor for operator<
    bool operator()(const char* _Left, const char* _Right) const
    {
        // apply operator< to operands
        return strcmp(_Left, _Right) < 0;
    }
};
static bool strpre(const char*a, const char*b)
{
    return strcmp(a, b) == 0;
}
static void refresh_cloud_treeview(const char* select_session)
{
    HWND sessionview = GetDlgItem(hCloudWnd, IDCX_CLOUDTREEVIEW);
    struct treeview_faff tv_faff_struct;
    struct treeview_faff* tvfaff = &tv_faff_struct;
    HTREEITEM hfirst = NULL;
    HTREEITEM item = NULL;
    int j, k;               //index to iterator all the characters of the sessions
    int level;              //tree item's level
    int b;                  //index of the tree item's first character
    char itemstr[64];
    char selected_session_name[256] = { 0 };
    char pre_show_session_name[256] = { 0 };
    int is_select;
    char session[256] = { 0 };

    tvfaff->treeview = sessionview;
    memset(tvfaff->lastat, 0, sizeof(tvfaff->lastat));
    TreeView_DeleteAllItems(tvfaff->treeview);

    std::map<std::string, std::string>& cloud_session_id_map = get_cloud_session_id_map();
    map<string, string>* download_list = NULL;
    set<string>* delete_list = NULL;
    map<string, string>* upload_list = NULL;
    get_cloud_all_change_list(download_list, delete_list, upload_list);

    std::set<const char*, StrCompare> changed_list;
    std::vector<const char*> all_cloud_session;
    std::map<std::string, std::string>::iterator itExist = cloud_session_id_map.begin();
    for (; itExist != cloud_session_id_map.end(); itExist++)
    {
        if (delete_list->find(itExist->first) == delete_list->end())
        {
            all_cloud_session.push_back(itExist->first.c_str());
        }
        else
        {
            changed_list.insert(itExist->first.c_str());
        }
    }

    std::map<std::string, std::string>::iterator itUpload = upload_list->begin();
    for (; itUpload != upload_list->end(); itUpload++)
    {
        all_cloud_session.push_back(itUpload->first.c_str());
        changed_list.insert(itUpload->first.c_str());
    }
    std::sort(all_cloud_session.begin(), all_cloud_session.end(), session_buf_cmp);
    std::vector<const char*>::iterator sortLast = std::unique(all_cloud_session.begin(), all_cloud_session.end(), session_buf_cmp_eq);
    all_cloud_session.resize(std::distance(all_cloud_session.begin(), sortLast));

    extern bool not_to_upload(const char* session_name);
    std::vector<const char*>::iterator it = all_cloud_session.begin();
    for (; it != all_cloud_session.end(); it++)
    {
        const char* session_name = *it;

        is_select = !strcmp(session_name, select_session);
        strncpy(session, session_name, sizeof(session));

        bool is_session_changed = changed_list.find(session_name) != changed_list.end();
        //if (is_session_changed){ is_pre_group_expend = true; }

        level = 0;
        b = 0;
        for (j = 0, k = 0; session_name[j] && level < 10; j++, k++)
        {
            if (session_name[j] == '#')
            {
                if (b == j)
                {
                    b = j + 1;
                    continue;
                }

                level++;
                if (it == all_cloud_session.begin() || strncmp(pre_show_session_name, session_name, j + 1))
                {
                    int len = (j - b) > 63 ? 63 : (j - b);
                    strncpy(itemstr, session_name + b, len);
                    itemstr[len] = '\0';
                    item = session_treeview_insert(tvfaff, level - 1, itemstr, SESSION_GROUP, is_session_changed);

                    //we can only expand a group with a child
                    //so we expand the previous group
                    //leave the group in tail alone.
                    TreeView_Expand(tvfaff->treeview, item, TVE_COLLAPSE);

                    char grp_session[256] = { 0 };
                    strncpy(grp_session, session, j + 1);

                }
                b = j + 1;
            }
        }
        strncpy(pre_show_session_name, session_name, sizeof(pre_show_session_name));
        if (is_session_changed)
        {
            for (int level_index = 0; level_index < level - 1; level_index++)
            {
                TreeView_Expand(tvfaff->treeview, tvfaff->lastat[level_index], TVE_EXPAND);
            }
        }

        if (b == j)
        {
            if (is_select)
            {
                hfirst = item;
                strncpy(selected_session_name, session_name, sizeof(selected_session_name));
            }
            continue;
        }
        item = session_treeview_insert(tvfaff, level, const_cast<char*>(session_name + b), SESSION_ITEM, is_session_changed, is_cmd_session(session_name));
    }

    InvalidateRect(sessionview, NULL, TRUE);
    if (hfirst)
    {
        TreeView_SelectItem(sessionview, hfirst);
    }
    else
    {
        dlg_refresh(NULL, &dp);
    }
}

/*
 * copy session, return FALSE if to_session exist
 */
extern int copy_session(
    struct sesslist* sesslist,
    const char* from_session,
    const char* to_session,
    int to_sess_flag);

extern int copy_item_under_group(const char* session_name, const char* origin_group, const char* dest_group);
extern void dup_session_treeview(
    HWND hwndSess,
    HTREEITEM selected_item,
    const char* from_session,
    const char* to_session_pre,
    int from_sess_flag,
    int to_sess_flag);

static void create_controls(HWND hwnd)
{
    char path[128] = { 0 };
    struct ctlpos cp;
    int index;
    int base_id;
    struct winctrls *wc;

    ctlposinit(&cp, hwnd, 3, 3, 16);
    wc = &ctrls_base;
    base_id = IDCX_STDBASE;

    for (index = -1; (index = ctrl_find_path(ctrlbox, path, index)) >= 0;)
    {
        struct controlset *s = ctrlbox->ctrlsets[index];
        winctrl_layout(&dp, wc, &cp, s, &base_id);
    }
}

static int edit_cloudsession_treeview(HWND hwndSess, int eflag)
{
    char buffer[256] = { 0 };

    char pre_session[512] = { 0 };
    char itemstr[64] = { 0 };
    char to_session[256] = { 0 };
    int i = 0;
    int pos = 0;
    char* c = NULL;
    TVITEM item;
    HTREEITEM hi;
    int sess_flags = SESSION_NONE;

    if (!hwndSess)
        return FALSE;

    switch (eflag)
    {
    case EDIT_INIT:
        hEdit = NULL;
        return TRUE;
        break;
    case EDIT_BEGIN:
    {
        extern bool is_doing_cloud_action();
        if (is_doing_cloud_action())
        {
            TreeView_EndEditLabelNow(hwndSess, TRUE);
            return false;
        }
        map<string, string>* download_list = NULL;
        set<string>* delete_list = NULL;
        map<string, string>* upload_list = NULL;
        get_cloud_all_change_list(download_list, delete_list, upload_list);
        sess_flags = get_selected_session(hwndSess, pre_session, sizeof(pre_session));
        if (upload_list->find(pre_session) == upload_list->end())
        {
            MessageBox(GetParent(hwndSess), "It is complexed and sometimes not safe to rename the exist sessions.\nPlease download, delete, rename and then upload", "Error", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
            TreeView_EndEditLabelNow(hwndSess, TRUE);
            return false;
        }

        hEdit = TreeView_GetEditControl(hwndSess);
        return TRUE;
    }
    break;

    case EDIT_CANCEL:
        hEdit = NULL;
        TreeView_EndEditLabelNow(hwndSess, TRUE);
        return TRUE;
        break;

    case EDIT_OK:
        TreeView_EndEditLabelNow(hwndSess, FALSE);
        return TRUE;
        break;

    case EDIT_END:
        GetWindowText(hEdit, buffer, sizeof(buffer));

        /*validate the buffer*/
        if (buffer[0] == '\0')
            return TRUE;
        for (i = 0; i < strlen(buffer); i++)
        {
            if (buffer[i] == '#' || buffer[i] == '/' || buffer[i] == '\\')
                buffer[i] = '%';
        }
        buffer[i] = '\0';

        /* if no changed, return */
        hi = TreeView_GetSelection(hwndSess);
        item.hItem = hi;
        item.pszText = itemstr;
        item.cchTextMax = sizeof(itemstr);
        item.mask = TVIF_TEXT | TVIF_PARAM;
        TreeView_GetItem(hwndSess, &item);
        if (!strcmp(item.pszText, buffer))
        {
            hEdit = NULL;
            return TRUE;
        }

        sess_flags = get_selected_session(hwndSess, pre_session, sizeof(pre_session));
        /* calc the to_session */
        strncpy(to_session, pre_session, sizeof to_session);
        c = strrchr(to_session, '#');
        if (c)
        {
            *(c + 1) = '\0';
        }
        if (sess_flags == SESSION_GROUP)
        {
            *c = '\0';
            c = strrchr(to_session, '#');
            if (c)
            {
                *(c + 1) = '\0';
            }
        }
        pos = c ? (c - to_session + 1) : 0;
        strncpy(to_session + pos, buffer, sizeof(to_session)-pos - 2);
        if (sess_flags == SESSION_GROUP)
        {
            strcat(to_session, "#");
        }

        map<string, string>* download_list = NULL;
        set<string>* delete_list = NULL;
        map<string, string>* upload_list = NULL;
        get_cloud_all_change_list(download_list, delete_list, upload_list);

        map<string, string>::iterator it = upload_list->find(pre_session);
        if (it == upload_list->end())
        {
            MessageBox(GetParent(hwndSess), "It is complexed and sometimes not safe to rename the exist sessions.\nPlease download, delete, rename and then upload", "Error", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
            return FALSE;
        }
        map<string, string>::iterator it2 = upload_list->find(to_session);
        if (it2 != upload_list->end())
        {
            MessageBox(GetParent(hwndSess), "Session already exist!", "Error", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
            return FALSE;
        }
        if (sess_flags == SESSION_ITEM)
        {
            (*upload_list)[to_session] = it->second;
            upload_list->erase(it);
        }
        else
        {
            map<string, string>::iterator it = upload_list->begin();
            vector<string> delete_keys;
            vector<string> new_keys;
            vector<string> values;
            int pre_session_len = strlen(pre_session);
            for (; it != upload_list->end(); it++)
            {
                if (strncmp(pre_session, it->first.c_str(), pre_session_len) == 0)
                {
                    delete_keys.push_back(it->first);
                    values.push_back(it->second);
                    new_keys.push_back(to_session + it->first.substr(pre_session_len));
                }
            }
            for (int i = 0; i < delete_keys.size(); i++)
            {
                upload_list->erase(delete_keys[i]);
            }
            for (int i = 0; i < new_keys.size(); i++)
            {
                upload_cloud_session(new_keys[i], values[i]);
            }
        }

        //MessageBox(GetParent(hwndSess), "Destination session already exists.", "Error", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        hEdit = NULL;

        refresh_cloud_treeview(to_session);

        break;
        return TRUE;
    }
    return FALSE;
}

/*
 * This function is the configuration box.
 * (Being a dialog procedure, in general it returns 0 if the default
 * dialog processing should be performed, and 1 if it should not.)
 */
static int CALLBACK GenericMainDlgProc(HWND hwnd, UINT msg,
                                       WPARAM wParam, LPARAM lParam)
{
    HWND hw, sessionview, cloudtreeview;
    struct treeview_faff tvfaff;
    int ret;

    if (msg == WM_INITDIALOG)
    {
        dp.hwnd = hwnd;
        {
            RECT rs, rd;
            rs = getMaxWorkArea();
            if (GetWindowRect(hwnd, &rd))
            {
                rd.right = rd.left + SESSION_TREEVIEW_WIDTH * 2 + 54;
                rd.bottom = rd.top + TREEVIEW_HEIGHT + 50;
                MapDialogRect(hwnd, &rd);
                MoveWindow(hwnd,
                           (rs.right + rd.left - rd.right),
                           (rs.bottom + rs.top + rd.top - rd.bottom) / 2,
                           rd.right - rd.left, rd.bottom - rd.top, TRUE);
            }
        }
        create_controls(hwnd);     /* buttons etc */
        SetWindowText(hwnd, dp.wintitle);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_BIG,
                    (LPARAM)LoadIcon(hinst, MAKEINTRESOURCE(IDI_CFGICON)));

        sessionview = create_session_treeview(hwnd, &tvfaff);
        refresh_session_treeview(DEFAULT_SESSION_NAME);

        cloudtreeview = create_cloud_treeview(hwnd, &tvfaff);
        refresh_cloud_treeview("");

        create_progress_bar(hwnd);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, 1);

        extern void get_remote_file();
        get_remote_file();
        //set_progress_bar("done", 100);
        return 0;
    }
    else if (msg == WM_NOTIFY)
    {
        if (LOWORD(wParam) == IDCX_SESSIONTREEVIEW
                && !isFreshingSessionTreeView)
        {
            switch(((LPNMHDR) lParam)->code)
            {
            case TVN_SELCHANGED:
                break;

            case NM_DBLCLK:
                break;
            case TVN_KEYDOWN:
                break;

            case TVN_BEGINLABELEDIT:
                break;

            case TVN_ENDLABELEDIT:
                break;

            case NM_RCLICK:
                break;

            case TVN_BEGINDRAG:
                break;

            case TVN_ITEMEXPANDED:
                break;
            default:
                break;

            };//switch
            return 0;
        }
        else if(LOWORD(wParam) == IDCX_CLOUDTREEVIEW
                && !isFreshingSessionTreeView)
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case TVN_SELCHANGED:
                break;

            case NM_DBLCLK:
                break;
            case TVN_KEYDOWN:
                break;

            case TVN_BEGINLABELEDIT:
                edit_cloudsession_treeview(((LPNMHDR)lParam)->hwndFrom, EDIT_BEGIN);
                break;

            case TVN_ENDLABELEDIT:
                edit_cloudsession_treeview(((LPNMHDR)lParam)->hwndFrom, EDIT_END);
                break;

            case NM_RCLICK:
                break;

            case TVN_BEGINDRAG:
                break;

            case TVN_ITEMEXPANDED:
                break;
            default:
                break;

            };//switch
            return 0;
        }
        {
            return winctrl_handle_command(&dp, msg, wParam, lParam);
        }
    }
    else if (msg == WM_CLOSE)
    {
        SaneEndDialog(hwnd, 0);
        return 0;
    }
    else if (msg == WM_SIZE)
    {
        if (wParam == SIZE_MAXIMIZED)
            force_normal(hwnd);
        return 0;
    }
    else
    {
        /*
         * Only process WM_COMMAND once the dialog is fully formed.
         */
        if (GetWindowLongPtr(hwnd, GWLP_USERDATA) == 1)
        {
            ret = winctrl_handle_command(&dp, msg, wParam, lParam);
            if (dp.ended && GetCapture() != hwnd)
                SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
        }
        else
            ret = 0;
        return ret;
    }
    return 0;
}

static int upload_session_to_clould_group(const char* session_name, const char* origin_group, const char* dest_group)
{
    extern bool not_to_upload(const char* session_name);
    if (not_to_upload(session_name))
    {
        return 0;
    }

    if (is_cmd_session(session_name))
    {
        upload_cloud_session(session_name, session_name);
    }
    else
    {
        char remote_session[512] = { 0 };
        snprintf(remote_session, sizeof(remote_session) - 1, "%s%s", dest_group, session_name + strlen(origin_group));

        upload_cloud_session(remote_session, session_name);
    }
    return 1;
}

static void on_upload_all_sessions(union control *ctrl, void *dlg,
                                   void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }
    HWND hwndCldSess = GetDlgItem(dp.hwnd, IDCX_CLOUDTREEVIEW);
    HTREEITEM hcloudItem = TreeView_GetSelection(hwndCldSess);
    char cloud_session[512] = { 0 };
    int cloud_sess_flags = conv_tv_to_sess(hwndCldSess, hcloudItem, cloud_session, sizeof(cloud_session)-1);
    char cloud_group[512] = { 0 };
    extract_group(cloud_session, cloud_group, sizeof(cloud_group)-1);

    struct sesslist sesslist;
    get_sesslist(&sesslist, TRUE);
    extern bool not_to_upload(const char* session_name);
    for (int i = 0; i < sesslist.nsessions; i++)
    {
        if (!sesslist.sessions[i][0])
            continue;

        if (not_to_upload(sesslist.sessions[i]))
        {
            continue;
        }
        upload_session_to_clould_group(sesslist.sessions[i], "", cloud_group);
    }
    get_sesslist(&sesslist, FALSE);

    vector<string> cmdlist;
    get_cmdlist(cmdlist);
    for (int i = 0; i < cmdlist.size(); i++)
    {
        if (cannot_save_cmd(cmdlist[i].c_str()))
        {
            continue;
        }

        cmdlist[i] = saved_cmd_settings_folder + cmdlist[i];
        upload_session_to_clould_group(cmdlist[i].c_str(), "", "");
    }
    refresh_cloud_treeview(cloud_group);
}

static void on_upload_selected_sessions(union control *ctrl, void *dlg,
                                        void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }
    HWND hwndSess = GetDlgItem(dp.hwnd, IDCX_SESSIONTREEVIEW);
    HTREEITEM hitem = TreeView_GetSelection(hwndSess);
    char sess_name[512] = { 0 };
    char sess_group[512] = { 0 };
    int local_sess_flags = conv_tv_to_sess(hwndSess, hitem, sess_name, sizeof(sess_name)-1);
    strcpy(sess_group, sess_name);
    if (local_sess_flags == SESSION_GROUP)
    {
        sess_group[strlen(sess_group) - 1] = '\0';
    }
    extract_group(sess_group, sess_group, sizeof(sess_group)-1);

    HWND hwndCldSess = GetDlgItem(dp.hwnd, IDCX_CLOUDTREEVIEW);
    HTREEITEM hcloudItem = TreeView_GetSelection(hwndCldSess);
    char cloud_session[512] = { 0 };
    int cloud_sess_flags = conv_tv_to_sess(hwndCldSess, hcloudItem, cloud_session, sizeof(cloud_session)-1);
    char cloud_group[512] = { 0 };
    extract_group(cloud_session, cloud_group, sizeof(cloud_group)-1);

    upload_session_to_clould_group(sess_name, sess_group, cloud_group);
    if (local_sess_flags == SESSION_GROUP)
    {
        for_grouped_session_do(sess_name, boost::bind(upload_session_to_clould_group, _1, sess_group, cloud_group), 100);
        if (is_cmd_session(sess_name))
        {
            vector<string> cmdlist;
            get_cmdlist(cmdlist);
            for (int i = 0; i < cmdlist.size(); i++)
            {
                if (cannot_save_cmd(cmdlist[i].c_str()))
                {
                    continue;
                }

                cmdlist[i] = saved_cmd_settings_folder + cmdlist[i];
                upload_session_to_clould_group(cmdlist[i].c_str(), "", "");
            }
        }
    }

    char select_session[512] = { 0 };
    snprintf(select_session, sizeof(select_session)-1, "%s%s", cloud_group, sess_name + strlen(sess_group));
    refresh_cloud_treeview(cloud_group);
}

static void on_download_all_sessions(union control *ctrl, void *dlg,
                                     void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }

    HWND hwndSess = GetDlgItem(dp.hwnd, IDCX_SESSIONTREEVIEW);
    HTREEITEM hitem = TreeView_GetSelection(hwndSess);
    char sess_name[512] = { 0 };
    char sess_group[512] = { 0 };
    int local_sess_flags = conv_tv_to_sess(hwndSess, hitem, sess_name, sizeof(sess_name)-1);
    strcpy(sess_group, sess_name);
    extract_group(sess_group, sess_group, sizeof(sess_group)-1);

    std::map<std::string, std::string>& cloud_session_id_map = get_cloud_session_id_map();
    std::map<std::string, std::string>::iterator itExist = cloud_session_id_map.begin();
    for (; itExist != cloud_session_id_map.end(); itExist++)
    {
        char local_session[512] = { 0 };
        snprintf(local_session, sizeof(local_session)-1, "%s%s", is_cmd_session(itExist->first.c_str()) ? "" : sess_group, itExist->first);
        download_cloud_session(itExist->first, local_session);
    }
    refresh_session_treeview(sess_group);
}

static void on_download_selected_sessions(union control *ctrl, void *dlg,
        void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }

    HWND hwndSess = GetDlgItem(dp.hwnd, IDCX_SESSIONTREEVIEW);
    HTREEITEM hitem = TreeView_GetSelection(hwndSess);
    char sess_name[512] = { 0 };
    char sess_group[512] = { 0 };
    int local_sess_flags = conv_tv_to_sess(hwndSess, hitem, sess_name, sizeof(sess_name)-1);
    strcpy(sess_group, sess_name);
    extract_group(sess_group, sess_group, sizeof(sess_group)-1);

    HWND hwndCldSess = GetDlgItem(dp.hwnd, IDCX_CLOUDTREEVIEW);
    HTREEITEM hcloudItem = TreeView_GetSelection(hwndCldSess);
    char cloud_session[512] = { 0 };
    int cloud_sess_flags = conv_tv_to_sess(hwndCldSess, hcloudItem, cloud_session, sizeof(cloud_session)-1);
    char cloud_group[512] = { 0 };
    if (!is_cmd_session(cloud_session))
    {
        strcpy(cloud_group, cloud_session);
        if (cloud_sess_flags == SESSION_GROUP)
        {
            cloud_group[strlen(cloud_group) - 1] = '\0';
        }
        extract_group(cloud_group, cloud_group, sizeof(cloud_group)-1);
    }

    if (cloud_sess_flags == SESSION_GROUP)
    {
        std::map<std::string, std::string>& cloud_session_id_map = get_cloud_session_id_map();
        std::map<std::string, std::string>::iterator itExist = cloud_session_id_map.begin();
        for (; itExist != cloud_session_id_map.end(); itExist++)
        {
            if (0 == strncmp(cloud_session, itExist->first.c_str(), strlen(cloud_session)))
            {
                char local_session[512] = { 0 };
                snprintf(local_session, sizeof(local_session)-1, "%s%s", is_cmd_session(itExist->first.c_str()) ? "" : sess_group, itExist->first.c_str() + strlen(cloud_group));
                download_cloud_session(itExist->first, local_session);
            }
        }
    }

    char local_session[512] = { 0 };
    snprintf(local_session, sizeof(local_session)-1, "%s%s", is_cmd_session(cloud_session) ? "" : sess_group, cloud_session + strlen(cloud_group));
    download_cloud_session(cloud_session, local_session);

    refresh_session_treeview(local_session);
}

static void download_full_session_path(union control *ctrl, void *dlg,
                                       void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }
}

static void apply_changes(union control *ctrl, void *dlg,
                          void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }
    extern void apply_cloud_changes();
    apply_cloud_changes();
}

static void reset_changes(union control *ctrl, void *dlg,
                          void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }
    extern void reset_cloud_changes();
    reset_cloud_changes();
    refresh_session_treeview("");
    refresh_cloud_treeview("");
}

static void new_cloud_session_folder(union control *ctrl, void *dlg,
                                     void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }
    char new_folder_name[512] = { 0 };
    HWND hwndCldSess = GetDlgItem(dp.hwnd, IDCX_CLOUDTREEVIEW);
    HTREEITEM hcloudItem = TreeView_GetSelection(hwndCldSess);
    char cloud_session[512] = { 0 };
    int cloud_sess_flags = conv_tv_to_sess(hwndCldSess, hcloudItem, cloud_session, sizeof(cloud_session)-1);
    char cloud_group[512] = { 0 };
    if (!is_cmd_session(cloud_session))
    {
        extract_group(cloud_session, cloud_group, sizeof(cloud_group) - 1);
    }

    map<string, string>* download_list = NULL;
    set<string>* delete_list = NULL;
    map<string, string>* upload_list = NULL;
    get_cloud_all_change_list(download_list, delete_list, upload_list);
    std::map<std::string, std::string>& cloud_session_id_map = get_cloud_session_id_map();
    int i = 1;
    while (true)
    {
        snprintf(new_folder_name, sizeof(new_folder_name)-1, "%sNew Folder %d#", cloud_group, i++);
        if (cloud_session_id_map.find(new_folder_name) == cloud_session_id_map.end() && upload_list->find(new_folder_name) == upload_list->end())
        {
            break;
        }
    }

    upload_cloud_session(new_folder_name, DEFAULT_SESSION_NAME);
    refresh_cloud_treeview(new_folder_name);
    TreeView_EditLabel(hwndCldSess, TreeView_GetSelection(hwndCldSess));
}

static void select_none_cloud_session(union control *ctrl, void *dlg,
                                      void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }
    HWND hwndCloud = GetDlgItem(dp.hwnd, IDCX_CLOUDTREEVIEW);
    TreeView_Select(hwndCloud, NULL, TVGN_CARET);
}

static void on_delete_cloud_session(union control *ctrl, void *dlg,
                                    void *data, int event)
{
    if (event != EVENT_ACTION)
    {
        return;
    }

    HWND hwndCldSess = GetDlgItem(dp.hwnd, IDCX_CLOUDTREEVIEW);
    HTREEITEM hcloudItem = TreeView_GetSelection(hwndCldSess);
    char cloud_session[512] = { 0 };
    int cloud_sess_flags = conv_tv_to_sess(hwndCldSess, hcloudItem, cloud_session, sizeof(cloud_session)-1);
    char cloud_group[512] = { 0 };
    extract_group(cloud_session, cloud_group, sizeof(cloud_group)-1);

    delete_cloud_session(cloud_session);
    if (cloud_sess_flags == SESSION_GROUP)
    {
        map<string, string>* download_list = NULL;
        set<string>* delete_list = NULL;
        map<string, string>* upload_list = NULL;
        get_cloud_all_change_list(download_list, delete_list, upload_list);
        std::map<std::string, std::string>& cloud_session_id_map = get_cloud_session_id_map();
        int cloud_session_len = strlen(cloud_session);

        map<string, string>::iterator itExist = cloud_session_id_map.begin();
        for (; itExist != cloud_session_id_map.end(); itExist++)
        {
            if (strncmp(cloud_session, itExist->first.c_str(), cloud_session_len) == 0)
            {
                delete_cloud_session(itExist->first);
            }
        }
        vector<string> delete_keys;
        map<string, string>::iterator it = upload_list->begin();
        for (; it != upload_list->end(); it++)
        {
            if (strncmp(cloud_session, it->first.c_str(), cloud_session_len) == 0)
            {
                delete_keys.push_back(it->first);
            }
        }
        for (int i = 0; i < delete_keys.size(); i++)
        {
            delete_cloud_session(delete_keys[i]);
        }

    }
    refresh_cloud_treeview("");
}

void setup_cloud_box(struct controlbox *b)
{
    struct controlset *s;
    union control *c, *bc;
    char *str;
    int i;

    s = ctrl_getset(b, "", "", "");
    middle_btn_controlset = s;
    ctrl_columns(s, 3, 43, 14, 43);
    c = ctrl_separator(s, I(10));
    c->generic.column = 1;
    c = ctrl_pushbutton(s,"all >>>", '\0',
                        HELPCTX(no_help),
                        on_upload_all_sessions, P(NULL));
    c->generic.column = 1;
    c = ctrl_separator(s, I(10));
    c->generic.column = 1;
    c = ctrl_pushbutton(s,"selected >", '\0',
                        HELPCTX(no_help),
                        on_upload_selected_sessions, P(NULL));
    c->generic.column = 1;
    c = ctrl_separator(s, I(10));
    c->generic.column = 1;
    c = ctrl_pushbutton(s,"<<< all", '\0',
                        HELPCTX(no_help),
                        on_download_all_sessions, P(NULL));
    c->generic.column = 1;
    c = ctrl_separator(s, I(10));
    c->generic.column = 1;
    c = ctrl_pushbutton(s,"< selected", '\0',
                        HELPCTX(no_help),
                        on_download_selected_sessions, P(NULL));
    c->generic.column = 1;
    c = ctrl_separator(s, I(10));
    c->generic.column = 1;
    c = ctrl_pushbutton(s, "apply", '\0',
                        HELPCTX(no_help),
                        apply_changes, P(NULL));
    c->generic.column = 1;
    c = ctrl_separator(s, I(10));
    c->generic.column = 1;
    c = ctrl_pushbutton(s, "reset", '\0',
                        HELPCTX(no_help),
                        reset_changes, P(NULL));
    c->generic.column = 1;
    c = ctrl_separator(s, I(108));
    c->generic.column = 1;

    s = ctrl_getset(b, "", "~bottom", "");
    bottom_btn_controlset = s;
    ctrl_columns(s, 7, 14, 14, 14, 15, 14, 15, 14);
    c = ctrl_pushbutton(s, "new folder", '\0',
                        HELPCTX(no_help),
                        new_cloud_session_folder, P(NULL));
    c->generic.column = 4;
    c = ctrl_pushbutton(s, "select none", '\0',
                        HELPCTX(no_help),
                        select_none_cloud_session, P(NULL));
    c->generic.column = 5;
    c = ctrl_pushbutton(s, "delete", '\0',
                        HELPCTX(no_help),
                        on_delete_cloud_session, P(NULL));
    c->generic.column = 6;

}

int do_cloud(void)
{
    int ret;

    if (hCloudWnd)
    {
        bringToForeground(hCloudWnd);
        return 0;
    }

    ctrlbox = ctrl_new_box();
    setup_cloud_box(ctrlbox);
    dp_init(&dp);
    winctrl_init(&ctrls_base);
    dp_add_tree(&dp, &ctrls_base);
    dp.wintitle = dupprintf("%s Cloud", appname);
    dp.errtitle = dupprintf("%s Error", appname);
    dp.data = cfg;
    dlg_auto_set_fixed_pitch_flag(&dp);
    dp.shortcuts['g'] = TRUE;	       /* the treeview: `Cate&gory' */

    ret = SaneDialogBox(hinst, MAKEINTRESOURCE(IDD_CLOUD_SESSION_BOX), NULL,
                        GenericMainDlgProc);

    ctrl_free_box(ctrlbox);
    ctrlbox = NULL;
    winctrl_cleanup(&ctrls_base);
    dp_cleanup(&dp);

    return ret;
}
