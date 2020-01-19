/** \file install_manifest.hpp
 *
 * Created: JohnE, 2008-07-11
 */


/*
DISCLAIMER:
The author(s) of this file's contents release it into the public domain, without
express or implied warranty of any kind. You may use, modify, and redistribute
this file freely.
*/


#ifndef INSTALL_MANIFEST_HPP_INC
#define INSTALL_MANIFEST_HPP_INC


#include <string>
#include <set>
#include <map>
#include <tinyxml2.h>


typedef std::string StringType;


class InstallManifest
{
public:
	InstallManifest(const StringType& loadfile = StringType());

	const tinyxml2::XMLElement* GetComponent(const StringType& comp_id) const;
	tinyxml2::XMLElement* SetComponent(const StringType& comp_id);
	void MarkComponentSuccess(bool success);
	void AddEntry(const char* entry);

private:
	typedef std::map< std::string, std::pair< tinyxml2::XMLElement*, std::set< std::string > > > EntrySetMap;
	EntrySetMap entry_setmap;
	EntrySetMap::iterator cur_comp;
	tinyxml2::XMLDocument doc;
};


#endif // INSTALL_MANIFEST_HPP_INC
