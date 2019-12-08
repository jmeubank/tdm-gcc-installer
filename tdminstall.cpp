/** \file TDMInstall.cpp
 *
 * Created: JohnE, 2008-06-25
 */


/*
DISCLAIMER:
The author(s) of this file's contents release it into the public domain, without
express or implied warranty of any kind. You may use, modify, and redistribute
this file freely.
*/


extern "C" {

#include <errno.h>
#include <direct.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <direct.h>
#include "tdminst_res.h"

} //extern "C"


#include <queue>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <tinyxml2.h>
#include "install_manifest.hpp"
#include "ref.hpp"
#include "componentstree.hpp"
#include "nsis_interface.hpp"
#include "config.h"

using namespace tinyxml2;

typedef char CharType;
typedef std::string StringType;
typedef RefType< tinyxml2::XMLDocument >::Ref XMLDocumentRef;


static HINSTANCE ginstance;
static RefType< struct _IMAGELIST >::Ref imglist;
static int nradio;
static int ncheck;
static HWND htree;
static WNDPROC oldCompWndProc, oldTreeWndProc, oldDirWndProc;
static HWND hitypecb = 0;
static HWND hdesclabel = 0;
static StringType inst_loc;
static RefType< InstallManifest >::Ref inst_man;
static HWND hinstfinddlg = 0;
struct CmpStrI
{
	bool operator () (const StringType& str1, const StringType& str2) const
	{
		return (stricmp(str1.c_str(), str2.c_str()) < 0);
	}
};
typedef std::set< StringType, CmpStrI > PrevInstSet;
static PrevInstSet prev_insts;
static HWND hdirpagedir = 0;
static HWND hdirlist;
static nsFunction previnstsel_callback = 0;
static HWND hsizelabel = 0;
static XMLDocumentRef working_man, prev_man;

static int num_addremove_ops;
static int cur_op_index;
static int num_archives_to_dl;

int cur_file_in_op_index = 0;
int num_files_in_cur_op = 0;

static ComponentsTree ctree;

static std::vector< XMLElement* > itypes_by_index;
static int cur_itype = -1;
static int itype_custom = -1, itype_current = -1;

static std::vector< StringType > mirrors;


static StringType FormatSize(unsigned size)
{
	unsigned divby = 1;
	const char* unit = "bytes";
	if (size >= 1024 * 1024 * 1024)
	{
		divby = 1024 * 1024 * 1024;
		unit = "GB";
	}
	else if (size >= 1024 * 1024)
	{
		divby = 1024 * 1024;
		unit = "MB";
	}
	else if (size >= 1024)
	{
		divby = 1024;
		unit = "KB";
	}

	float count = static_cast< float >(size) / divby;

	char ret[1024];
	if (count < 1000.0f)
		snprintf(ret, 1023, "%.3G %s", count, unit);
	else
		snprintf(ret, 1023, "%d %s", static_cast< int >(count), unit);
	ret[1023] = '\0';

	return ret;
}


static void MaybeUpdateSizeLabel()
{
	if (hsizelabel)
	{
		StringType ltext;
		if (ctree.GetDownloadSize() > 0)
			ltext += StringType("Download: ")
			 + FormatSize(ctree.GetDownloadSize()) + "; ";
		if (ctree.GetInstallSize() > 0)
		{
			ltext += StringType("Install: ")
			 + FormatSize(ctree.GetInstallSize());
		}
		if (ctree.GetUninstallSize() > 0)
		{
			if (ltext.length() > 0)
				ltext += "; ";
			ltext += StringType("Uninstall: ")
			 + FormatSize(ctree.GetUninstallSize());
		}
		if (ltext.length() <= 0)
			ltext = "(No changes)";
		Static_SetText(hsizelabel, ltext.c_str());
	}
}


static void UpdateDescLabel(HTREEITEM hitem, bool force = false)
{
	static HTREEITEM last_item = 0;

	if (!hdesclabel)
		return;

	if (hitem == last_item && !force)
		return;
	last_item = hitem;

	StringType desc;
	int index = ctree.GetIndex(hitem);
	if (index >= 0)
		desc = ctree.GetDescription(index);
	if (desc.length() > 0)
	{
		Static_SetText(hdesclabel, desc.c_str());
		Static_Enable(hdesclabel, TRUE);
	}
	else
	{
		Static_SetText(hdesclabel,
		 "Position your mouse over a component to see its description.");
		Static_Enable(hdesclabel, FALSE);
	}
}


/* recursive mkdir */
/* abort on ENOENT; ignore other errors like "directory already exists" */
/* return 1 if OK */
/*        0 on error */

static int makedir (const char* base, const char *newdir)
{
  char buffer[1024];
  int blen = strlen(strncpy(buffer, base, 1022));
  buffer[blen] = '/';
  int len = blen + strlen(strncpy(buffer + blen + 1, newdir, 1021 - blen));
  char *p;

  if (len <= 0)
    return 0;
  if (buffer[len-1] == '/')
    buffer[len-1] = '\0';
  if (_mkdir(buffer) == 0)
    return 1;

  p = buffer+1;
  while (1)
    {
      char hold;

      while(*p && *p != '\\' && *p != '/')
        p++;
      hold = *p;
      *p = 0;
      if ((_mkdir(buffer) == -1))
        {
          if (errno == ENOENT)
              return 0;
        }
      if (hold == 0)
        break;
      *p++ = hold;
    }
  return 1;
}


extern "C" void __declspec(dllexport) SetDescLabel
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	hdesclabel = (HWND)NSIS::popint();
	UpdateDescLabel(0, true);
}


static void UpdateInstallType();

