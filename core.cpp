#include "StdAfx.h"
#include <string>
#include "Scintilla.h"
#include "Report.h"

__declspec(dllexport)
LPCSTR prettyPrint(bool autoindenttext, bool addlinebreaks, const std::string &str_in) {	   	 
	std::string str = str_in;
	// count the < and > signs; > are ignored if tagsignlevel <= 0. This prevent indent errors if text or attributes contain > sign.
	int tagsignlevel = 0;
	// some state variables
	bool in_comment = false, in_header = false, in_attribute = false, in_nodetext = false, in_cdata = false, in_text = false;

	// some counters
	std::string::size_type curpos = 0, strlength = 0, prevspecchar, nexwchar_t, deltapos, tagend, startprev, endnext;
	long xmllevel = 0;
	// some char value (pc = previous char, cc = current char, nc = next char, nnc = next next char)
	char pc, cc, nc, nnc;

	const int tabwidth = 4;
	const int usetabs = 1;

	bool isclosingtag;
	const std::string eolchar("\r\n");
	const int eolmode = 0;

	// Proceed to first pass if break adds are enabled
	if (addlinebreaks) {
		while (curpos < str.length() && (curpos = str.find_first_of("<>", curpos)) != std::string::npos) {
			cc = str.at(curpos);

			if (cc == '<' && curpos < str.length() - 4 && !str.compare(curpos, 4, "<!--")) {
				// Let's skip the comment
				curpos = str.find("-->", curpos + 1) + 2;
				// Adds a line break after comment if required
				nexwchar_t = str.find_first_not_of(" \t", curpos + 1);
				if (nexwchar_t != std::string::npos && str.at(nexwchar_t) == '<') {
					str.insert(++curpos, eolchar);
				}
			}
			else if (cc == '<' && curpos < str.length() - 9 && !str.compare(curpos, 9, "<![CDATA[")) {
				// Let's skip the CDATA block
				curpos = str.find("]]>", curpos + 1) + 2;
				// Adds a line break after CDATA if required
				nexwchar_t = str.find_first_not_of(" \t", curpos + 1);
				if (nexwchar_t != std::string::npos && str.at(nexwchar_t) == '<') {
					str.insert(++curpos, eolchar);
				}
			}
			else if (cc == '>') {
				// Let's see if '>' is a end tag char (it might also be simple text)
				// To perform test, we search last of "<>". If it is a '>', current '>' is
				// simple text, if not, it is a end tag char. '>' text chars are ignored.
				prevspecchar = str.find_last_of("<>", curpos - 1);
				if (prevspecchar != std::string::npos) {
					// let's see if our '>' is in attribute
					std::string::size_type nextt = str.find_first_of("\"'", prevspecchar + 1);
					if (str.at(prevspecchar) == '>' && (nextt == std::string::npos || nextt > curpos)) {
						// current > is simple text, in text node
						++curpos;
						continue;
					}
					nextt = str.find_first_of("<>", curpos + 1);
					if (nextt != std::string::npos && str.at(nextt) == '>') {
						// current > is text in attribute
						++curpos;
						continue;
					}

					// We have found a '>' char and we are in a tag, let's see if it's an opening or closing one
					isclosingtag = ((curpos > 0 && str.at(curpos - 1) == '/') || str.at(prevspecchar + 1) == '/');

					nexwchar_t = str.find_first_not_of(" \t", curpos + 1);
					deltapos = nexwchar_t - curpos;
					// Test below identifies a <x><y> case and excludes <x>abc<y> case; in second case, we won't add line break
					if (nexwchar_t != std::string::npos && str.at(nexwchar_t) == '<' && curpos < str.length() - (deltapos + 1)) {
						// we compare previous and next tags; if they are same, we don't add line break
						startprev = str.rfind("<", curpos);
						endnext = str.find(">", nexwchar_t);

						if (startprev != std::string::npos && endnext != std::string::npos && curpos > startprev && endnext > nexwchar_t) {
							tagend = str.find_first_of(" />", startprev + 1);
							std::string tag1(str.substr(startprev + 1, tagend - startprev - 1));
							tagend = str.find_first_of(" />", nexwchar_t + 2);
							std::string tag2(str.substr(nexwchar_t + 2, tagend - nexwchar_t - 2));
							if (strcmp(tag1.c_str(), tag2.c_str()) || isclosingtag) {
								// Case of "<data><data>..." -> add a line break between tags
								str.insert(++curpos, eolchar);
							}
							else if (str.at(nexwchar_t + 1) == '/' && !isclosingtag && nexwchar_t == curpos + 1) {
								// Case of "<data id="..."></data>" -> replace by "<data id="..."/>"
								str.replace(curpos, endnext - curpos, "/");
							}
						}
					}
				}
			}

			++curpos;           // go to next char
		}
		/*
			  while (curpos < str.length()-2 && (curpos = str.find("><",curpos)) != std::string::npos) {
				// we compare previous and next tags; if they are same, we don't add line break
				startprev = str.rfind("<",curpos);
				endnext = str.find(">",curpos+1);

				if (startprev != std::string::npos &&
					endnext != std::string::npos &&
					curpos > startprev &&
					endnext > curpos+1 &&
					strcmp(str.substr(startprev+1,curpos-startprev-1).c_str(),
						   str.substr(curpos+3,endnext-curpos-3).c_str()))
				  str.insert(++curpos,"\n");

				++curpos;// go to next char
			  }
		*/

		// reinitialize cursor pos for second pass
		curpos = 0;
	}

	// Proceed to reformatting (second pass)
	prevspecchar = std::string::npos;
	std::string sep("<>\"'");
	char attributeQuote = '\0';
	sep += eolchar;
	strlength = str.length();
	while (curpos < strlength && (curpos = str.find_first_of(sep, curpos)) != std::string::npos) {
		if (!Report::isEOL(str, strlength, curpos, eolmode)) {
			if (curpos < strlength - 4 && !str.compare(curpos, 4, "<!--")) {
				in_comment = true;
			}
			if (curpos < strlength - 9 && !str.compare(curpos, 9, "<![CDATA[")) {
				in_cdata = true;
			}
			else if (curpos < strlength - 2 && !str.compare(curpos, 2, "<?")) {
				in_header = true;
			}
			else if (!in_comment && !in_cdata && !in_header &&
				curpos < strlength && (str.at(curpos) == '\"' || str.at(curpos) == '\'')) {
				if (in_attribute && attributeQuote != '\0' && str.at(curpos) == attributeQuote && prevspecchar != std::string::npos && str.at(prevspecchar) == '<') {
					// attribute end
					in_attribute = false;
					attributeQuote = '\0';  // store the attribute quote char to detect the end of attribute
				}
				else if (!in_attribute) {
					std::string::size_type x = str.find_last_not_of(" \t", curpos - 1);
					std::string::size_type tagbeg = str.find_last_of("<>", x - 1);
					std::string::size_type tagend = str.find_first_of("<>", curpos + 1);
					if (x != std::string::npos && tagbeg != std::string::npos && tagend != std::string::npos &&
						str.at(x) == '=' && str.at(tagbeg) == '<' && str.at(tagend) == '>') {
						in_attribute = true;
						attributeQuote = str.at(curpos);  // store the attribute quote char to detect the end of attribute
					}
				}
			}
		}

		if (!in_comment && !in_cdata && !in_header) {
			if (curpos > 0) {
				pc = str.at(curpos - 1);
			}
			else {
				pc = ' ';
			}

			cc = str.at(curpos);

			if (curpos < strlength - 1) {
				nc = str.at(curpos + 1);
			}
			else {
				nc = ' ';
			}

			if (curpos < strlength - 2) {
				nnc = str.at(curpos + 2);
			}
			else {
				nnc = ' ';
			}

			// managing of case where '>' char is present in text content
			in_text = false;
			if (cc == '>') {
				std::string::size_type tmppos = str.find_last_of("<>\"'", curpos - 1);
				// skip attributes which can contain > char
				while (tmppos != std::string::npos && (str.at(tmppos) == '\"' || str.at(tmppos) == '\'')) {
					tmppos = str.find_last_of("=", tmppos - 1);
					tmppos = str.find_last_of("<>\"'", tmppos - 1);
				}
				in_text = (tmppos != std::string::npos && str.at(tmppos) == '>');
			}

			if (cc == '<') {
				prevspecchar = curpos++;
				++tagsignlevel;
				in_nodetext = false;
				if (nc != '/' && (nc != '!' || nnc == '[')) {
					xmllevel += 2;
				}
			}
			else if (cc == '>' && !in_attribute && !in_text) {
				// > are ignored inside attributes
				if (pc != '/' && pc != ']') {
					--xmllevel;
					in_nodetext = true;
				}
				else {
					xmllevel -= 2;
				}

				if (xmllevel < 0) {
					xmllevel = 0;
				}
				--tagsignlevel;
				prevspecchar = curpos++;
			}
			else if (Report::isEOL(cc, nc, eolmode)) {
				if (eolmode == SC_EOL_CRLF) {
					++curpos; // skipping \n of \r\n
				}

				// \n are ignored inside attributes
				nexwchar_t = str.find_first_not_of(" \t", ++curpos);

				bool skipformat = false;
				if (!autoindenttext && nexwchar_t != std::string::npos) {
					cc = str.at(nexwchar_t);
					skipformat = (cc != '<' && cc != '\r' && cc != '\n');
				}
				if (nexwchar_t >= curpos && xmllevel >= 0 && !skipformat) {
					if (nexwchar_t < 0) {
						nexwchar_t = curpos;
					}
					int delta = 0;
					str.erase(curpos, nexwchar_t - curpos);
					strlength = str.length();

					if (curpos < strlength) {
						cc = str.at(curpos);
						// we set delta = 1 if we technically can + if we are in a text or inside an attribute
						if (xmllevel > 0 && curpos < strlength - 1 && ((cc == '<' && str.at(curpos + 1) == '/') || in_attribute)) {
							delta = 1;
						}
						else if (cc == '\n' || cc == '\r') {
							delta = xmllevel;
						}
					}

					if (usetabs) {
						str.insert(curpos, (xmllevel - delta), '\t');
					}
					else {
						str.insert(curpos, tabwidth*(xmllevel - delta), ' ');
					}
					strlength = str.length();
				}
			}
			else {
				++curpos;
			}
		}
		else {
			if (in_comment && curpos > 1 && !str.compare(curpos - 2, 3, "-->")) {
				in_comment = false;
			}
			else if (in_cdata && curpos > 1 && !str.compare(curpos - 2, 3, "]]>")) {
				in_cdata = false;
			}
			else if (in_header && curpos > 0 && !str.compare(curpos - 1, 2, "?>")) {
				in_header = false;
			}
			++curpos;
		}
	}
	LPSTR buff = (LPSTR)HeapAlloc(GetProcessHeap(), 0, str.size() + 1);
	strcpy(buff, str.c_str());
	return buff;
}

