/** \file prune_manifest.cc
 *
 * Created: JohnE, 2020-01-02
 */


/*
DISCLAIMER:
The author(s) of this file's contents release it into the public domain, without
express or implied warranty of any kind. You may use, modify, and redistribute
this file freely.
*/


#include <iostream>
#include <set>
#include <stack>
#include <string>
#include <tinyxml2.h>

#include "ref.hpp"


using namespace tinyxml2;
using std::string;


struct PMPrinter : public XMLPrinter
{
	const std::set <const XMLElement*>* selected;
	const XMLElement* skip_element = nullptr;
	virtual bool VisitEnter(const XMLElement& el, const XMLAttribute* attr)
	{
		if (skip_element)
			return false;
		const char* elname = el.Name();
		if (!selected->count(&el) &&
			(strcmp(elname, "Version") == 0 || strcmp(elname, "Component") == 0
			|| strcmp(elname, "Category") == 0 || strcmp(elname, "InstallType") == 0
			|| strcmp(elname, "System") == 0))
		{
			skip_element = &el;
			return true;
		}
		return XMLPrinter::VisitEnter(el, attr);
	}
	virtual bool VisitExit(const XMLElement& el)
	{
		if (skip_element)
		{
			if (&el == skip_element)
			{
				skip_element = nullptr;
				return true;
			}
			return false;
		}
		return XMLPrinter::VisitExit(el);
	}
};


struct PMSelector : public XMLVisitor
{
	std::stack <const XMLElement*> visit_stack;
	std::set <string> select_ids;
	std::set <const XMLElement*>* selected;
	virtual bool VisitEnter(const XMLElement& el, const XMLAttribute* attr)
	{
		visit_stack.push(&el);
		if (strcmp(el.Name(), "Select") == 0 || strcmp(el.Name(), "SelectTree") == 0)
			return true;
		const char* el_id = el.Attribute("id");
		if (el_id && select_ids.count(string(el_id)) > 0)
			selected->insert(&el);
		return true;
	}
	virtual bool VisitExit(const XMLElement& el)
	{
		visit_stack.pop();
		if (visit_stack.size() > 0 && selected->count(&el))
			selected->insert(visit_stack.top());
		return true;
	}
};


int main()
{
	XMLDocument doc;
	{
		string mft_file_path;
		std::getline(std::cin, mft_file_path);
		if (doc.LoadFile(mft_file_path.c_str()) != XML_SUCCESS)
		{
			fprintf(stderr, "\nCouldn't load '%s' as XML\n", mft_file_path.c_str());
			return -1;
		}
	}

	string system_id;
	std::getline(std::cin, system_id);
	if (system_id.empty())
	{
		fprintf(stderr, "\nUnexpected EOF on input, looking for system ID\n");
		return -1;
	}

	string install_type_id;
	std::getline(std::cin, install_type_id);
	if (install_type_id.empty())
	{
		fprintf(stderr, "\nUnexpected EOF on input, looking for InstallType ID\n");
		return -1;
	}

	/* Get the first System element */
	const XMLElement* sys_el = XMLHandle(doc.FirstChildElement()).FirstChildElement("System").ToElement();
	for (; sys_el && system_id != sys_el->Attribute("id"); sys_el = sys_el->NextSiblingElement("System"))
		;
	if (!sys_el)
	{
		fprintf(stderr, "\nCouldn't find System element with ID '%s'\n", system_id.c_str());
		return -1;
	}

	const XMLElement* itype_el = sys_el->FirstChildElement("InstallType");
	for (; itype_el && install_type_id != itype_el->Attribute("id"); itype_el = itype_el->NextSiblingElement("InstallType"))
		;
	if (!sys_el)
	{
		fprintf(stderr, "\nCouldn't find InstallType element with ID '%s'\n", install_type_id.c_str());
		return -1;
	}

	PMSelector sel;

	const XMLElement* select_el = itype_el->FirstChildElement();
	for (; select_el; select_el = select_el->NextSiblingElement())
	{
		if (strcmp(select_el->Name(), "Select") == 0 || strcmp(select_el->Name(), "SelectTree") == 0)
			sel.select_ids.insert(select_el->Attribute("id"));
	}

	std::set <const XMLElement*> selected;
	selected.insert(itype_el);
	sel.selected = &selected;
	sys_el->Accept(&sel);

	PMPrinter pmp;
	pmp.selected = &selected;
	doc.Accept(&pmp);
	fputs(pmp.CStr(), stdout);

	return 0;
}