static DWORD WINAPI CompPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == CBN_SELCHANGE && (HWND)lParam == hitypecb)
		{
			cur_itype = SendMessage(hitypecb, CB_GETCURSEL, 0, 0);
			if (cur_itype == CB_ERR)
				cur_itype = -1;
			else
				UpdateInstallType();
			return 0;
		}
		break;
	case WM_NOTIFY:
		{
			LPNMHDR lpnmh = (LPNMHDR)lParam;
			if (lpnmh->idFrom == 1300)
			{
				switch (lpnmh->code)
				{
				case NM_CLICK:
					{
						TVHITTESTINFO ht;
						DWORD dwpos = GetMessagePos();
						ht.pt.x = GET_X_LPARAM(dwpos);
						ht.pt.y = GET_Y_LPARAM(dwpos);
						ScreenToClient(htree, &ht.pt);
						(void)TreeView_HitTest(htree, &ht);
						if (ht.flags & TVHT_ONITEMSTATEICON)
						{
							if (ctree.OnStateToggle(ht.hItem))
							{
								cur_itype = itype_custom;
								SendMessage(hitypecb, CB_SETCURSEL, itype_custom, 0);
								UpdateInstallType();
							}
						}
					}
					return 0;
				case TVN_SELCHANGED:
					UpdateDescLabel(TreeView_GetSelection(htree));
					return 0;
				default:
					break;
				}
			}
		}
		break;
	case WM_MOUSEMOVE:
		UpdateDescLabel(0);
		break;
	default:
		break;
	}
	return CallWindowProc(oldCompWndProc, hwnd, uMsg, wParam, lParam);
}


static LRESULT WINAPI TreeProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CHAR && wParam == VK_SPACE)
	{
		if (ctree.OnStateToggle(TreeView_GetSelection(htree)))
		{
			cur_itype = itype_custom;
			SendMessage(hitypecb, CB_SETCURSEL, itype_custom, 0);
			UpdateInstallType();
		}
		return 0;
	}

	if (uMsg == WM_MOUSEMOVE && IsWindowVisible(htree))
	{
		TVHITTESTINFO ht;
		DWORD dwpos = GetMessagePos();
		ht.pt.x = GET_X_LPARAM(dwpos);
		ht.pt.y = GET_Y_LPARAM(dwpos);
		ScreenToClient(htree, &ht.pt);
		(void)TreeView_HitTest(htree, &ht);
		if (ht.flags & (TVHT_ONITEMSTATEICON | TVHT_ONITEMLABEL | TVHT_ONITEMRIGHT | TVHT_ONITEM))
			UpdateDescLabel(ht.hItem);
		else
			UpdateDescLabel(0);
	}

	return CallWindowProc(oldTreeWndProc, hwnd, uMsg, wParam, lParam);
}


static bool IsDigit(char ch)
{
	return (ch >= '0' && ch <= '9');
}


static int VersionCompare(const char* v1, const char* v2)
{
	int v1len = strlen(v1);
	int v2len = strlen(v2);
	int start1 = 0, start2 = 0;
	while (start1 < v1len && start2 < v2len)
	{
		bool d = IsDigit(v1[start1]);
		if (d != IsDigit(v2[start2]))
			return strcmp(v1 + start1, v2 + start2);
		int at1 = start1 + 1;
		while (IsDigit(v1[at1]) == d)
			++at1;
		int at2 = start2 + 1;
		while (IsDigit(v2[at2]) == d)
			++at2;
		if (d)
		{
			int n1 = atoi(std::string(v1 + start1, at1 - start1).c_str());
			int n2 = atoi(std::string(v2 + start2, at2 - start2).c_str());
			if (n1 != n2)
				return n1 - n2;
		}
		else
		{
			int res = strcmp(std::string(v1 + start1, at1 - start1).c_str(),
			 std::string(v2 + start2, at2 - start2).c_str());
			if (res != 0)
				return res;
		}
		start1 = at1;
		start2 = at2;
	}
	return strcmp(v1 + start1, v2 + start2);
}


#if 0
static INT_PTR CALLBACK UpdateVerProc
 (HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			// Get the owner window and dialog box rectangles.
			HWND hwndOwner = GetParent(hwndDlg);
			if (hwndOwner == NULL)
				hwndOwner = GetDesktopWindow();
			RECT rc, rcOwner, rcDlg;
			GetWindowRect(hwndOwner, &rcOwner);
			GetWindowRect(hwndDlg, &rcDlg);
			CopyRect(&rc, &rcOwner);
			// Offset the owner and dialog box rectangles so that right and
			// bottom values represent the width and height, and then offset the
			// owner again to discard space taken up by the dialog box.
			OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
			OffsetRect(&rc, -rc.left, -rc.top);
			OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
			// The new position is the sum of half the remaining space and the
			// owner's original position.
			SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left + (rc.right / 2),
			 rcOwner.top + (rc.bottom / 2), 0, 0, SWP_NOSIZE);
			const char* txt = (const char*)lParam;
			Static_SetText(GetDlgItem(hwndDlg, IDT_UVMSG), txt);
		}
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			EndDialog(hwndDlg, 0);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}
#endif


extern "C" void __declspec(dllexport) SetManifest
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType ret = "OK";

	StringType fname = NSIS::popstring();
	HWND parentwnd = (HWND)NSIS::popint();

	working_man = XMLDocumentRef(new tinyxml2::XMLDocument);
	if (working_man->LoadFile(fname.c_str()) == XML_SUCCESS)
	{
		XMLElement* minver =
		 working_man->RootElement()->FirstChildElement("MinVersion");
		if (minver)
		{
			const char* ver = minver->Attribute("version");
			if (ver && strlen(ver) > 0)
			{
				if (VersionCompare(STR_SETUP_VER, ver) < 0)
				{
					XMLText* txtel =
					 XMLHandle(minver->FirstChild()).ToText();
					if (txtel)
					{
						MessageBox(parentwnd, txtel->Value(),
						 "Updated Version Available",
						 MB_OK | MB_ICONINFORMATION);
						ret = "update";
					}
				}
			}
		}
	}
	else
	{
		working_man = 0;
		ret = "bad XML";
	}

	NSIS::pushstring(ret.c_str());
}


