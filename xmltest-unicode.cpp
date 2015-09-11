#if defined( _MSC_VER )
	#if !defined( _CRT_SECURE_NO_WARNINGS )
		#define _CRT_SECURE_NO_WARNINGS		// This test file is not intended to be secure.
	#endif
#endif

#include "tinyxml2.h"
#include <cstdlib>
#include <cstring>
#include <ctime>

#if defined( _MSC_VER )
	#include <direct.h>		// _mkdir
	#include <crtdbg.h>
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	_CrtMemState startMemState;
	_CrtMemState endMemState;
#elif defined(MINGW32) || defined(__MINGW32__)
    #include <io.h>  // mkdir
#else
	#include <sys/stat.h>	// mkdir
#endif

using namespace tinyxml2;
using namespace std;
int gPass = 0;
int gFail = 0;

template<typename xchar>
inline std::size_t strtlen(const xchar *pStr)
{
    const xchar *tmp = pStr;
    while (*tmp) 
        ++tmp;
    return tmp - pStr;
}

template<typename T>
bool XMLTest (const wchar_t* testString, const T* expected, const T* found, bool echo=true, bool extraNL=false )
{
	bool pass;
	if ( !expected && !found )
		pass = true;
	else if ( !expected || !found )
		pass = false;
	else 
		pass = !memcmp(expected, found, strtlen(expected));//!wcscmp( expected, found );
	if ( pass )
		printf ("[pass]");
	else
		printf ("[fail]");

	if ( !echo ) {
		wprintf (L" %s\n", testString);
	}
	else {
		if ( extraNL ) {
			wprintf( L" %s\n", testString );
			wprintf( L"%s\n", expected );
			wprintf( L"%s\n", found );
		}
		else {
			wprintf (L" %s [%s][%s]\n", testString, expected, found);
		}
	}

	if ( pass )
		++gPass;
	else
		++gFail;
	return pass;
}


template< class T > 
bool XMLTest( const wchar_t* testString, T expected, T found, bool echo=true )
{
	bool pass = ( expected == found );
	if ( pass )
		printf ("[pass]");
	else
		printf ("[fail]");

	if ( !echo )
		wprintf (L" %s\n", testString);
	else
		wprintf (L" %s [%d][%d]\n", testString, static_cast<int>(expected), static_cast<int>(found) );

	if ( pass )
		++gPass;
	else
		++gFail;
	return pass;
}


void NullLineEndings( char* p )
{
	while( p && *p ) {
		if ( *p == '\n' || *p == '\r' ) {
			*p = 0;
			return;
		}
		++p;
	}
}


int example_1()
{
	XMLDocument doc;
	doc.LoadFile( L"resources/Unicode/dream.xml" );

	return doc.ErrorID();
}
/** @page Example-1 Load an XML File
 *  @dontinclude ./xmltest.cpp
 *  Basic XML file loading.
 *  The basic syntax to load an XML file from
 *  disk and check for an error. (ErrorID()
 *  will return 0 for no error.)
 *  @skip example_1()
 *  @until }
 */
 

int example_2()
{
	static const wchar_t* xml = L"<element/>";
	XMLDocument doc;
	doc.Parse( xml );

	return doc.ErrorID();
}
/** @page Example-2 Parse an XML from char buffer
 *  @dontinclude ./xmltest.cpp
 *  Basic XML string parsing.
 *  The basic syntax to parse an XML for
 *  a char* and check for an error. (ErrorID()
 *  will return 0 for no error.)
 *  @skip example_2()
 *  @until }
 */


int example_3()
{
	static const wchar_t* xml =
		L"<?xml version=\"1.0\"?>"
		L"<!DOCTYPE PLAY SYSTEM \"play.dtd\">"
		L"<PLAY>"
		L"<TITLE>A Midsummer Night's Dream</TITLE>"
		L"</PLAY>";

	XMLDocument doc;
	doc.Parse( xml );

	XMLElement* titleElement = doc.FirstChildElement( L"PLAY" )->FirstChildElement( L"TITLE" );
	const wchar_t* title = titleElement->GetText();
	wprintf( L"Name of play (1): %s\n", title );

	XMLText* textNode = titleElement->FirstChild()->ToText();
	title = textNode->Value();
	wprintf( L"Name of play (2): %s\n", title );

	return doc.ErrorID();
}
/** @page Example-3 Get information out of XML
	@dontinclude ./xmltest.cpp
	In this example, we navigate a simple XML
	file, and read some interesting text. Note
	that this example doesn't use error
	checking; working code should check for null
	pointers when walking an XML tree, or use
	XMLHandle.
	
	(The XML is an excerpt from "dream.xml"). 

	@skip example_3()
	@until </PLAY>";

	The structure of the XML file is:

	<ul>
		<li>(declaration)</li>
		<li>(dtd stuff)</li>
		<li>Element "PLAY"</li>
		<ul>
			<li>Element "TITLE"</li>
			<ul>
			    <li>Text "A Midsummer Night's Dream"</li>
			</ul>
		</ul>
	</ul>

	For this example, we want to print out the 
	title of the play. The text of the title (what
	we want) is child of the "TITLE" element which
	is a child of the "PLAY" element.

	We want to skip the declaration and dtd, so the
	method FirstChildElement() is a good choice. The
	FirstChildElement() of the Document is the "PLAY"
	Element, the FirstChildElement() of the "PLAY" Element
	is the "TITLE" Element.

	@until ( "TITLE" );

	We can then use the convenience function GetText()
	to get the title of the play.

	@until title );

	Text is just another Node in the XML DOM. And in
	fact you should be a little cautious with it, as
	text nodes can contain elements. 
	
	@verbatim
	Consider: A Midsummer Night's <b>Dream</b>
	@endverbatim

	It is more correct to actually query the Text Node
	if in doubt:

	@until title );

	Noting that here we use FirstChild() since we are
	looking for XMLText, not an element, and ToText()
	is a cast from a Node to a XMLText. 
*/


bool example_4()
{
	static const wchar_t* xml =
		L"<information>"
		L"	<attributeApproach v='2' />"
		L"	<textApproach>"
		L"		<v>2</v>"
		L"	</textApproach>"
		L"</information>";

	XMLDocument doc;
	doc.Parse( xml );

	int v0 = 0;
	int v1 = 0;

	XMLElement* attributeApproachElement = doc.FirstChildElement()->FirstChildElement( L"attributeApproach" );
	attributeApproachElement->QueryIntAttribute( L"v", &v0 );

	XMLElement* textApproachElement = doc.FirstChildElement()->FirstChildElement( L"textApproach" );
	textApproachElement->FirstChildElement( L"v" )->QueryIntText( &v1 );

	printf( "Both values are the same: %d and %d\n", v0, v1 );

	return !doc.Error() && ( v0 == v1 );
}
/** @page Example-4 Read attributes and text information.
	@dontinclude ./xmltest.cpp

	There are fundamentally 2 ways of writing a key-value
	pair into an XML file. (Something that's always annoyed
	me about XML.) Either by using attributes, or by writing
	the key name into an element and the value into
	the text node wrapped by the element. Both approaches
	are illustrated in this example, which shows two ways
	to encode the value "2" into the key "v":

	@skip example_4()
	@until "</information>";

	TinyXML-2 has accessors for both approaches. 

	When using an attribute, you navigate to the XMLElement
	with that attribute and use the QueryIntAttribute()
	group of methods. (Also QueryFloatAttribute(), etc.)

	@skip XMLElement* attributeApproachElement
	@until &v0 );

	When using the text approach, you need to navigate
	down one more step to the XMLElement that contains
	the text. Note the extra FirstChildElement( "v" )
	in the code below. The value of the text can then
	be safely queried with the QueryIntText() group
	of methods. (Also QueryFloatText(), etc.)

	@skip XMLElement* textApproachElement
	@until &v1 );
*/


