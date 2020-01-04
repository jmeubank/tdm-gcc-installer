/** \file ctemplate.cpp
 *
 * Created: JohnE, 2008-07-29
 */


/*
DISCLAIMER:
The author(s) of this file's contents release it into the public domain, without
express or implied warranty of any kind. You may use, modify, and redistribute
this file freely.
*/


#include <deque>
#include <iostream>
#include <list>
#include <string>
#include <tinyxml2.h>

extern "C" {
#include <regexp.h>
}

#include "ref.hpp"


using namespace tinyxml2;
using std::string;


struct CTVisitor : public XMLVisitor
{
	virtual bool VisitEnter(const XMLElement& el, const XMLAttribute* attr)
	{
		/* if we've already matched to the end of the deque, we don't need to
		 * match child elements
		 */
		if (matched_selector_in_deque >= selectors.size() - 1)
			return false;
		string linestr = selectors[matched_selector_in_deque + 1].first;
		string elementtypestr = linestr.substr(0, linestr.find(":"));
		/* If we didn't match the element name, don't try to do anything
		 * else with this element, but do check its siblings and children
		 */
		if (elementtypestr != el.Name())
			return true;
		linestr.erase(0, elementtypestr.length() + 1);
		string attributetypestr = linestr.substr(0, linestr.find(":"));
		if (attributetypestr.length() > 0)
		{
			/* If we didn't have the required attribute, don't emit
			 * but do continue on to siblings/children.
			 */
			if (!el.FindAttribute(attributetypestr.c_str()))
				return true;
		}
		linestr.erase(0, attributetypestr.length() + 1);
		Resub rmatches;
		if (linestr.length() > 0)
		{
			const char* rcerr = nullptr;
			RefType< Reprog >::Ref rgx(regcomp(linestr.c_str(), 0, &rcerr), regfree);
			/* If we don't match the regex, don't emit but do search siblings
			 * and children.
			 */
			if (regexec(RefGetPtr(rgx), el.Attribute(attributetypestr.c_str()), &rmatches, 0) != 0)
				return true;
		}
		/* If we make it to this point, we've matched all attributes in the
		 * next selector.
		 */
		++matched_selector_in_deque;
		selectors[matched_selector_in_deque].second = &el;
		/* If we haven't reached the end of the deque, carry on the search. */
		if (matched_selector_in_deque != selectors.size() - 1)
			return true;
		/* If we have reached the end of the deque, render the template. (And
		 * carry on the search.)
		 */
		for (const auto &line : out_template)
		{
			const char* estr = line.c_str();
			for (size_t at = 0; estr[at]; ++at)
			{
				if (estr[at] == '@')
				{
					size_t end = at + 1;
					while (estr[end] && estr[end] != '@')
						++end;
					if (estr[end])
					{
						if (end - at == 1)
							fputc('@', stdout);
						else
						{
							string var(estr + at + 1, end - at - 1);
							if (var == "()" && rmatches.nsub >= 2)
							{
								/* @()@ means render the first matched sub-
								 * regex.
								 */
								for (const char* mch = rmatches.sub[1].sp; mch != rmatches.sub[1].ep; ++mch)
									fputc(*mch, stdout);
							}
							else if (var == "[]")
							{
								/* @[]@ means render the entire matched XML
								 * element.
								 */
								XMLPrinter p;
								el.Accept(&p);
								fputs(p.CStr(), stdout);
							}
							else if (el.FindAttribute(var.c_str()))
							{
								/* Otherwise, render just the attribute's value */
								const char* attrval = el.Attribute(var.c_str());
								if (attrval && strlen(attrval) > 0)
									fputs(attrval, stdout);
							}
						}
					}
					at = end;
				}
				else
					fputc(estr[at], stdout);
			}
			fprintf(stdout, "\n");
		}
		return true;
	}
	virtual bool VisitExit(const XMLElement& el)
	{
		if (selectors[matched_selector_in_deque].second == &el)
		{
			selectors[matched_selector_in_deque].second = nullptr;
			--matched_selector_in_deque;
		}
		return true;
	}
	std::deque< std::pair< string, const XMLElement* > > selectors;
	size_t matched_selector_in_deque;
	std::list< string > out_template;
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

	while (!std::cin.eof())
	{
		/* Set up the visitor struct */
		CTVisitor v;
		v.matched_selector_in_deque = 0;
		v.selectors.push_back(
			std::make_pair(
				string(),
				static_cast< const XMLElement* >(nullptr)
			)
		);
		/* Read in selector(s) until an end marker is found */
		while (true)
		{
			string linestr;
			getline(std::cin, linestr);
			if (linestr.empty())
			{
				fprintf(stderr, "\nUnexpected EOF reading selectors\n");
				return -1;
			}
			if (linestr == "]]>]]>")
				break;
			v.selectors.push_back(
				std::make_pair(
					linestr,
					static_cast< const XMLElement* >(nullptr)
				)
			);
		}
		/* Read in the template to be emitted, and store it in
		 * the visitor struct.
		 */
		while (!std::cin.eof())
		{
			string linestr;
			getline(std::cin, linestr);
			if (linestr == "]]>]]>")
				break;
			v.out_template.push_back(linestr);
		}
		/* Run the visitor algo, which will emit the templated
		 * output to stdout.
		 */
		doc.Accept(&v);
	}

	return 0;
}