extern "C" void __declspec(dllexport) GetTidbits
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);
	StringType system_id = NSIS::popstring();
	StringType tempdir = NSIS::popstring();
	StringType title;
	StringType desc;
	StringType content;
	do {
		if (!working_man)
			break;
		XMLElement* system_el = working_man->RootElement()->FirstChildElement("System");
		for (; system_el; system_el = system_el->NextSiblingElement("System")) {
			if (strcmp(system_el->Attribute("id"), system_id.c_str()) == 0)
				break;
		}
		if (!system_el)
			break;
		XMLElement* tidbit_el = system_el->FirstChildElement("Tidbit");
		if (!tidbit_el)
			break;
		XMLText* txtnode = XMLHandle(tidbit_el->FirstChildElement("Title"))
		.FirstChild().ToText();
		if (txtnode)
			title = txtnode->Value();
		txtnode = XMLHandle(tidbit_el->FirstChildElement("Description"))
		.FirstChild().ToText();
		if (txtnode)
			desc = txtnode->Value();
		txtnode = XMLHandle(tidbit_el->FirstChildElement("Content"))
		.FirstChild().ToText();
		if (txtnode) {
			StringType cfile = tempdir + "\\tidbit-" + system_id + ".txt";
			FILE* f = fopen(cfile.c_str(), "wb");
			if (!f)
				break;
			content = txtnode->Value();
			fwrite(content.c_str(), content.length(), 1, f);
			fclose(f);
			content = cfile;
		}
	} while (false);
	NSIS::pushstring(title.c_str());
	NSIS::pushstring(desc.c_str());
	NSIS::pushstring(content.c_str());
}


static void SelectWithChildren(const XMLElement* el)
{
	std::queue< const XMLElement* > eq;
	eq.push(el);
	while (!eq.empty())
	{
		const XMLElement* el = eq.front();
		eq.pop();
		if (!el)
			continue;
		const char* el_val = el->Value();
		if (el_val
		 && (strcmp(el_val, "Component") == 0
		  || strcmp(el_val, "Version") == 0))
		{
			const char* el_id = el->Attribute("id");
			if (el_id && strlen(el_id) > 0)
			{
				int el_index = ctree.GetIndex(el_id);
				if (el_index >= 0)
					ctree.SetSelected(el_index, true);
			}
		}
		for (const XMLElement* c = el->FirstChildElement();
		 c;
		 c = c->NextSiblingElement())
			eq.push(c);
	}
}


static void UpdateInstallType()
{
	if (cur_itype < 0)
		return;
	if (cur_itype == itype_current)
		ctree.SetPrevInstSelected();
	else
	{
		XMLElement* it_el = itypes_by_index[cur_itype];
		if (it_el)
		{
			ctree.DeselectAll();
			for (XMLElement* sel_el = it_el->FirstChildElement();
			 sel_el;
			 sel_el = sel_el->NextSiblingElement())
			{
				const char* sel_val = sel_el->Value();
				if (strcmp(sel_val, "Select") == 0)
				{
					const char* sel_id = sel_el->Attribute("id");
					if (sel_id && strlen(sel_id) > 0)
					{
						int sel_index = ctree.GetIndex(sel_id);
						if (sel_index >= 0)
							ctree.SetSelected(sel_index, true);
					}
				}
				else if (strcmp(sel_val, "SelectTree") == 0)
				{
					const char* sel_id = sel_el->Attribute("id");
					if (sel_id && strlen(sel_id) > 0)
					{
						int sel_index = ctree.GetIndex(sel_id);
						if (sel_index >= 0)
							SelectWithChildren(ctree.GetElement(sel_index));
					}
				}
			}
		}
	}
	MaybeUpdateSizeLabel();
}


extern "C" void __declspec(dllexport) PopulateInstallTypeList
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType system_id = NSIS::popstring();
	hitypecb = (HWND)NSIS::popint();
	int is_previnst = NSIS::popint();

	XMLElement* sys_el = XMLHandle(working_man->RootElement()).FirstChildElement("System").ToElement();
	while (sys_el)
	{
		const char* id = sys_el->Attribute("id");
		if (id && (system_id == id))
			break;
		sys_el = sys_el->NextSiblingElement("System");
	}
	if (!sys_el)
		return;

	itypes_by_index.clear();
	itype_current = -1;
	itype_custom = -1;
	for (XMLElement* itype = sys_el->FirstChildElement("InstallType");
	 itype;
	 itype = itype->NextSiblingElement("InstallType"))
	{
		const char* it_name = itype->Attribute("name");
		SendMessage(hitypecb, CB_ADDSTRING, 0, (LPARAM)it_name);
		itypes_by_index.push_back(itype);
	}
	if (is_previnst)
	{
		SendMessage(hitypecb, CB_ADDSTRING, 0,
		 (LPARAM)"[Current Installation]");
		itype_current = itypes_by_index.size();
		itypes_by_index.push_back(0);
	}
	SendMessage(hitypecb, CB_ADDSTRING, 0, (LPARAM)"Custom");
	itype_custom = itypes_by_index.size();
	itypes_by_index.push_back(0);
	SendMessage(hitypecb, CB_SETCURSEL, 0, 0);
	cur_itype = 0;
}


extern "C" void __declspec(dllexport) CreateComponentsTree
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	HWND hdialog = (HWND)NSIS::popint();
	StringType system_id = NSIS::popstring();

	if (system_id.length() == 0)
	{
		if (!prev_man)
			return;
		XMLElement* sys_el = prev_man->RootElement()->FirstChildElement("System");
		if (!sys_el)
			return;
		const char* sys_id = sys_el->Attribute("id");
		if (!sys_id || strlen(sys_id) <= 0)
			return;
		system_id = sys_id;
	}

	htree = CreateWindowEx(
	 WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE,
     WC_TREEVIEW,
     "Tree View",
     WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VSCROLL
      | TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT,
     0, 80, 300, 124,
     hdialog,
     (HMENU)1300,
     ginstance,
     NULL);
	oldCompWndProc = (WNDPROC)SetWindowLong(GetParent(htree), GWL_WNDPROC, (LONG)CompPageProc);
	oldTreeWndProc = (WNDPROC)SetWindowLong(htree, GWL_WNDPROC, (LONG)TreeProc);
	SendMessage(htree, WM_SETFONT, SendMessage(hwndParent, WM_GETFONT, 0, 0),
	 TRUE);

	if (!imglist)
	{
		imglist = RefType< struct _IMAGELIST >::Ref(
		 ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 10, 0),
		 ImageList_Destroy);
		HBITMAP buttonsbmp =
		 LoadBitmap(ginstance, MAKEINTRESOURCE(IDB_TDMINST_BUTTONS));
		int nfirst = ImageList_AddMasked(RefGetPtr(imglist), buttonsbmp,
		 RGB(255, 0, 255));
		DeleteObject(buttonsbmp);
		ncheck = nfirst + 1;
		nradio = nfirst + 6;
	}
	(void)TreeView_SetImageList(htree, RefGetPtr(imglist), TVSIL_STATE);

	ctree.BuildTreeView(htree, ncheck, nradio, system_id,
	 working_man ? working_man->RootElement() : 0,
	 prev_man ? prev_man->RootElement() : 0);

	UpdateInstallType();
}


