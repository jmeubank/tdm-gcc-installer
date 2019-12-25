/** \file archive.cpp
 *
 * Format-agnostic functions for unpacking archives. Called from tdminstall.cpp;
 * calls various functions in archive_7z.cpp, archive_base.c, archive_zip.cpp,
 * and untgz.c.
 */

/* Created: JohnE, 2008-07-11
 */


/*
DISCLAIMER:
The author(s) of this file's contents release it into the public domain, without
express or implied warranty of any kind. You may use, modify, and redistribute
this file freely.
*/


#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <string>
#include "install_manifest.hpp"
#include "ref.hpp"
#include <xarc.hpp>
#include <xarc/xarc_exception.hpp>


// Set when InstallArchive is called; called from ArcBeforeCallback to indicate
// that an entry is about to be unpacked
static int (*ia_callback)(const char*, bool, bool, bool) = 0;

// Set when InstallArchive is called; entries are added to it in
// ArcCreateCallback
InstallManifest* inst_man = 0;


static std::string fmt(const char* format_string, ...)
{
	char buf[2048];
	va_list ap;
	va_start(ap, format_string);
	vsnprintf(buf, 2048, format_string, ap);
	va_end(ap);
	return std::string(buf);
}

static std::string XarcCombinedErrorString(XARC::ExtractArchive const& xa)
{
	return fmt("(%d) %s : %s", xa.GetXarcErrorID(),
	 xa.GetErrorDescription().c_str(), xa.GetErrorAdditional().c_str());
}

void InstallArchiveXarcCallback(const std::string& item, uint8_t flags)
{
	if (!inst_man)
		return;
	if (flags & XARC_PROP_DIR)
	{
		// Ensure that the path ends in a slash if it is a directory
		if (item[item.length() - 1] == '/'
		 || item[item.length() - 1] == '\\')
			inst_man->AddEntry(item.c_str());
		else
			inst_man->AddEntry((item + "/").c_str());
	}
	else
		inst_man->AddEntry(item.c_str());
};

/* Unpack an archive to the specified location, with callbacks. "base" is the
 * location to unpack to, "archive" is the file to unpack, "man" is the
 * installation manifest to add unpacked entries to, and "callback" is a
 * function to call right before each entry is unpacked. "callback"'s arguments
 * are the path (relative to base) of the entry, a boolean that is true if the
 * entry is a directory, and a boolean that is false if the entry is being
 * created (in other words, always false for InstallArchive) */
std::string InstallArchive
 (const char* base,
  const std::string& archive,
  InstallManifest& man,
  int (*callback)(const char*, bool, bool, bool))
{
	ia_callback = callback;
	inst_man = &man;

	if (archive.length() <= 0)
		return "No archive given";
	else if (archive.length() < 5)
		return std::string("Archive name '") + archive + "' is too short.";

	try
	{
		XARC::ExtractArchive xa(archive.c_str());
		if (!xa.IsOkay())
			return XarcCombinedErrorString(xa);

		XARC::ExtractItemInfo xi;
		do
		{
			xi = xa.GetItemInfo();
			if (callback)
			{
				if (callback(xi.GetPath().c_str(), xi.IsDirectory() ? 1 : 0,
				 false, false) != 0)
					return fmt("Failed to unpack '%s': cancelled");
			}
			if (xa.ExtractItem(base, XARC_XFLAG_CALLBACK_DIRS, InstallArchiveXarcCallback)
			 != XARC_OK)
				return XarcCombinedErrorString(xa);
		} while (xa.NextItem() == XARC_OK);
	}
	catch (XARC::XarcException& e)
	{
		return e.GetString();
	}

	return "OK";
}

