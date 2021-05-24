/** \file componentstree.hpp
 *
 * Created: JohnE, 2008-07-24
 */


/*
DISCLAIMER:
The author(s) of this file's contents release it into the public domain, without
express or implied warranty of any kind. You may use, modify, and redistribute
this file freely.
*/


#ifndef COMPONENTSTREE_HPP_INC
#define COMPONENTSTREE_HPP_INC


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <tinyxml2.h>
#include "ref.hpp"


typedef std::string StringType;
typedef std::list< tinyxml2::XMLElement* > ElementList;


enum ItemType
{
	ITEM_HEADER = 0,
	ITEM_CATEGORY = 1,
	ITEM_COMPONENT = 2,
	ITEM_VERSION = 3,
	ITEM_EXTRA = 4
};


enum CheckState
{
	CHECK_NONE = 0,
	CHECK_FULL,
	CHECK_PARTIAL
};


struct Item;
class InstallManifest;

class ComponentsTree
{
public:
	ComponentsTree();

	void SetInnerArchive(const StringType& arc);

	bool BuildTreeView
	 (HWND htv,
	  int check_index,
	  int radio_index,
	  const StringType& system_id,
	  tinyxml2::XMLElement* comp_man_root,
	  tinyxml2::XMLElement* prev_man_root,
	  bool startmenu_selected,
	  bool addpath_selected);

	bool OnStateToggle(HTREEITEM hitem);

	int GetArchivesToInstall(ElementList& list) const;
	int GetComponentsToRemove(ElementList& list) const;

	void WriteInstMan
	 (const StringType& outpath,
	  const InstallManifest& inst_man);

	int GetIndex(HTREEITEM hitem) const;
	int GetIndex(const StringType& id) const;

	bool IsSelected(int index) const;
	void SetSelected(int index, bool selected);
	void SetPrevInstSelected();
	void DeselectAll();

	const tinyxml2::XMLElement* GetElement(int index) const;

	StringType GetDescription(int index) const;

	unsigned GetDownloadSize() const;
	unsigned GetInstallSize() const;
	unsigned GetUninstallSize() const;

private:
	bool ProcessManifest
	 (tinyxml2::XMLElement* mroot,
	  const StringType& system_id,
	  HTREEITEM hroot);
	HTREEITEM AddItem
	 (tinyxml2::XMLElement* ex_el,
	  tinyxml2::XMLElement* store_el,
	  ItemType type,
	  const StringType& id,
	  const StringType& txt,
	  HTREEITEM parent);
	Item* GetItemFromHandle(HTREEITEM handle);
	void UpdateCategoryChildren(HTREEITEM hcat, bool checked);
	void UpdateAncestors(HTREEITEM hitem, bool checked);
	void ToggleSizes(Item* item, bool selected);
	void UpdateVersionedComps(HTREEITEM hparent, HTREEITEM hver);
	void EnsureSelectable(Item* item);
	void ToggleSel(Item* item);
	UINT SetCheckState(Item* item, CheckState state);
	void UpdateItemAttrs(Item* item, tinyxml2::XMLElement* attr_el, bool prev_inst);
	void EnsurePrevText(Item* item);

	HWND htreeview;
	int nradio;
	int ncheck;
	RefType< tinyxml2::XMLDocument >::Ref m_doc;
	std::vector< RefType< Item >::Ref > index_items;
	typedef std::map< StringType, int > IDIndexMap;
	IDIndexMap id_items;
	unsigned dl_size;
	unsigned inst_size;
	unsigned uninst_size;
	std::set< StringType > inner_arcs;
};


#endif // COMPONENTSTREE_HPP_INC