extern "C" void __declspec(dllexport) PopulateMirrorList
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	HWND hlist = (HWND)NSIS::popint();

	mirrors.clear();
	for (XMLElement* mir = XMLHandle(working_man->RootElement()).FirstChildElement("Mirror").ToElement();
	 mir;
	 mir = mir->NextSiblingElement("Mirror"))
	{
		const char* mir_name = mir->Attribute("name");
		const char* mir_url = mir->Attribute("baseurl");
		if (mir_name && strlen(mir_name) > 0 && mir_url && strlen(mir_url) > 0)
		{
			SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)mir_name);
			mirrors.push_back(mir_url);
		}
	}
	SendMessage(hlist, LB_SETCURSEL, 0, 0);
}


extern "C" void __declspec(dllexport) EnumArchives
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	nsFunction callback = NSIS::popint();
	StringType result = "OK";
	ElementList alist;
	num_archives_to_dl = ctree.GetArchivesToInstall(alist);
	for (ElementList::const_iterator it = alist.begin();
	 it != alist.end();
	 ++it)
	{
		const char* ar_path = (*it)->Attribute("path");
		if (ar_path && strlen(ar_path) > 0)
		{
			int at = strlen(ar_path) - 2;
			for (; at >= 0 && ar_path[at] != '/';
			 --at);
			NSIS::pushstring(ar_path + at + 1);
			NSIS::pushstring(at >= 0 ?
			 std::string(ar_path).substr(0, at + 1).c_str()
			 :
			 "");
			NSIS::ExecuteCallback(callback);
			result = NSIS::popstring();
			if (result != "OK")
				break;
		}
	}
	NSIS::pushstring(result.c_str());
}


extern "C" void __declspec(dllexport) GetSelMirror
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	HWND hlist = (HWND)NSIS::popint();
	int sel = SendMessage(hlist, LB_GETCURSEL, 0, 0);
	if (sel >= 0)
		NSIS::pushstring(mirrors[sel].c_str());
	else
		NSIS::pushstring("");
}


extern "C" void __declspec(dllexport) SetInstLocation
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	inst_loc = NSIS::popstring();
	if (inst_loc.length() > 0)
		inst_man = RefType< InstallManifest >::Ref(
		 new InstallManifest(inst_loc + "\\__installer\\installed_man.txt")
		 );
	else
		inst_man = RefType< InstallManifest >::Ref();
}


extern "C" void __declspec(dllexport) SetPrevInstMan
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	int ret = 0;

	prev_man = 0;
	StringType mfile = NSIS::popstring();
	if (mfile.length() > 0)
	{
		XMLDocumentRef newprev(new tinyxml2::XMLDocument);
		if (newprev->LoadFile(mfile.c_str()) == XML_SUCCESS)
		{
			prev_man = newprev;
			ret = 1;
		}
	}

	NSIS::pushint(ret);
}


StringType InstallArchive
 (const char* base,
  const StringType&,
  InstallManifest&,
  int (*)(const char*, bool, bool, bool));
static nsFunction ra_cb_func = 0;

static int RAOnCallback(const char* file, bool is_dir, bool is_del, bool is_arc)
{
	if (ra_cb_func > 0)
	{
		NSIS::pushint(is_arc ? 1 : 0);
		NSIS::pushint(is_del ? 1 : 0);
		NSIS::pushint(is_dir ? 1 : 0);
		NSIS::pushstring(file);
		NSIS::ExecuteCallback(ra_cb_func);
	}
	return 0;
}


static void RemoveEntry(const char* base, const char* entry)
{
	char ent[1024];
	strncpy(ent, base, 1023);
	int blen = strlen(ent);
	ent[blen] = '/';
	strncpy(ent + blen + 1, entry, 1023 - blen - 1);
	int flen = strlen(ent);
	if (ent[flen - 1] != '/' && ent[flen - 1] != '\\')
	{
		chmod(ent, _S_IREAD | _S_IWRITE);
		int ret = remove(ent);
		if (ret != 0 && ret != ENOENT)
			return;
		RAOnCallback(entry, false, true, false);
	}
	for (int i = flen - 1; i > blen + 1; --i)
	{
		if (ent[i] == '/' || ent[i] == '\\')
		{
			ent[i] = '\0';
			int ret = _rmdir(ent);
			if (ret != 0 && ret != ENOENT)
				return;
			RAOnCallback(ent + blen + 1, true, true, false);
		}
	}
}


static bool FileExists(const char* fname)
{
	struct stat st;
	return (stat(fname, &st) == 0);
}