int main( int argc, const char ** argv )
{
	#if defined( _MSC_VER ) && defined( DEBUG )
		_CrtMemCheckpoint( &startMemState );
		// Enable MS Visual C++ debug heap memory leaks dump on exit
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	#endif

	#if defined(_MSC_VER) || defined(MINGW32) || defined(__MINGW32__)
		#if defined __MINGW64_VERSION_MAJOR && defined __MINGW64_VERSION_MINOR
			//MINGW64: both 32 and 64-bit
			mkdir( "resources/Unicode/out/" );
                #else
                	_mkdir( "resources/Unicode/out/" );
                #endif
	#else
		mkdir( "resources/Unicode/out/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	#endif

	{
		TIXMLASSERT( true );
	}

	if ( argc > 1 ) {
		XMLDocument* doc = new XMLDocument();
		clock_t startTime = clock();
		doc->LoadFile( (wchar_t*)argv[1] );
 		clock_t loadTime = clock();
		int errorID = doc->ErrorID();
		delete doc; doc = 0;
 		clock_t deleteTime = clock();

		printf( "Test file '%s' loaded. ErrorID=%d\n", argv[1], errorID );
		if ( !errorID ) {
			printf( "Load time=%u\n",   (unsigned)(loadTime - startTime) );
			printf( "Delete time=%u\n", (unsigned)(deleteTime - loadTime) );
			printf( "Total time=%u\n",  (unsigned)(deleteTime - startTime) );
		}
		exit(0);
	}

	FILE* fp = fopen( "resources/Unicode/dream.xml", "r" );
	if ( !fp ) {
		printf( "Error opening test file 'dream.xml'.\n"
				"Is your working directory the same as where \n"
				"the xmltest.cpp and dream.xml file are?\n\n"
	#if defined( _MSC_VER )
				"In windows Visual Studio you may need to set\n"
				"Properties->Debugging->Working Directory to '..'\n"
	#endif
			  );
		exit( 1 );
	}
	fclose( fp );

	XMLTest( L"Example-1", 0, example_1() );
	XMLTest( L"Example-2", 0, example_2() );
	XMLTest( L"Example-3", 0, example_3() );
	XMLTest( L"Example-4", true, example_4() );

	/* ------ Example 2: Lookup information. ---- */

	{
		static const wchar_t* test[] = {	L"<element />",
										L"<element></element>",
										L"<element><subelement/></element>",
										L"<element><subelement></subelement></element>",
										L"<element><subelement><subsub/></subelement></element>",
										L"<!--comment beside elements--><element><subelement></subelement></element>",
										L"<!--comment beside elements, this time with spaces-->  \n <element>  <subelement> \n </subelement> </element>",
										L"<element attrib1='foo' attrib2=\"bar\" ></element>",
										L"<element attrib1='foo' attrib2=\"bar\" ><subelement attrib3='yeehaa' /></element>",
										L"<element>Text inside element.</element>",
										L"<element><b></b></element>",
										L"<element>Text inside and <b>bolded</b> in the element.</element>",
										L"<outer><element>Text inside and <b>bolded</b> in the element.</element></outer>",
										L"<element>This &amp; That.</element>",
										L"<element attrib='This&lt;That' />",
										0
		};
		for( int i=0; test[i]; ++i ) {
			XMLDocument doc;
			doc.Parse( test[i] );
			doc.Print();
			printf( "----------------------------------------------\n" );
		}
	}
#if 1
	{
		static const wchar_t* test = L"<!--hello world\n"
								  L"          line 2\r"
								  L"          line 3\r\n"
								  L"          line 4\n\r"
								  L"          line 5\r-->";

		XMLDocument doc;
		doc.Parse( test );
		doc.Print();
	}

	{
		static const wchar_t* test = L"<element>Text before.</element>";
		XMLDocument doc;
		doc.Parse( test );
		XMLElement* root = doc.FirstChildElement();
		XMLElement* newElement = doc.NewElement( L"Subelement" );
		root->InsertEndChild( newElement );
		doc.Print();
	}
	{
		XMLDocument* doc = new XMLDocument();
		static const wchar_t* test = L"<element><sub/></element>";
		doc->Parse( test );
		delete doc;
	}
	{
		// Test: Programmatic DOM
		// Build:
		//		<element>
		//			<!--comment-->
		//			<sub attrib="1" />
		//			<sub attrib="2" />
		//			<sub attrib="3" >& Text!</sub>
		//		<element>

		XMLDocument* doc = new XMLDocument();
		XMLNode* element = doc->InsertEndChild( doc->NewElement( L"element" ) );

		XMLElement* sub[3] = { doc->NewElement( L"sub" ), doc->NewElement( L"sub" ), doc->NewElement( L"sub" ) };
		for( int i=0; i<3; ++i ) {
			sub[i]->SetAttribute( L"attrib", i );
		}
		element->InsertEndChild( sub[2] );
		XMLNode* comment = element->InsertFirstChild( doc->NewComment( L"comment" ) );
		element->InsertAfterChild( comment, sub[0] );
		element->InsertAfterChild( sub[0], sub[1] );
		sub[2]->InsertFirstChild( doc->NewText( L"& Text!" ));
		doc->Print();
		XMLTest( L"Programmatic DOM", L"comment", doc->FirstChildElement( L"element" )->FirstChild()->Value() );
		XMLTest( L"Programmatic DOM", L"0", doc->FirstChildElement( L"element" )->FirstChildElement()->Attribute( L"attrib" ) );
		XMLTest( L"Programmatic DOM", 2, doc->FirstChildElement()->LastChildElement( L"sub" )->IntAttribute( L"attrib" ) );
		XMLTest( L"Programmatic DOM", L"& Text!",
				 doc->FirstChildElement()->LastChildElement( L"sub" )->FirstChild()->ToText()->Value() );

		// And now deletion:
		element->DeleteChild( sub[2] );
		doc->DeleteNode( comment );

		element->FirstChildElement()->SetAttribute( L"attrib", true );
		element->LastChildElement()->DeleteAttribute( L"attrib" );

		XMLTest( L"Programmatic DOM", true, doc->FirstChildElement()->FirstChildElement()->BoolAttribute( L"attrib" ) );
		int value = 10;
		int result = doc->FirstChildElement()->LastChildElement()->QueryIntAttribute( L"attrib", &value );
		XMLTest( L"Programmatic DOM", result, (int)XML_NO_ATTRIBUTE );
		XMLTest( L"Programmatic DOM", value, 10 );

		doc->Print();

		{
			XMLPrinter streamer;
			doc->Print( &streamer );
			wprintf( L"%s", streamer.CStr() );
		}
		{
			XMLPrinter streamer( 0, true );
			doc->Print( &streamer );
			XMLTest( L"Compact mode", L"<element><sub attrib=\"1\"/><sub/></element>", streamer.CStr(), false );
		}
		doc->SaveFile( L"./resources/Unicode/out/pretty.xml" );
		doc->SaveFile( L"./resources/Unicode/out/compact.xml", true );
		delete doc;
	}
	{
		// Test: Dream
		// XML1 : 1,187,569 bytes	in 31,209 allocations
		// XML2 :   469,073	bytes	in    323 allocations
		//int newStart = gNew;
		XMLDocument doc;
		doc.LoadFile( L"resources/Unicode/dream.xml" );

		doc.SaveFile( L"resources/Unicode/out/dreamout.xml" );
		doc.PrintError();

		XMLTest( L"Dream", L"xml version=\"1.0\"",
						  doc.FirstChild()->ToDeclaration()->Value() );
		XMLTest( L"Dream", true, doc.FirstChild()->NextSibling()->ToUnknown() ? true : false );
		XMLTest( L"Dream", L"DOCTYPE PLAY SYSTEM \"play.dtd\"",
						  doc.FirstChild()->NextSibling()->ToUnknown()->Value() );
		XMLTest( L"Dream", L"And Robin shall restore amends.",
						  doc.LastChild()->LastChild()->LastChild()->LastChild()->LastChildElement()->GetText() );
		XMLTest( L"Dream", L"And Robin shall restore amends.",
						  doc.LastChild()->LastChild()->LastChild()->LastChild()->LastChildElement()->GetText() );

		XMLDocument doc2;
		doc2.LoadFile( L"resources/Unicode/out/dreamout.xml" );
		XMLTest( L"Dream-out", L"xml version=\"1.0\"",
						  doc2.FirstChild()->ToDeclaration()->Value() );
		XMLTest( L"Dream-out", true, doc2.FirstChild()->NextSibling()->ToUnknown() ? true : false );
		XMLTest( L"Dream-out", L"DOCTYPE PLAY SYSTEM \"play.dtd\"",
						  doc2.FirstChild()->NextSibling()->ToUnknown()->Value() );
		XMLTest( L"Dream-out", L"And Robin shall restore amends.",
						  doc2.LastChild()->LastChild()->LastChild()->LastChild()->LastChildElement()->GetText() );

		//gNewTotal = gNew - newStart;
	}


	{
		const wchar_t* error =	L"<?xml version=\"1.0\" standalone=\"no\" ?>\n"
							L"<passages count=\"006\" formatversion=\"20020620\">\n"
							L"    <wrong error>\n"
							L"</passages>";

		XMLDocument doc;
		doc.Parse( error );
		XMLTest( L"Bad XML", doc.ErrorID(), XML_ERROR_PARSING_ATTRIBUTE );
	}

	{
		const wchar_t* str = L"<doc attr0='1' attr1='2.0' attr2='foo' />";

		XMLDocument doc;
		doc.Parse( str );

		XMLElement* ele = doc.FirstChildElement();

		int iVal, result;
		double dVal;

		result = ele->QueryDoubleAttribute( L"attr0", &dVal );
		XMLTest( L"Query attribute: int as double", result, (int)XML_NO_ERROR );
		XMLTest( L"Query attribute: int as double", (int)dVal, 1 );
		result = ele->QueryDoubleAttribute( L"attr1", &dVal );
		XMLTest( L"Query attribute: double as double", result, (int)XML_NO_ERROR );
		XMLTest( L"Query attribute: double as double", (int)dVal, 2 );
		result = ele->QueryIntAttribute( L"attr1", &iVal );
		XMLTest( L"Query attribute: double as int", result, (int)XML_NO_ERROR );
		XMLTest( L"Query attribute: double as int", iVal, 2 );
		result = ele->QueryIntAttribute( L"attr2", &iVal );
		XMLTest( L"Query attribute: not a number", result, (int)XML_WRONG_ATTRIBUTE_TYPE );
		result = ele->QueryIntAttribute( L"bar", &iVal );
		XMLTest( L"Query attribute: does not exist", result, (int)XML_NO_ATTRIBUTE );
	}

	{
		const wchar_t* str = L"<doc/>";

		XMLDocument doc;
		doc.Parse( str );

		XMLElement* ele = doc.FirstChildElement();

		int iVal, iVal2;
		double dVal, dVal2;

		ele->SetAttribute( L"str", L"strValue" );
		ele->SetAttribute( L"int", 1 );
		ele->SetAttribute( L"double", -1.0 );

		const wchar_t* cStr = ele->Attribute( L"str" );
		ele->QueryIntAttribute( L"int", &iVal );
		ele->QueryDoubleAttribute( L"double", &dVal );

		ele->QueryAttribute( L"int", &iVal2 );
		ele->QueryAttribute( L"double", &dVal2 );

		XMLTest( L"Attribute match test", ele->Attribute( L"str", L"strValue" ), L"strValue" );
		XMLTest( L"Attribute round trip. c-string.", L"strValue", cStr );
		XMLTest( L"Attribute round trip. int.", 1, iVal );
		XMLTest( L"Attribute round trip. double.", -1, (int)dVal );
		XMLTest( L"Alternate query", true, iVal == iVal2 );
		XMLTest( L"Alternate query", true, dVal == dVal2 );
	}

	{
		XMLDocumentA doc;
		doc.LoadFile( "resources/Unicode/utf8test.xml" );

		// Get the attribute "value" from the "Russian" element and check it.
		XMLElementA* element = doc.FirstChildElement( "document" )->FirstChildElement( "Russian" );
		const unsigned char correctValue[] = {	0xd1U, 0x86U, 0xd0U, 0xb5U, 0xd0U, 0xbdU, 0xd0U, 0xbdU,
												0xd0U, 0xbeU, 0xd1U, 0x81U, 0xd1U, 0x82U, 0xd1U, 0x8cU, 0 };

		XMLTest( L"UTF-8: Russian value.", (const char*)correctValue, element->Attribute( "value" ) );

		const  char russianElementName[] = {	0xd0U, 0xa0U, 0xd1U, 0x83U,
														0xd1U, 0x81U, 0xd1U, 0x81U,
														0xd0U, 0xbaU, 0xd0U, 0xb8U,
														0xd0U, 0xb9U, 0 };
		const char russianText[] = "<\xD0\xB8\xD0\xBC\xD0\xB5\xD0\xB5\xD1\x82>";

		XMLTextA* text = doc.FirstChildElement( "document" )->FirstChildElement( (const char*) russianElementName )->FirstChild()->ToText();
		XMLTest( L"UTF-8: Browsing russian element name.",
				 russianText,
				 text->Value() );

		// Now try for a round trip.
		doc.SaveFile( "resources/Unicode/out/utf8testout.xml" );

		// Check the round trip.
		int okay = 0;

		FILE* saved  = fopen( "resources/Unicode/out/utf8testout.xml", "r" );
		FILE* verify = fopen( "resources/Unicode/utf8testverify.xml", "r" );

		if ( saved && verify )
		{
			okay = 1;
			char verifyBuf[256];
			while ( fgets( verifyBuf, 256, verify ) )
			{
				char savedBuf[256];
				fgets( savedBuf, 256, saved );
				NullLineEndings( verifyBuf );
				NullLineEndings( savedBuf );

				if ( strcmp( verifyBuf, savedBuf ) )
				{
					printf( "verify:%s<\n", verifyBuf );
					printf( "saved :%s<\n", savedBuf );
					okay = 0;
					break;
				}
			}
		}
		if ( saved )
			fclose( saved );
		if ( verify )
			fclose( verify );
		XMLTest( L"UTF-8: Verified multi-language round trip.", 1, okay );
	}

	// --------GetText()-----------
	{
		const wchar_t* str = L"<foo>This is  text</foo>";
		XMLDocument doc;
		doc.Parse( str );
		const XMLElement* element = doc.RootElement();

		XMLTest( L"GetText() normal use.", L"This is  text", element->GetText() );

		str = L"<foo><b>This is text</b></foo>";
		doc.Parse( str );
		element = doc.RootElement();

		XMLTest( L"GetText() contained element.", element->GetText() == 0, true );
	}


	// --------SetText()-----------
	{
		const wchar_t* str = L"<foo></foo>";
		XMLDocument doc;
		doc.Parse( str );
		XMLElement* element = doc.RootElement();

		element->SetText(L"darkness.");
		XMLTest( L"SetText() normal use (open/close).", L"darkness.", element->GetText() );

		element->SetText(L"blue flame.");
		XMLTest( L"SetText() replace.", L"blue flame.", element->GetText() );

		str = L"<foo/>";
		doc.Parse( str );
		element = doc.RootElement();

		element->SetText(L"The driver");
		XMLTest( L"SetText() normal use. (self-closing)", L"The driver", element->GetText() );

		element->SetText(L"<b>horses</b>");
		XMLTest( L"SetText() replace with tag-like text.", L"<b>horses</b>", element->GetText() );
		//doc.Print();

		str = L"<foo><bar>Text in nested element</bar></foo>";
		doc.Parse( str );
		element = doc.RootElement();
		
		element->SetText(L"wolves");
		XMLTest( L"SetText() prefix to nested non-text children.", L"wolves", element->GetText() );

		str = L"<foo/>";
		doc.Parse( str );
		element = doc.RootElement();
		
		element->SetText( L"str" );
		XMLTest( L"SetText types", L"str", element->GetText() );

		element->SetText( 1 );
		XMLTest( L"SetText types", L"1", element->GetText() );

		element->SetText( 1U );
		XMLTest( L"SetText types", L"1", element->GetText() );

		element->SetText( true );
		XMLTest( L"SetText types", L"1", element->GetText() ); // TODO: should be 'true'?

		element->SetText( 1.5f );
		XMLTest( L"SetText types", L"1.5", element->GetText() );

		element->SetText( 1.5 );
		XMLTest( L"SetText types", L"1.5", element->GetText() );
	}


	// ---------- CDATA ---------------
	{
		const wchar_t* str =	L"<xmlElement>"
								L"<![CDATA["
									L"I am > the rules!\n"
									L"...since I make symbolic puns"
								L"]]>"
							L"</xmlElement>";
		XMLDocument doc;
		doc.Parse( str );
		doc.Print();

		XMLTest( L"CDATA parse.", doc.FirstChildElement()->FirstChild()->Value(),
								 L"I am > the rules!\n...since I make symbolic puns",
								 false );
	}

	// ----------- CDATA -------------
	{
		const wchar_t* str =	L"<xmlElement>"
								L"<![CDATA["
									L"<b>I am > the rules!</b>\n"
									L"...since I make symbolic puns"
								L"]]>"
							L"</xmlElement>";
		XMLDocument doc;
		doc.Parse( str );
		doc.Print();

		XMLTest( L"CDATA parse. [ tixml1:1480107 ]", doc.FirstChildElement()->FirstChild()->Value(),
								 L"<b>I am > the rules!</b>\n...since I make symbolic puns",
								 false );
	}

	// InsertAfterChild causes crash.
	{
		// InsertBeforeChild and InsertAfterChild causes crash.
		XMLDocument doc;
		XMLElement* parent = doc.NewElement( L"Parent" );
		doc.InsertFirstChild( parent );

		XMLElement* childText0 = doc.NewElement( L"childText0" );
		XMLElement* childText1 = doc.NewElement( L"childText1" );

		XMLNode* childNode0 = parent->InsertEndChild( childText0 );
		XMLNode* childNode1 = parent->InsertAfterChild( childNode0, childText1 );

		XMLTest( L"Test InsertAfterChild on empty node. ", ( childNode1 == parent->LastChild() ), true );
	}

	{
		// Entities not being written correctly.
		// From Lynn Allen

		const wchar_t* passages =
			L"<?xml version=\"1.0\" standalone=\"no\" ?>"
			L"<passages count=\"006\" formatversion=\"20020620\">"
				L"<psg context=\"Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;."
				L" It also has &lt;, &gt;, and &amp;, as well as a fake copyright &#xA9;.\"> </psg>"
			L"</passages>";

		XMLDocument doc;
		doc.Parse( passages );
		XMLElement* psg = doc.RootElement()->FirstChildElement();
		const wchar_t* context = psg->Attribute( L"context" );
		const wchar_t* expected = L"Line 5 has \"quotation marks\" and 'apostrophe marks'. It also has <, >, and &, as well as a fake copyright \xC2\xA9.";

		XMLTest( L"Entity transformation: read. ", expected, context, true );

		FILE* textfile = _wfopen( L"resources/Unicode/out/textfile.txt", L"w, ccs=UNICODE" );
		if ( textfile )
		{
			XMLPrinter streamer( textfile );
			psg->Accept( &streamer );
			fclose( textfile );
		}

        textfile = fopen( "resources/Unicode/out/textfile.txt", "r, ccs=UNICODE" );
		TIXMLASSERT( textfile );
		if ( textfile )
		{
			wchar_t buf[ 1024 ];
			fgetws( buf, 1024, textfile );
			XMLTest( L"Entity transformation: write. ",
					 L"<psg context=\"Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;."
					 L" It also has &lt;, &gt;, and &amp;, as well as a fake copyright \xC2\xA9.\"/>\n",
					 buf, false );
			fclose( textfile );
		}
	}

	{
		// Suppress entities.
		const wchar_t* passages =
			L"<?xml version=\"1.0\" standalone=\"no\" ?>"
			L"<passages count=\"006\" formatversion=\"20020620\">"
				L"<psg context=\"Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;.\">Crazy &ttk;</psg>"
			L"</passages>";

		XMLDocument doc( false );
		doc.Parse( passages );

		XMLTest( L"No entity parsing.", doc.FirstChildElement()->FirstChildElement()->Attribute( L"context" ),
				 L"Line 5 has &quot;quotation marks&quot; and &apos;apostrophe marks&apos;." );
		XMLTest( L"No entity parsing.", doc.FirstChildElement()->FirstChildElement()->FirstChild()->Value(),
				 L"Crazy &ttk;" );
		doc.Print();
	}

	{
		const wchar_t* test = L"<?xml version='1.0'?><a.elem xmi.version='2.0'/>";

		XMLDocument doc;
		doc.Parse( test );
		XMLTest( L"dot in names", doc.Error(), false );
		XMLTest( L"dot in names", doc.FirstChildElement()->Name(), L"a.elem" );
		XMLTest( L"dot in names", doc.FirstChildElement()->Attribute( L"xmi.version" ), L"2.0" );
	}

	{
		const wchar_t* test = L"<element><Name>1.1 Start easy ignore fin thickness&#xA;</Name></element>";

		XMLDocument doc;
		doc.Parse( test );

		XMLText* text = doc.FirstChildElement()->FirstChildElement()->FirstChild()->ToText();
		XMLTest( L"Entity with one digit.",
				 text->Value(), L"1.1 Start easy ignore fin thickness\n",
				 false );
	}

	{
		// DOCTYPE not preserved (950171)
		//
		const wchar_t* doctype =
			L"<?xml version=\"1.0\" ?>"
			L"<!DOCTYPE PLAY SYSTEM 'play.dtd'>"
			L"<!ELEMENT title (#PCDATA)>"
			L"<!ELEMENT books (title,authors)>"
			L"<element />";

		XMLDocument doc;
		doc.Parse( doctype );
		doc.SaveFile( L"resources/Unicode/out/test7.xml" );
		doc.DeleteChild( doc.RootElement() );
		doc.LoadFile( L"resources/Unicode/out/test7.xml" );
		doc.Print();

		const XMLUnknown* decl = doc.FirstChild()->NextSibling()->ToUnknown();
		XMLTest( L"Correct value of unknown.", L"DOCTYPE PLAY SYSTEM 'play.dtd'", decl->Value() );

	}

	{
		// Comments do not stream out correctly.
		const wchar_t* doctype =
			L"<!-- Somewhat<evil> -->";
		XMLDocument doc;
		doc.Parse( doctype );

		XMLComment* comment = doc.FirstChild()->ToComment();

		XMLTest( L"Comment formatting.", L" Somewhat<evil> ", comment->Value() );
	}
	{
		// Double attributes
		const wchar_t* doctype = L"<element attr='red' attr='blue' />";

		XMLDocument doc;
		doc.Parse( doctype );

		XMLTest( L"Parsing repeated attributes.", XML_ERROR_PARSING_ATTRIBUTE, doc.ErrorID() );	// is an  error to tinyxml (didn't use to be, but caused issues)
		doc.PrintError();
	}

	{
		// Embedded null in stream.
		const wchar_t* doctype = L"<element att\0r='red' attr='blue' />";

		XMLDocument doc;
		doc.Parse( doctype );
		XMLTest( L"Embedded null throws error.", true, doc.Error() );
	}

	{
		// Empty documents should return TIXML_XML_ERROR_PARSING_EMPTY, bug 1070717
		const wchar_t* str = L"";
		XMLDocument doc;
		doc.Parse( str );
		XMLTest( L"Empty document error", XML_ERROR_EMPTY_DOCUMENT, doc.ErrorID() );
	}

	{
		// Documents with all whitespaces should return TIXML_XML_ERROR_PARSING_EMPTY, bug 1070717
		const wchar_t* str = L"    ";
		XMLDocument doc;
		doc.Parse( str );
		XMLTest( L"All whitespaces document error", XML_ERROR_EMPTY_DOCUMENT, doc.ErrorID() );
	}

	{
		// Low entities
		XMLDocument doc;
		doc.Parse( L"<test>&#x0e;</test>" );
		const wchar_t result[] = { 0x0e, 0 };
		XMLTest( L"Low entities.", doc.FirstChildElement()->GetText(), result );
		doc.Print();
	}

	{
		// Attribute values with trailing quotes not handled correctly
		XMLDocument doc;
		doc.Parse( L"<foo attribute=bar\" />" );
		XMLTest( L"Throw error with bad end quotes.", doc.Error(), true );
	}

	{
		// [ 1663758 ] Failure to report error on bad XML
		XMLDocument xml;
		xml.Parse(L"<x>");
		XMLTest(L"Missing end tag at end of input", xml.Error(), true);
		xml.Parse(L"<x> ");
		XMLTest(L"Missing end tag with trailing whitespace", xml.Error(), true);
		xml.Parse(L"<x></y>");
		XMLTest(L"Mismatched tags", xml.ErrorID(), XML_ERROR_MISMATCHED_ELEMENT);
	}


	{
		// [ 1475201 ] TinyXML parses entities in comments
		XMLDocument xml;
		xml.Parse(L"<!-- declarations for <head> & <body> -->"
				  L"<!-- far &amp; away -->" );

		XMLNode* e0 = xml.FirstChild();
		XMLNode* e1 = e0->NextSibling();
		XMLComment* c0 = e0->ToComment();
		XMLComment* c1 = e1->ToComment();

		XMLTest( L"Comments ignore entities.", L" declarations for <head> & <body> ", c0->Value(), true );
		XMLTest( L"Comments ignore entities.", L" far &amp; away ", c1->Value(), true );
	}

	{
		XMLDocument xml;
		xml.Parse( L"<Parent>"
						L"<child1 att=''/>"
						L"<!-- With this comment, child2 will not be parsed! -->"
						L"<child2 att=''/>"
					L"</Parent>" );
		xml.Print();

		int count = 0;

		for( XMLNode* ele = xml.FirstChildElement( L"Parent" )->FirstChild();
			 ele;
			 ele = ele->NextSibling() )
		{
			++count;
		}

		XMLTest( L"Comments iterate correctly.", 3, count );
	}

	{
		// trying to repro ]1874301]. If it doesn't go into an infinite loop, all is well.
		wchar_t buf[] = L"<?xml version=\"1.0\" encoding=\"utf-8\"?><feed><![CDATA[Test XMLblablablalblbl";
		buf[60] = 239;
		buf[61] = 0;

		XMLDocument doc;
		doc.Parse( (const wchar_t*)buf);
	}


	{
		// bug 1827248 Error while parsing a little bit malformed file
		// Actually not malformed - should work.
		XMLDocument xml;
		xml.Parse( L"<attributelist> </attributelist >" );
		XMLTest( L"Handle end tag whitespace", false, xml.Error() );
	}

	{
		// This one must not result in an infinite loop
		XMLDocument xml;
		xml.Parse( L"<infinite>loop" );
		XMLTest( L"Infinite loop test.", true, true );
	}
#endif
	{
		const wchar_t* pub = L"<?xml version='1.0'?> <element><sub/></element> <!--comment--> <!DOCTYPE>";
		XMLDocument doc;
		doc.Parse( pub );

		XMLDocument clone;
		for( const XMLNode* node=doc.FirstChild(); node; node=node->NextSibling() ) {
			XMLNode* copy = node->ShallowClone( &clone );
			clone.InsertEndChild( copy );
		}

		clone.Print();

		int count=0;
		const XMLNode* a=clone.FirstChild();
		const XMLNode* b=doc.FirstChild();
		for( ; a && b; a=a->NextSibling(), b=b->NextSibling() ) {
			++count;
			XMLTest( L"Clone and Equal", true, a->ShallowEqual( b ));
		}
		XMLTest( L"Clone and Equal", 4, count );
	}

	{
		// This shouldn't crash.
		XMLDocument doc;
		if(XML_NO_ERROR != doc.LoadFile( L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" ))
		{
			doc.PrintError();
		}
		XMLTest( L"Error in snprinf handling.", true, doc.Error() );
	}

	{
		// Attribute ordering.
		static const wchar_t* xml = L"<element attrib1=\"1\" attrib2=\"2\" attrib3=\"3\" />";
		XMLDocument doc;
		doc.Parse( xml );
		XMLElement* ele = doc.FirstChildElement();

		const XMLAttribute* a = ele->FirstAttribute();
		XMLTest( L"Attribute order", L"1", a->Value() );
		a = a->Next();
		XMLTest( L"Attribute order", L"2", a->Value() );
		a = a->Next();
		XMLTest( L"Attribute order", L"3", a->Value() );
		XMLTest( L"Attribute order", L"attrib3", a->Name() );

		ele->DeleteAttribute( L"attrib2" );
		a = ele->FirstAttribute();
		XMLTest( L"Attribute order", L"1", a->Value() );
		a = a->Next();
		XMLTest( L"Attribute order", L"3", a->Value() );

		ele->DeleteAttribute( L"attrib1" );
		ele->DeleteAttribute( L"attrib3" );
		XMLTest( L"Attribute order (empty)", false, ele->FirstAttribute() ? true : false );
	}

	{
		// Make sure an attribute with a space in it succeeds.
		static const wchar_t* xml0 = L"<element attribute1= \"Test Attribute\"/>";
		static const wchar_t* xml1 = L"<element attribute1 =\"Test Attribute\"/>";
		static const wchar_t* xml2 = L"<element attribute1 = \"Test Attribute\"/>";
		XMLDocument doc0;
		doc0.Parse( xml0 );
		XMLDocument doc1;
		doc1.Parse( xml1 );
		XMLDocument doc2;
		doc2.Parse( xml2 );

		XMLElement* ele = 0;
		ele = doc0.FirstChildElement();
		XMLTest( L"Attribute with space #1", L"Test Attribute", ele->Attribute( L"attribute1" ) );
		ele = doc1.FirstChildElement();
		XMLTest( L"Attribute with space #2", L"Test Attribute", ele->Attribute( L"attribute1" ) );
		ele = doc2.FirstChildElement();
		XMLTest( L"Attribute with space #3", L"Test Attribute", ele->Attribute( L"attribute1" ) );
	}

	{
		// Make sure we don't go into an infinite loop.
		static const wchar_t* xml = L"<doc><element attribute='attribute'/><element attribute='attribute'/></doc>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLElement* ele0 = doc.FirstChildElement()->FirstChildElement();
		XMLElement* ele1 = ele0->NextSiblingElement();
		bool equal = ele0->ShallowEqual( ele1 );

		XMLTest( L"Infinite loop in shallow equal.", true, equal );
	}

	// -------- Handles ------------
	{
		static const wchar_t* xml = L"<element attrib='bar'><sub>Text</sub></element>";
		XMLDocument doc;
		doc.Parse( xml );

		XMLElement* ele = XMLHandle( doc ).FirstChildElement( L"element" ).FirstChild().ToElement();
		XMLTest( L"Handle, success, mutable", ele->Value(), L"sub" );

		XMLHandle docH( doc );
		ele = docH.FirstChildElement( L"none" ).FirstChildElement( L"element" ).ToElement();
		XMLTest( L"Handle, dne, mutable", false, ele != 0 );
	}

	{
		static const wchar_t* xml = L"<element attrib='bar'><sub>Text</sub></element>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLConstHandle docH( doc );

		const XMLElement* ele = docH.FirstChildElement( L"element" ).FirstChild().ToElement();
		XMLTest( L"Handle, success, const", ele->Value(), L"sub" );

		ele = docH.FirstChildElement( L"none" ).FirstChildElement( L"element" ).ToElement();
		XMLTest( L"Handle, dne, const", false, ele != 0 );
	}
	{
		// Default Declaration & BOM
		XMLDocument doc;
		doc.InsertEndChild( doc.NewDeclaration() );
		doc.SetBOM( true );

		XMLPrinter printer;
		doc.Print( &printer );

		static const wchar_t* result  = L"\xFEFF<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
		XMLTest( L"BOM and default declaration", printer.CStr(), result, false );
		XMLTest( L"CStrSize", printer.CStrSize(), 40, false );
	}
	{
		const wchar_t* xml = L"<ipxml ws='1'><info bla=' /></ipxml>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( L"Ill formed XML", true, doc.Error() );
	}

	// QueryXYZText
	{
		const wchar_t* xml = L"<point> <x>1.2</x> <y>1</y> <z>38</z> <valid>true</valid> </point>";
		XMLDocument doc;
		doc.Parse( xml );

		const XMLElement* pointElement = doc.RootElement();

		int intValue = 0;
		unsigned unsignedValue = 0;
		float floatValue = 0;
		double doubleValue = 0;
		bool boolValue = false;

		pointElement->FirstChildElement( L"y" )->QueryIntText( &intValue );
		pointElement->FirstChildElement( L"y" )->QueryUnsignedText( &unsignedValue );
		pointElement->FirstChildElement( L"x" )->QueryFloatText( &floatValue );
		pointElement->FirstChildElement( L"x" )->QueryDoubleText( &doubleValue );
		pointElement->FirstChildElement( L"valid" )->QueryBoolText( &boolValue );


		XMLTest( L"QueryIntText", intValue, 1,						false );
		XMLTest( L"QueryUnsignedText", unsignedValue, (unsigned)1,	false );
		XMLTest( L"QueryFloatText", floatValue, 1.2f,				false );
		XMLTest( L"QueryDoubleText", doubleValue, 1.2,				false );
		XMLTest( L"QueryBoolText", boolValue, true,					false );
	}

	{
		const wchar_t* xml = L"<element><_sub/><:sub/><sub:sub/><sub-sub/></element>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLTest( L"Non-alpha element lead letter parses.", doc.Error(), false );
	}
    
    {
        const wchar_t* xml = L"<element _attr1=\"foo\" :attr2=\"bar\"></element>";
        XMLDocument doc;
        doc.Parse( xml );
        XMLTest(L"Non-alpha attribute lead character parses.", doc.Error(), false);
    }
    
    {
        const wchar_t* xml = L"<3lement></3lement>";
        XMLDocument doc;
        doc.Parse( xml );
        XMLTest(L"Element names with lead digit fail to parse.", doc.Error(), true);
    }

	{
		const wchar_t* xml = L"<element/>WOA THIS ISN'T GOING TO PARSE";
		XMLDocument doc;
		doc.Parse( xml, 10 );
		XMLTest( L"Set length of incoming data", doc.Error(), false );
	}

    {
        XMLDocument doc;
        XMLTest( L"Document is initially empty", doc.NoChildren(), true );
        doc.Clear();
        XMLTest( L"Empty is empty after Clear()", doc.NoChildren(), true );
        doc.LoadFile( L"resources/Unicode/dream.xml" );
        XMLTest( L"Document has something to Clear()", doc.NoChildren(), false );
        doc.Clear();
        XMLTest( L"Document Clear()'s", doc.NoChildren(), true );
    }
    
	// ----------- Whitespace ------------
	{
		const wchar_t* xml = L"<element>"
							L"<a> This \nis &apos;  text  &apos; </a>"
							L"<b>  This is &apos; text &apos;  \n</b>"
							L"<c>This  is  &apos;  \n\n text &apos;</c>"
						  L"</element>";
		XMLDocument doc( true, COLLAPSE_WHITESPACE );
		doc.Parse( xml );

		const XMLElement* element = doc.FirstChildElement();
		for( const XMLElement* parent = element->FirstChildElement();
			 parent;
			 parent = parent->NextSiblingElement() )
		{
			XMLTest( L"Whitespace collapse", L"This is ' text '", parent->GetText() );
		}
	}

#if 0
	{
		// Passes if assert doesn't fire.
		XMLDocument xmlDoc;

	    xmlDoc.NewDeclaration();
	    xmlDoc.NewComment("Configuration file");

	    XMLElement *root = xmlDoc.NewElement("settings");
	    root->SetAttribute("version", 2);
	}
#endif

	{
		const wchar_t* xml = L"<element>    </element>";
		XMLDocument doc( true, COLLAPSE_WHITESPACE );
		doc.Parse( xml );
		XMLTest( L"Whitespace  all space", true, 0 == doc.FirstChildElement()->FirstChild() );
	}

	{
		// An assert should not fire.
		const wchar_t* xml = L"<element/>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLElement* ele = doc.NewElement( L"unused" );		// This will get cleaned up with the 'doc' going out of scope.
		XMLTest( L"Tracking unused elements", true, ele != 0, false );
	}


	{
		const wchar_t* xml = L"<parent><child>abc</child></parent>";
		XMLDocument doc;
		doc.Parse( xml );
		XMLElement* ele = doc.FirstChildElement( L"parent")->FirstChildElement( L"child");

		XMLPrinter printer;
		ele->Accept( &printer );
		XMLTest( L"Printing of sub-element", L"<child>abc</child>\n", printer.CStr(), false );
	}


	{
		XMLDocument doc;
		XMLError error = doc.LoadFile( L"resources/Unicode/empty.xml" );
		XMLTest( L"Loading an empty file", XML_ERROR_EMPTY_DOCUMENT, error );
		XMLTest( L"Loading an empty file and ErrorName as string", "XML_ERROR_EMPTY_DOCUMENT", doc.ErrorName() );
		doc.PrintError();
	}

	{
        // BOM preservation
        static const wchar_t* xml_bom_preservation  = L"\xFEFF<element/>\n";
        {
			XMLDocument doc;
			XMLTest( L"BOM preservation (parse)", XML_NO_ERROR, doc.Parse( xml_bom_preservation ), false );
            XMLPrinter printer;
            doc.Print( &printer );

            XMLTest( L"BOM preservation (compare)", xml_bom_preservation, printer.CStr(), false, true );
			doc.SaveFile( L"resources/Unicode/bomtest.xml" );
        }
		{
			XMLDocument doc;
			doc.LoadFile( L"resources/Unicode/bomtest.xml" );
			XMLTest( L"BOM preservation (load)", true, doc.HasBOM(), false );

            XMLPrinter printer;
            doc.Print( &printer );
            XMLTest( L"BOM preservation (compare)", xml_bom_preservation, printer.CStr(), false, true );
		}
	}

	{
		// Insertion with Removal
		const wchar_t* xml = L"<?xml version=\"1.0\" ?>"
			L"<root>"
			L"<one>"
			L"<subtree>"
			L"<elem>element 1</elem>text<!-- comment -->"
			L"</subtree>"
			L"</one>"
			L"<two/>"
			L"</root>";
		const wchar_t* xmlInsideTwo = L"<?xml version=\"1.0\" ?>"
			L"<root>"
			L"<one/>"
			L"<two>"
			L"<subtree>"
			L"<elem>element 1</elem>text<!-- comment -->"
			L"</subtree>"
			L"</two>"
			L"</root>";
		const wchar_t* xmlAfterOne = L"<?xml version=\"1.0\" ?>"
			L"<root>"
			L"<one/>"
			L"<subtree>"
			L"<elem>element 1</elem>text<!-- comment -->"
			L"</subtree>"
			L"<two/>"
			L"</root>";
		const wchar_t* xmlAfterTwo = L"<?xml version=\"1.0\" ?>"
			L"<root>"
			L"<one/>"
			L"<two/>"
			L"<subtree>"
			L"<elem>element 1</elem>text<!-- comment -->"
			L"</subtree>"
			L"</root>";

		XMLDocument doc;
		doc.Parse(xml);
		XMLElement* subtree = doc.RootElement()->FirstChildElement(L"one")->FirstChildElement(L"subtree");
		XMLElement* two = doc.RootElement()->FirstChildElement(L"two");
		two->InsertFirstChild(subtree);
		XMLPrinter printer1(0, true);
		doc.Accept(&printer1);
		XMLTest(L"Move node from within <one> to <two>", xmlInsideTwo, printer1.CStr());

		doc.Parse(xml);
		subtree = doc.RootElement()->FirstChildElement(L"one")->FirstChildElement(L"subtree");
		two = doc.RootElement()->FirstChildElement(L"two");
		doc.RootElement()->InsertAfterChild(two, subtree);
		XMLPrinter printer2(0, true);
		doc.Accept(&printer2);
		XMLTest(L"Move node from within <one> after <two>", xmlAfterTwo, printer2.CStr(), false);

		doc.Parse(xml);
		XMLNode* one = doc.RootElement()->FirstChildElement(L"one");
		subtree = one->FirstChildElement(L"subtree");
		doc.RootElement()->InsertAfterChild(one, subtree);
		XMLPrinter printer3(0, true);
		doc.Accept(&printer3);
		XMLTest(L"Move node from within <one> after <one>", xmlAfterOne, printer3.CStr(), false);

		doc.Parse(xml);
		subtree = doc.RootElement()->FirstChildElement(L"one")->FirstChildElement(L"subtree");
		two = doc.RootElement()->FirstChildElement(L"two");
		doc.RootElement()->InsertEndChild(subtree);
		XMLPrinter printer4(0, true);
		doc.Accept(&printer4);
		XMLTest(L"Move node from within <one> after <two>", xmlAfterTwo, printer4.CStr(), false);
	}

	{
		const wchar_t* xml = L"<svg width = \"128\" height = \"128\">"
			L"	<text> </text>"
			L"</svg>";
		XMLDocument doc;
		doc.Parse(xml);
		doc.Print();
	}

	{
		// Test that it doesn't crash.
		const wchar_t* xml = L"<?xml version=\"1.0\"?><root><sample><field0><1</field0><field1>2</field1></sample></root>";
		XMLDocument doc;
		doc.Parse(xml);
		doc.PrintError();
	}

#if 1
		// the question being explored is what kind of print to use: 
		// https://github.com/leethomason/tinyxml2/issues/63
	{
		//const char* xml = "<element attrA='123456789.123456789' attrB='1.001e9' attrC='1.0e-10' attrD='1001000000.000000' attrE='0.1234567890123456789'/>";
		const wchar_t* xml = L"<element/>";
		XMLDocument doc;
		doc.Parse( xml );
		doc.FirstChildElement()->SetAttribute( L"attrA-f64", 123456789.123456789 );
		doc.FirstChildElement()->SetAttribute( L"attrB-f64", 1.001e9 );
		doc.FirstChildElement()->SetAttribute( L"attrC-f64", 1.0e9 );
		doc.FirstChildElement()->SetAttribute( L"attrC-f64", 1.0e20 );
		doc.FirstChildElement()->SetAttribute( L"attrD-f64", 1.0e-10 );
		doc.FirstChildElement()->SetAttribute( L"attrD-f64", 0.123456789 );

		doc.FirstChildElement()->SetAttribute( L"attrA-f32", 123456789.123456789f );
		doc.FirstChildElement()->SetAttribute( L"attrB-f32", 1.001e9f );
		doc.FirstChildElement()->SetAttribute( L"attrC-f32", 1.0e9f );
		doc.FirstChildElement()->SetAttribute( L"attrC-f32", 1.0e20f );
		doc.FirstChildElement()->SetAttribute( L"attrD-f32", 1.0e-10f );
		doc.FirstChildElement()->SetAttribute( L"attrD-f32", 0.123456789f );

		doc.Print();

		/* The result of this test is platform, compiler, and library version dependent. :("
		XMLPrinter printer;
		doc.Print( &printer );
		XMLTest( "Float and double formatting.", 
			"<element attrA-f64=\"123456789.12345679\" attrB-f64=\"1001000000\" attrC-f64=\"1e+20\" attrD-f64=\"0.123456789\" attrA-f32=\"1.2345679e+08\" attrB-f32=\"1.001e+09\" attrC-f32=\"1e+20\" attrD-f32=\"0.12345679\"/>\n",
			printer.CStr(), 
			true );
		*/
	}
#endif
    
    {
        // Issue #184
        // If it doesn't assert, it passes. Caused by objects
        // getting created during parsing which are then
        // inaccessible in the memory pools.
        {
            XMLDocument doc;
            doc.Parse(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?><test>");
        }
        {
            XMLDocument doc;
            doc.Parse(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?><test>");
            doc.Clear();
        }
    }
    
    {
        // If this doesn't assert in DEBUG, all is well.
        tinyxml2::XMLDocument doc;
        tinyxml2::XMLElement *pRoot = doc.NewElement(L"Root");
        doc.DeleteNode(pRoot);
    }

	{
		// Should not assert in DEBUG
		XMLPrinter printer;
	}

	{
		// Issue 291. Should not crash
		const wchar_t* xml = L"&#0</a>";
		XMLDocument doc;
		doc.Parse( xml );

		XMLPrinter printer;
		doc.Print( &printer );
	}
	{
		// Issue 299. Can print elements that are not linked in. 
		// Will crash if issue not fixed.
		XMLDocument doc;
		XMLElement* newElement = doc.NewElement( L"printme" );
		XMLPrinter printer;
		newElement->Accept( &printer );
		// Delete the node to avoid possible memory leak report in debug output
		doc.DeleteNode( newElement );
	}
	{
		// Issue 302. Clear errors from LoadFile/SaveFile
		XMLDocument doc;
		XMLTest( L"Issue 302. Should be no error initially", "XML_SUCCESS", doc.ErrorName() );
		doc.SaveFile( L"./no/such/path/pretty.xml" );
		XMLTest( L"Issue 302. Fail to save", "XML_ERROR_FILE_COULD_NOT_BE_OPENED", doc.ErrorName() );
		doc.SaveFile( L"./resources/Unicode/out/compact.xml", true );
		XMLTest( L"Issue 302. Subsequent success in saving", "XML_SUCCESS", doc.ErrorName() );
	}

	{
		// If a document fails to load then subsequent
		// successful loads should clear the error
		XMLDocument doc;
		XMLTest( L"Should be no error initially", false, doc.Error() );
		doc.LoadFile( L"resources/Unicode/no-such-file.xml" );
		XMLTest( L"No such file - should fail", true, doc.Error() );

		doc.LoadFile( L"resources/Unicode/dream.xml" );
		XMLTest( L"Error should be cleared", false, doc.Error() );
	}

	{
		// Check that declarations are parsed only as the FirstChild
	    const wchar_t* xml0 = L"<?xml version=\"1.0\" ?>"
	                       L"   <!-- xml version=\"1.1\" -->"
	                       L"<first />";
	    const wchar_t* xml1 = L"<?xml version=\"1.0\" ?>"
	                       L"   <?xml version=\"1.1\" ?>"
	                       L"<first />";
	    const wchar_t* xml2 = L"<first />"
	                       L"<?xml version=\"1.0\" ?>";
	    XMLDocument doc;
	    doc.Parse(xml0);
	    XMLTest(L"Test that the code changes do not affect normal parsing", doc.Error(), false);
	    doc.Parse(xml1);
	    XMLTest(L"Test that the second declaration throws an error", doc.ErrorID(), XML_ERROR_PARSING_DECLARATION);
	    doc.Parse(xml2);
	    XMLTest(L"Test that declaration after a child throws an error", doc.ErrorID(), XML_ERROR_PARSING_DECLARATION);
	}

    {
	    // No matter - before or after successfully parsing a text -
	    // calling XMLDocument::Value() causes an assert in debug.
	    const wchar_t* validXml = L"<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
	                           L"<first />"
	                           L"<second />";
	    XMLDocument* doc = new XMLDocument();
	    XMLTest( L"XMLDocument::Value() returns null?", (wchar_t*)NULL, doc->Value() );
	    doc->Parse( validXml );
	    XMLTest( L"XMLDocument::Value() returns null?", (wchar_t*)NULL, doc->Value() );
	    delete doc;
    }

	{
		XMLDocument doc;
		for( int i = 0; i < XML_ERROR_COUNT; i++ ) {
			doc.SetError( (XMLError)i, 0, 0 );
			doc.ErrorName();
		}
	}

    // ----------- Performance tracking --------------
	{
#if defined( _MSC_VER )
		__int64 start, end, freq;
		QueryPerformanceFrequency( (LARGE_INTEGER*) &freq );
#endif

		FILE* fp  = fopen( "resources/Unicode/dream.xml", "r" );
		fseek( fp, 0, SEEK_END );
		long size = ftell( fp );
		fseek( fp, 0, SEEK_SET );

		wchar_t* mem = new wchar_t[size+1];
		fread( mem, size, 1, fp );
		fclose( fp );
		mem[size] = 0;

#if defined( _MSC_VER )
		QueryPerformanceCounter( (LARGE_INTEGER*) &start );
#else
		clock_t cstart = clock();
#endif
		static const int COUNT = 10;
		for( int i=0; i<COUNT; ++i ) {
			XMLDocument doc;
			doc.Parse( mem );
		}
#if defined( _MSC_VER )
		QueryPerformanceCounter( (LARGE_INTEGER*) &end );
#else
		clock_t cend = clock();
#endif

		delete [] mem;

		static const char* note =
#ifdef DEBUG
			"DEBUG";
#else
			"Release";
#endif

#if defined( _MSC_VER )
		printf( "\nParsing %s of dream.xml: %.3f milli-seconds\n", note, 1000.0 * (double)(end-start) / ( (double)freq * (double)COUNT) );
#else
		printf( "\nParsing %s of dream.xml: %.3f milli-seconds\n", note, (double)(cend - cstart)/(double)COUNT );
#endif
	}

	#if defined( _MSC_VER ) &&  defined( DEBUG )
		_CrtMemCheckpoint( &endMemState );

		_CrtMemState diffMemState;
		_CrtMemDifference( &diffMemState, &startMemState, &endMemState );
		_CrtMemDumpStatistics( &diffMemState );
	#endif

	printf ("\nPass %d, Fail %d\n", gPass, gFail);

	return gFail;
}