__declspec(dllexport)
LPCSTR prettyPrintAttributes(const std::string &str_in) {
	std::string str = str_in;
	const int tabwidth = 4;
	const int usetabs = 1;

	bool in_comment = false, in_header = false, in_attribute = false, in_nodetext = false, in_cdata = false;
	long curpos = 0, strlength = 0;
	std::string lineindent = "";
	char pc, cc, nc, nnc;
	int tagsignlevel = 0, nattrs = 0;

	const std::string eolchar = "\r\n";
	const int eolmode = 0;

	int prevspecchar = -1;
	while (curpos < (long)str.length() && (curpos = str.find_first_of("<>\"", curpos)) >= 0) {
		strlength = str.length();
		if (curpos < strlength - 3 && !str.compare(curpos, 4, "<!--")) in_comment = true;
		if (curpos < strlength - 8 && !str.compare(curpos, 9, "<![CDATA[")) in_cdata = true;
		else if (curpos < strlength - 1 && !str.compare(curpos, 2, "<?")) in_header = true;
		else if (curpos < strlength && !str.compare(curpos, 1, "\"") &&
			prevspecchar >= 0 && str.at(prevspecchar) == '<') in_attribute = !in_attribute;

		if (!in_comment && !in_cdata && !in_header) {
			if (curpos > 0) pc = str.at(curpos - 1);
			else pc = ' ';

			cc = str.at(curpos);

			if (curpos < strlength - 1) nc = str.at(curpos + 1);
			else nc = ' ';

			if (curpos < strlength - 2) nnc = str.at(curpos + 2);
			else nnc = ' ';

			if (cc == '<') {
				prevspecchar = curpos++;
				++tagsignlevel;
				in_nodetext = false;
				if (nc != '/' && (nc != '!' || nnc == '[')) nattrs = 0;
			}
			else if (cc == '>' && !in_attribute) {
				// > are ignored inside attributes
				if (pc != '/' && pc != ']') { in_nodetext = true; }
				--tagsignlevel;
				nattrs = 0;
				prevspecchar = curpos++;
			}
			else if (in_attribute) {
				if (++nattrs > 1) {
					long attrpos = str.find_last_of(eolchar + "\t ", curpos - 1) + 1;
					if (!Report::isEOL(str, strlength, attrpos, eolmode)) {
						long spacpos = str.find_last_not_of(eolchar + "\t ", attrpos - 1) + 1;
						str.replace(spacpos, attrpos - spacpos, lineindent);
						curpos -= attrpos - spacpos;
						curpos += lineindent.length();
					}
				}
				else {
					long attrpos = str.find_last_of(eolchar + "\t ", curpos - 1) + 1;
					if (!Report::isEOL(str, strlength, attrpos, eolmode)) {
						long linestart = str.find_last_of(eolchar, attrpos - 1) + 1;
						lineindent = str.substr(linestart, attrpos - linestart);
						long lineindentlen = lineindent.length();
						for (long i = 0; i < lineindentlen; ++i) {
							char lic = lineindent.at(i);
							if (lic != '\t' && lic != ' ') {
								lineindent.replace(i, 1, " ");
							}
						}
						lineindent.insert(0, eolchar);
					}
				}
				++curpos;
			}
			else {
				++curpos;
			}
		}
		else {
			if (in_comment && curpos > 1 && !str.compare(curpos - 2, 3, "-->")) in_comment = false;
			else if (in_cdata && curpos > 1 && !str.compare(curpos - 2, 3, "]]>")) in_cdata = false;
			else if (in_header && curpos > 0 && !str.compare(curpos - 1, 2, "?>")) in_header = false;
			++curpos;
		}
	}
	LPSTR buff = (LPSTR)HeapAlloc(GetProcessHeap(), 0, str.size() + 1);
	strcpy(buff, str.c_str());
	return buff;
}