extern "C" void __declspec(dllexport) RemoveAndAdd
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType lpathsstring = NSIS::popstring();
	ra_cb_func = NSIS::popint();

	if (inst_loc.length() <= 0)
	{
		NSIS::pushstring("Installation location hasn't been set");
		return;
	}

	std::list< StringType > local_paths;
	size_t pstart = 0;
	for (size_t i = 1; i <= lpathsstring.length(); ++i)
	{
		if (i == lpathsstring.length() || lpathsstring[i] == '|')
		{
			local_paths.push_back(lpathsstring.substr(pstart, i - pstart));
			pstart = i + 1;
			i += 2;
		}
	}

	cur_op_index = 0;

	ElementList alist;
	ElementList rlist;
	num_addremove_ops = ctree.GetArchivesToInstall(alist);
	num_addremove_ops += ctree.GetComponentsToRemove(rlist);

	// Remove deselected components //

	for (ElementList::const_iterator it = rlist.begin();
	 it != rlist.end();
	 ++it)
	{
		num_files_in_cur_op = 0;
		for (XMLElement* entry =
		 (*it)->FirstChildElement("Entry");
		 entry;
		 entry = entry->NextSiblingElement("Entry"))
			++num_files_in_cur_op;
		cur_file_in_op_index = 0;
		for (XMLElement* entry = (*it)->LastChildElement("Entry");
		 entry;
		 entry = entry->PreviousSiblingElement("Entry"))
		{
			XMLText* etxt = XMLHandle(entry->FirstChild()).ToText();
			if (etxt)
				RemoveEntry(inst_loc.c_str(), etxt->Value());
			++cur_file_in_op_index;
		}
		++cur_op_index;
	}

	// Add selected components //

	inst_man->SetComponent("MiscFiles");
	mkdir((inst_loc + "/__installer").c_str());
	inst_man->AddEntry("__installer/");
	inst_man->AddEntry("__installer/installed_man.txt");

	StringType result = "OK";

	for (ElementList::const_iterator it = alist.begin();
	 it != alist.end();
	 ++it)
	{
		const char* comp_id = 0;
		XMLElement* parent1 = XMLHandle((*it)->Parent()).ToElement();
		if (parent1)
			comp_id = parent1->Attribute("id");
		if (comp_id && strlen(comp_id) > 0)
		{
			inst_man->SetComponent(comp_id);
			const char* ar_path = (*it)->Attribute("path");
			if (ar_path && strlen(ar_path) > 0)
			{
				int at = strlen(ar_path) - 2;
				for (; at >= 0 && ar_path[at] != '/';
				 --at);
				std::list< StringType >::const_iterator it =
				 local_paths.begin();
				for (; it != local_paths.end(); ++it)
				{
					if (FileExists((*it + "\\" + (ar_path + at + 1)).c_str()))
						break;
				}
				if (it != local_paths.end())
				{
				    RAOnCallback(ar_path + at + 1, false, false, true);
					result = InstallArchive(inst_loc.c_str(),
					 (*it + "\\" + (ar_path + at + 1)).c_str(),
					 *RefGetPtr(inst_man),
					 RAOnCallback);
					if (result != "OK")
						break;
				}
				else
				{
					result = StringType("Couldn't find local archive '")
					 + ar_path + "'";
					break;
				}
			}
		}
		++cur_op_index;
	}

	NSIS::pushstring(result.c_str());
}


extern "C" void __declspec(dllexport) GetInstallProgress
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	float per_op = 100.0f / num_addremove_ops;
	if (num_files_in_cur_op <= 0)
	{
		float per_file = per_op / 100.0f;
		if (cur_file_in_op_index * per_file > per_op)
			NSIS::pushint(per_op * (cur_op_index + 1));
		else
			NSIS::pushint((per_op * cur_op_index)
			 + (cur_file_in_op_index * per_file));
	}
	else
	{
		float per_file = per_op / num_files_in_cur_op;
		NSIS::pushint((per_op * cur_op_index)
		 + (per_file * cur_file_in_op_index));
	}
}


extern "C" void __declspec(dllexport) GetDownloadProgress
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	int cur_dl = NSIS::popint();
	NSIS::pushint((100.0f / num_archives_to_dl) * cur_dl);
}


extern "C" void __declspec(dllexport) GetComponentSelected
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	int smindex = ctree.GetIndex(NSIS::popstring());
	if (smindex >= 0)
		NSIS::pushint(ctree.IsSelected(smindex) ? 1 : 0);
	else
		NSIS::pushint(0);
}


extern "C" void __declspec(dllexport) AddManMiscFile
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	inst_man->SetComponent("MiscFiles");
	inst_man->AddEntry(NSIS::popstring().c_str());
}


extern "C" void __declspec(dllexport) WriteInstManifest
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	if (inst_man)
	{
		ctree.WriteInstMan(inst_loc + "\\__installer\\installed_man.txt",
		 *RefGetPtr(inst_man));
	}
}


extern "C" void __declspec(dllexport) CheckIfUserInstall
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType idir = NSIS::popstring();
	StringType uname = NSIS::popstring();
	if (idir.find(StringType("\\") + uname + "\\") != StringType::npos
	 || idir.find(StringType("\\") + uname + ".") != StringType::npos)
		NSIS::pushint(1);
	else
		NSIS::pushint(0);
}


static void ParseInstsFile(const char* fname)
{
	if (!fname)
		return;
	FILE* in = fopen(fname, "r");
	if (!in)
		return;
	RefType< FILE >::Ref fcloser(in, fclose);
	char instr[2048];
	while (fgets(instr, 2047, in))
	{
		for (int i = strlen(instr) - 1;
		 instr[i] == '\n' || instr[i] == '\r';
		 --i)
			instr[i] = '\0';
		if (FileExists((StringType(instr) + "\\__installer\\installed_man.txt").c_str()))
			prev_insts.insert(instr);
	}
}


extern "C" void __declspec(dllexport) UpdateFoundInsts
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType sfile = NSIS::popstring();
	ParseInstsFile(sfile.c_str());

	NSIS::pushint(prev_insts.size());
}


static INT_PTR CALLBACK InstFindProc
 (HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			// Get the owner window and dialog box rectangles.
			HWND hwndOwner = GetParent(hwndDlg);
			if (hwndOwner == NULL)
				hwndOwner = GetDesktopWindow();
			RECT rc, rcOwner, rcDlg;
			GetWindowRect(hwndOwner, &rcOwner);
			GetWindowRect(hwndDlg, &rcDlg);
			CopyRect(&rc, &rcOwner);
			// Offset the owner and dialog box rectangles so that right and
			// bottom values represent the width and height, and then offset the
			// owner again to discard space taken up by the dialog box.
			OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
			OffsetRect(&rc, -rc.left, -rc.top);
			OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);
			// The new position is the sum of half the remaining space and the
			// owner's original position.
			SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left + (rc.right / 2),
			 rcOwner.top + (rc.bottom / 2), 0, 0, SWP_NOSIZE);
		}
		return TRUE;
	default:
		break;
	}
	return FALSE;
}


static DWORD WINAPI InstFindDlgThread(LPVOID lpParameter)
{
	HWND hdlg = (HWND)lpParameter;
	BOOL bRet;
	MSG msg;
	while ((bRet = GetMessage(&msg, hdlg, 0, 0)) != 0)
	{
		if (bRet == -1)
			return -1;
		else if (!IsWindow(hdlg) || !IsDialogMessage(hdlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}


extern "C" void __declspec(dllexport) BeginInstFindBanner
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	hinstfinddlg = CreateDialog(ginstance, MAKEINTRESOURCE(IDD_INSTFINDBANNER),
	 0, InstFindProc);
	ShowWindow(hinstfinddlg, SW_SHOW);
	CreateThread(0, 0, InstFindDlgThread, hinstfinddlg, 0, 0);
}


extern "C" void __declspec(dllexport) EndInstFindBanner
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	if (hinstfinddlg)
	{
		DestroyWindow(hinstfinddlg);
		hinstfinddlg = 0;
	}
}


extern "C" void __declspec(dllexport) WriteInstList
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	StringType ldir = NSIS::popstring();
	StringType lfile = NSIS::popstring();
	makedir(ldir.c_str(), "");
	FILE* infile = fopen((ldir + "\\" + lfile).c_str(), "r");
	if (infile)
	{
		RefType< FILE >::Ref fcloser(infile, fclose);
		char instr[2048];
		while (fgets(instr, 2047, infile))
		{
			for (int i = strlen(instr) - 1;
			 instr[i] == '\n' || instr[i] == '\r';
			 --i)
				instr[i] = '\0';
			if (stricmp(instr, inst_loc.c_str()) == 0)
				return;
		}
	}
	FILE* outfile = fopen((ldir + "\\" + lfile).c_str(), "a");
	if (!outfile)
		return;
	fprintf(outfile, "%s\n", inst_loc.c_str());
	fclose(outfile);
}


extern "C" void __declspec(dllexport) IsPrevInst
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	if (prev_insts.count(NSIS::popstring()) > 0)
		NSIS::pushint(1);
	else
		NSIS::pushint(0);
}


extern "C" void __declspec(dllexport) GetFirstPrevInst
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	NSIS::pushstring(prev_insts.empty() ? "" : prev_insts.begin()->c_str());
}


extern "C" void __declspec(dllexport) PopulatePrevInstList
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	HWND hlist = (HWND)NSIS::popint();

	for (PrevInstSet::iterator it = prev_insts.begin();
	 it != prev_insts.end();
	 ++it)
	{
		SendMessage(hlist, LB_ADDSTRING, 0, (LPARAM)it->c_str());
	}
}


static void CharArrayDeleter(char* a)
{
	delete[] a;
}

static DWORD WINAPI DirPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if ((HWND)lParam == hdirlist)
		{
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				int cursel = SendMessage(hdirlist, LB_GETCURSEL, 0, 0);
				int len = SendMessage(hdirlist, LB_GETTEXTLEN, cursel, 0);
				if (len >= 3)
				{
					RefType< char >::Ref str(new char[len + 1],
					 CharArrayDeleter);
					if (SendMessage(hdirlist, LB_GETTEXT, cursel,
					 (LPARAM)RefGetPtr(str)) >= 3)
						Edit_SetText(hdirpagedir, RefGetPtr(str));
				}
				return 0;
			}
			else if (HIWORD(wParam) == LBN_DBLCLK)
			{
				if (previnstsel_callback > 0)
				{
					int cursel = SendMessage(hdirlist, LB_GETCURSEL, 0, 0);
					int len = SendMessage(hdirlist, LB_GETTEXTLEN, cursel, 0);
					if (len >= 3)
					{
						RefType< char >::Ref str(new char[len + 1],
						 CharArrayDeleter);
						if (SendMessage(hdirlist, LB_GETTEXT, cursel,
						 (LPARAM)RefGetPtr(str)) >= 3)
						{
							NSIS::pushstring(RefGetPtr(str));
							NSIS::ExecuteCallback(previnstsel_callback);
						}
					}
				}
				return 0;
			}
		}
		break;
	default:
		break;
	}
	return CallWindowProc(oldDirWndProc, hwnd, uMsg, wParam, lParam);
}

extern "C" void __declspec(dllexport) CreateInstDirPrevList
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	HWND hdialog = (HWND)NSIS::popint();
	int page_id = NSIS::popint();
	previnstsel_callback = NSIS::popint();

	RECT rc;
	if (page_id == 2)
	{
		HWND htext = GetDlgItem(hdialog, 1006);
		SetWindowPos(htext, 0, 0, 0,
		 450, 40,
		 SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		hdirpagedir = GetDlgItem(hdialog, 1019);
		rc.left = 15;
		rc.top = 42;
		rc.right = 315;
		rc.bottom = 60;
	}
	else
	{
		rc.left = 264;
		rc.top = 15;
		rc.right = 176;
		rc.bottom = 181;
	}

	hdirlist = CreateWindowEx(
	 WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE,
     WC_LISTBOX,
     "List Box",
     WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VSCROLL
      | LBS_DISABLENOSCROLL | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT
      | LBS_NOTIFY,
     rc.left, rc.top, rc.right, rc.bottom,
     hdialog,
     (HMENU)1301,
     ginstance,
     NULL);
	SendMessage(hdirlist, WM_SETFONT, SendMessage(hwndParent, WM_GETFONT, 0, 0),
	 TRUE);
	for (PrevInstSet::iterator it = prev_insts.begin();
	 it != prev_insts.end();
	 ++it)
	{
		SendMessage(hdirlist, LB_ADDSTRING, 0, (LPARAM)it->c_str());
	}
	if (inst_loc.length() > 0)
		ListBox_SelectString(hdirlist, 0, inst_loc.c_str());
}


extern "C" void __declspec(dllexport) SetSpaceReqLabel
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	hsizelabel = (HWND)NSIS::popint();

	MaybeUpdateSizeLabel();
}


extern "C" void __declspec(dllexport) ReplaceDirProc
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	HWND hdialog = (HWND)NSIS::popint();

	oldDirWndProc = (WNDPROC)SetWindowLong(hdialog, GWL_WNDPROC,
	 (LONG)DirPageProc);
}


extern "C" void __declspec(dllexport) CreateInstDirManMsg
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	HWND hdialog = (HWND)NSIS::popint();

	HWND hmsg = CreateWindowEx(
	 WS_EX_TRANSPARENT,
     WC_STATIC,
     "",
     WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
     17, 176, 440, 20,
     hdialog,
     (HMENU)1302,
     ginstance,
     NULL);
	SendMessage(hmsg, WM_SETFONT, SendMessage(hwndParent, WM_GETFONT, 0, 0),
	 TRUE);

	NSIS::pushint((int)hmsg);
}


extern "C" void __declspec(dllexport) CreateInstDirUninstNote
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	HWND hdialog = (HWND)NSIS::popint();

	HWND hmsg = CreateWindowEx(
	 WS_EX_TRANSPARENT,
     WC_STATIC,
     "",
     WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
     0, 206, 440, 20,
     hdialog,
     (HMENU)1302,
     ginstance,
     NULL);
	SendMessage(hmsg, WM_SETFONT, SendMessage(hwndParent, WM_GETFONT, 0, 0),
	 TRUE);
	Static_SetText(hmsg, "Note: Any files not originally created by the "
	 STR_SHORTNAME " installer will not be removed.");

	NSIS::pushint((int)hmsg);
}


extern "C" void __declspec(dllexport) RunUninstall
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType uninst_loc = NSIS::popstring();
	StringType tempdir = NSIS::popstring();
	StringType exepath = NSIS::popstring();
	StringType exefile = NSIS::popstring();
	nsFunction writeuninstcb = NSIS::popint();

	StringType ret = "OK";

	StringType tfile;
	char begin_ch = 'A';
	for (; begin_ch <= 'Z'; ++begin_ch)
	{
		tfile = tempdir + "\\" + begin_ch + "u_.exe";
		DeleteFile(tfile.c_str());
		NSIS::pushstring(tfile.c_str());
		NSIS::ExecuteCallback(writeuninstcb);
		if (FileExists(tfile.c_str()))
			break;
	}
	if (begin_ch <= 'Z')
	{
		MoveFileEx(tfile.c_str(), 0,
		 MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING);
		StringType ucmd = tfile + " \"/tdmu=" + uninst_loc + "\"";
		RefType< char >::Ref cmd(new char[ucmd.length() + 2],
		 CharArrayDeleter);
		strncpy(RefGetPtr(cmd), ucmd.c_str(), ucmd.length() + 1);
		PROCESS_INFORMATION ProcInfo;
		static STARTUPINFO StartUp;
		StartUp.cb = sizeof(StartUp);
		if (CreateProcess(NULL, RefGetPtr(cmd), NULL, NULL, FALSE, 0, NULL,
		 NULL, &StartUp, &ProcInfo))
		{
			CloseHandle(ProcInfo.hThread);
			CloseHandle(ProcInfo.hProcess);
		}
		else
			ret = StringType("Couldn't run '") + tfile + "'";
	}
	else
		ret = StringType("Couldn't copy '") + exepath + "' to '" + tfile + "'";

	NSIS::pushstring(ret.c_str());
}


extern "C" void __declspec(dllexport) GetUninst
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType params = NSIS::popstring();

	StringType ret = "";

	size_t pbegin = 0;
	bool inquotes = false;
	for (size_t plen = 0; pbegin + plen <= params.length(); ++plen)
	{
		if (pbegin + plen < params.length()
		 && params[pbegin + plen] == '"')
			inquotes = !inquotes;
		if ((pbegin + plen == params.length())
		 || (!inquotes && params[pbegin + plen] == ' '))
		{
			if (plen > 0)
			{
				StringType cur;
				if (params[pbegin] != '"')
					cur = params.substr(pbegin, plen);
				else if (plen > 2)
					cur = params.substr(pbegin + 1, plen - 2);
				if (cur.length() >= 5
				 && strcmp(cur.substr(0, 5).c_str(), "/tdmu") == 0)
				{
					if (cur.length() >= 6)
						ret = cur.substr(6);
					else
						ret = "true";
					break;
				}
			}
			pbegin += plen + 1;
			plen = 0;
		}
	}

	NSIS::pushstring(ret.c_str());
}


extern "C" void __declspec(dllexport) RemoveInst
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType uninstloc = NSIS::popstring();
	StringType instlist = NSIS::popstring();
	ra_cb_func = NSIS::popint();

	if (uninstloc.length() <= 0)
	{
		NSIS::pushstring("Uninstall location hasn't been set");
		return;
	}
	tinyxml2::XMLDocument mdoc;
	if (mdoc.LoadFile((uninstloc + "\\__installer\\installed_man.txt").c_str()) != XML_SUCCESS)
	{
		StringType estr = std::string("Couldn't load '") + uninstloc
		 + "\\__installer\\installed_man.txt'";
		NSIS::pushstring(estr.c_str());
		return;
	}

	StringType ret = "OK";

	cur_op_index = 0;

	ElementList rlist;
	num_addremove_ops = ctree.GetComponentsToRemove(rlist, &mdoc);

	// Remove components //

	for (ElementList::const_iterator it = rlist.begin();
	 it != rlist.end();
	 ++it)
	{
		num_files_in_cur_op = 0;
		for (XMLElement* entry =
		 (*it)->FirstChildElement("Entry");
		 entry;
		 entry = entry->NextSiblingElement("Entry"))
			++num_files_in_cur_op;
		cur_file_in_op_index = 0;
		for (XMLElement* entry =
		 (*it)->LastChildElement("Entry");
		 entry;
		 entry = entry->PreviousSiblingElement("Entry"))
		{
			XMLText* etxt = XMLHandle(entry->FirstChild()).ToText();
			if (etxt)
				RemoveEntry(uninstloc.c_str(), etxt->Value());
			++cur_file_in_op_index;
		}
		++cur_op_index;
	}

	// Remove installation from list //

	if (instlist.length() > 0)
	{
		FILE* in = fopen(instlist.c_str(), "r");
		if (in)
		{
			RefType< FILE >::Ref ifcloser(in, fclose);
			FILE* out = fopen((instlist + ".tmp").c_str(), "w");
			if (out)
			{
				char instr[2048];
				while (fgets(instr, 2047, in))
				{
					for (int i = strlen(instr) - 1;
					 instr[i] == '\n' || instr[i] == '\r';
					 --i)
						instr[i] = '\0';
					if (stricmp(instr, uninstloc.c_str()) != 0)
						fprintf(out, "%s\n", instr);
				}
				fclose(out);
				ifcloser = RefType< FILE >::Ref();
				MoveFileEx((instlist + ".tmp").c_str(), instlist.c_str(),
				 MOVEFILE_REPLACE_EXISTING);
			}
		}
	}

	NSIS::pushstring(ret.c_str());
}


extern "C" void __declspec(dllexport) StringInString
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType in = NSIS::popstring();
	StringType check = NSIS::popstring();

	if (check.find(in) != StringType::npos)
		NSIS::pushint(1);
	else
		NSIS::pushint(0);
}


extern "C" void __declspec(dllexport) RegisterInnerArchive
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	ctree.SetInnerArchive(NSIS::popstring());
}


enum WOW64CheckStatus
{
	CHECK,
	SYS32,
	SYS64
};

static REGSAM MaybeWOW64Flag()
{
	static WOW64CheckStatus status = CHECK;
	if (status == CHECK)
	{
		FARPROC IsWow64Process_p = GetProcAddress(GetModuleHandle("kernel32"), "IsWow64Process");
		if (IsWow64Process_p)
		{
			BOOL wow64_out = TRUE;
			typedef void (WINAPI *IsWow64ProcessFunc)(HANDLE, PBOOL);
			((IsWow64ProcessFunc)IsWow64Process_p)(GetCurrentProcess(), &wow64_out);
			status = (wow64_out) ? SYS64 : SYS32;
		}
		else
			status = SYS32;
	}
	return (status == SYS64) ? 0x0100 : 0; // 0x0100 is KEY_WOW64_64KEY
}


extern "C" void __declspec(dllexport) EnsureInPathEnv
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra) {
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType path = NSIS::popstring();
	StringType mui_mode = NSIS::popstring();

	StringType ret = "OK";

	do {
		HKEY base_key = HKEY_CURRENT_USER;
		const char* base_path = "Environment";
		if (mui_mode == "AllUsers") {
			base_key = HKEY_LOCAL_MACHINE;
			base_path = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
		}
		HKEY this_key;
		if (RegOpenKeyEx(base_key, base_path, 0, MaybeWOW64Flag() | KEY_READ,
		 &this_key) != ERROR_SUCCESS) {
			ret = "Failed to open PATH setting in registry";
			break;
		}
		StringType current_path;
		DWORD value_size = 0;
		if (RegQueryValueEx(this_key, "Path", 0, 0, 0, &value_size)
		 == ERROR_SUCCESS) {
			TCHAR value_buf[value_size + 1];
			if (RegQueryValueEx(this_key, "Path", 0, 0, (BYTE*)value_buf,
			 &value_size) != ERROR_SUCCESS) {
				ret = "Failed to retrieve PATH setting from registry";
				break;
			}
			value_buf[value_size] = 0;
			current_path = value_buf;
			if (current_path.find(path) != StringType::npos)
				break;
			if (current_path.length() > 0)
				current_path += ";";
		}
		current_path += path;
		HKEY write_key;
		if (RegCreateKeyEx(base_key, base_path, 0, 0, 0,
		 MaybeWOW64Flag() | KEY_WRITE, 0, &write_key, 0) != ERROR_SUCCESS) {
			ret = "Failed to open PATH registry setting with write privileges";
			break;
		}
		if (RegSetValueEx(write_key, "Path", 0, REG_EXPAND_SZ,
		 (const BYTE*)current_path.c_str(),
		 (current_path.length() + 1) * sizeof(CharType))
		  != ERROR_SUCCESS) {
			ret = "Failed to write new PATH setting to registry";
			break;
		}
		DWORD dwReturnValue;
		SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
		 (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, &dwReturnValue);
	} while (false);

	NSIS::pushstring(ret.c_str());
}


extern "C" void __declspec(dllexport) EnsureNotInPathEnv
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType path = NSIS::popstring();

	typedef std::list< std::pair< HKEY, const char* > > HivesList;
	HivesList hives;
	hives.push_back(std::pair< HKEY, const char* >(HKEY_CURRENT_USER, "Environment"));
	hives.push_back(std::pair< HKEY, const char* >(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"));

	bool changes = false;
	for (HivesList::iterator it = hives.begin(); it != hives.end(); ++it)
	{
		HKEY this_key;
		if (RegOpenKeyEx(it->first, it->second, 0, MaybeWOW64Flag() | KEY_READ,
		 &this_key) != ERROR_SUCCESS)
			continue;
		DWORD value_size = 0;
		if (RegQueryValueEx(this_key, "Path", 0, 0, 0, &value_size)
		 != ERROR_SUCCESS)
			continue;
		TCHAR value_buf[value_size + 1];
		if (RegQueryValueEx(this_key, "Path", 0, 0, (BYTE*)value_buf,
		 &value_size) != ERROR_SUCCESS)
			continue;
		value_buf[value_size] = 0;
		StringType current_path = value_buf;
		StringType::size_type fd;
		while ((fd = current_path.find(path)) != StringType::npos)
		{
			changes = true;
			if ((fd = current_path.find(path + ";")) != StringType::npos)
			{
				current_path.erase(fd, path.length() + 1);
				continue;
			}
			if ((fd = current_path.find(StringType(";") + path)) != StringType::npos)
			{
				current_path.erase(fd, path.length() + 1);
				continue;
			}
			fd = current_path.find(path);
			current_path.erase(fd, path.length());
		}
		HKEY write_key;
		if (RegCreateKeyEx(it->first, it->second, 0, 0, 0,
		 MaybeWOW64Flag() | KEY_WRITE, 0, &write_key, 0) != ERROR_SUCCESS)
			continue;
		if (RegSetValueEx(write_key, "Path", 0, REG_EXPAND_SZ,
		 (const BYTE*)current_path.c_str(),
		 (current_path.length() + 1) * sizeof(CharType))
		  != ERROR_SUCCESS)
			continue;
	}

	if (changes)
	{
		DWORD dwReturnValue;
		SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
		 (LPARAM)"Environment", SMTO_ABORTIFHUNG, 5000, &dwReturnValue);
	}
}


extern "C" void __declspec(dllexport) GetSystemID
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
	NSIS::UpdateParams(string_size, variables, stacktop, extra);

	StringType path = NSIS::popstring();

	const char* id = 0;
	tinyxml2::XMLDocument doc;
	if (doc.LoadFile((path + "\\__installer\\installed_man.txt").c_str()) == XML_SUCCESS)
	{
		XMLElement* sys_el
		 = XMLHandle(doc.RootElement()).FirstChildElement("System").ToElement();
		if (sys_el)
			id = sys_el->Attribute("id");
	}

	if (id && strlen(id) > 0)
		NSIS::pushstring(id);
	else
		NSIS::pushstring("");
}


extern "C" void __declspec(dllexport) Unload
(HWND hwndParent,
 int string_size,
 char *variables,
 stack_t **stacktop,
 extra_parameters *extra)
{
}


extern "C" BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	ginstance = (HINSTANCE)hInst;
	return TRUE;
}
