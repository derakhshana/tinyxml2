/*
Original code by Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include "tinyxml2.h"

#include <new>		// yes, this one new style header, is in the Android SDK.
#if defined(ANDROID_NDK) || defined(__QNXNTO__)
#   include <stddef.h>
#   include <stdarg.h>
#else
#   include <cstddef>
#   include <cstdarg>
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1400 ) && (!defined WINCE)
	// Microsoft Visual Studio, version 2005 and higher. Not WinCE.
	/*int _snprintf_s(
	   char *buffer,
	   size_t sizeOfBuffer,
	   size_t count,
	   const char *format [,
		  argument] ...
	);*/
	static int TIXML_SNPRINTF( char* buffer, size_t size, const char* format, ... )
	{
		va_list va;
		va_start( va, format );
		int result = vsnprintf_s( buffer, size, _TRUNCATE, format, va );
		va_end( va );
		return result;
	}

	static int TIXML_SNWPRINTF( wchar_t* buffer, size_t size, const wchar_t* format, ... )
	{
		va_list va;
		va_start( va, format );
		int result = _vsnwprintf_s( buffer, size, _TRUNCATE, format, va );
		va_end( va );
		return result;
	}

	static int TIXML_VSNPRINTF( char* buffer, size_t size, const char* format, va_list va )
	{
		int result = vsnprintf_s( buffer, size, _TRUNCATE, format, va );
		return result;
	}
	
	static int TIXML_VSNWPRINTF( wchar_t* buffer, size_t size, const wchar_t* format, va_list va )
	{
		int result = _vsnwprintf_s( buffer, size, _TRUNCATE, format, va );
		return result;
	}

	#define TIXML_VSCPRINTF	_vscprintf
	#define TIXML_VSCWPRINTF	_vscwprintf
	//#define TIXML_SSCANF	sscanf_s
	int TIXML_SSCANF( const char * _Src,  const char * _Format, ...)
	{
		va_list argptr;
		va_start(argptr, _Format);
		va_list argptr2 = reinterpret_cast<char* &>(*argptr);
		return sscanf_s( _Src, _Format, argptr2);
	}

	int TIXML_SWSCANF( const wchar_t * _Src,  const wchar_t * _Format, ... )
	{
		va_list argptr;
		va_start(argptr, _Format);
		va_list argptr2 = reinterpret_cast<char* &>(*argptr);
		return swscanf_s( _Src, _Format, argptr2 );
	}

#elif defined _MSC_VER
	// Microsoft Visual Studio 2003 and earlier or WinCE
	#define TIXML_SNPRINTF	_snprintf
	#define TIXML_SNWPRINTF	_snwprintf
	#define TIXML_VSNPRINTF _vsnprintf
	#define TIXML_VSNWPRINTF _vsnwprintf
	#define TIXML_SSCANF	sscanf
	#define TIXML_SWSCANF	swscanf
	#if (_MSC_VER < 1400 ) && (!defined WINCE)
		// Microsoft Visual Studio 2003 and not WinCE.
		#define TIXML_VSCPRINTF   _vscprintf // VS2003's C runtime has this, but VC6 C runtime or WinCE SDK doesn't have.
		#define TIXML_VSCWPRINTF   _vscwprintf
	#else
		// Microsoft Visual Studio 2003 and earlier or WinCE.
		static int TIXML_VSCPRINTF( const char* format, va_list va )
		{
			int len = 512;
			for (;;) {
				len = len*2;
				char* str = new char[len]();
				const int required = _vsnprintf(str, len, format, va);
				delete[] str;
				if ( required != -1 ) {
					TIXMLASSERT( required >= 0 );
					len = required;
					break;
				}
			}
			TIXMLASSERT( len >= 0 );
			return len;
		}
		static int TIXML_VSCWPRINTF( const wchar_t* format, va_list va )
		{
			int len = 512;
			for (;;) {
				len = len*2;
				wchar_t* str = new wchar_t[len]();
				const int required = _vsnwprintf(str, len, format, va);
				delete[] str;
				if ( required != -1 ) {
					TIXMLASSERT( required >= 0 );
					len = required;
					break;
				}
			}
			TIXMLASSERT( len >= 0 );
			return len;
		}
	#endif
#else
	// GCC version 3 and higher
	//#warning( "Using sn* functions." )
	#define TIXML_SNPRINTF	snprintf
	#define TIXML_SNWPRINTF	snwprintf
	#define TIXML_VSNPRINTF	vsnprintf
	#define TIXML_VSNWPRINTF	vsnwprintf
	static int TIXML_VSCPRINTF( const char* format, va_list va )
	{
		int len = vsnprintf( 0, 0, format, va );
		TIXMLASSERT( len >= 0 );
		return len;
	}
	static int TIXML_VSCWPRINTF( const wchar_t* format, va_list va )
	{
		int len = vsnwprintf( 0, 0, format, va );
		TIXMLASSERT( len >= 0 );
		return len;
	}
	#define TIXML_SSCANF   sscanf
	#define TIXML_SWSCANF   swscanf
#endif


static const char LINE_FEED				= (char)0x0a;			// all line endings are normalized to LF
static const char LF = LINE_FEED;
static const char CARRIAGE_RETURN		= (char)0x0d;			// CR gets filtered out
static const char CR = CARRIAGE_RETURN;
static const char SINGLE_QUOTE			= '\'';
static const char DOUBLE_QUOTE			= '\"';

// Bunch of unicode info at:
//		http://www.unicode.org/faq/utf_bom.html
//	ef bb bf (Microsoft "lead bytes") - designates UTF-8

static const unsigned char TIXML_UTF_LEAD_0 = 0xefU;
static const unsigned char TIXML_UTF_LEAD_1 = 0xbbU;
static const unsigned char TIXML_UTF_LEAD_2 = 0xbfU;

static const unsigned char TIXML_UNICODE_LEAD_0 = 0xffU;
static const unsigned char TIXML_UNICODE_LEAD_1 = 0xfeU;

namespace tinyxml2
{
template<typename xchar>
inline std::size_t strlen(const xchar *pStr)
{
    const xchar *tmp = pStr;
    while (*tmp) 
        ++tmp;
    return tmp - pStr;
}


template<typename xchar1, typename xchar2>
inline int strncmp(const xchar1 *pStr1, const xchar2 *pStr2, std::size_t sizeMax)
{
	for (const xchar1 *bgn = pStr1; pStr1 < bgn + sizeMax; ++pStr1, ++pStr2)
    {
		if (*pStr1 < *pStr2)
            return -1;
		else if (*pStr1 > *pStr2)
			return 1;
	}
    return 0;
}


template<typename xchar>
inline const xchar* strchr(const xchar *pStr, xchar xVal)
{
	for (const xchar *q = pStr; *q; q++)
		if (*q == xVal)
			return q;
	return 0;
}


struct Entity {
    const char* pattern;
    int length;
    char value;
};

static const int NUM_ENTITIES = 5;
static const Entity entities[NUM_ENTITIES] = {
    { "quot", 4,	DOUBLE_QUOTE },
    { "amp", 3,		'&'  },
    { "apos", 4,	SINGLE_QUOTE },
    { "lt",	2, 		'<'	 },
    { "gt",	2,		'>'	 }
};

template<typename xchar>
StrPairT<xchar>::~StrPairT()
{
    Reset();
}

template<typename xchar>
void StrPairT<xchar>::TransferTo( StrPairT<xchar>* other )
{
    if ( this == other ) {
        return;
    }
    // This in effect implements the assignment operator by "moving"
    // ownership (as in auto_ptr).

    TIXMLASSERT( other->_flags == 0 );
    TIXMLASSERT( other->_start == 0 );
    TIXMLASSERT( other->_end == 0 );

    other->Reset();

    other->_flags = _flags;
    other->_start = _start;
    other->_end = _end;

    _flags = 0;
    _start = 0;
    _end = 0;
}

template<typename xchar>
void StrPairT<xchar>::Reset()
{
    if ( _flags & NEEDS_DELETE ) {
        delete [] _start;
    }
    _flags = 0;
    _start = 0;
    _end = 0;
}

template<typename xchar>
inline void StrPairT<xchar>::Set( xchar* start, xchar* end, int flags )
{
    Reset();
    _start  = start;
    _end    = end;
    _flags  = flags | NEEDS_FLUSH;
}

template<typename xchar>
inline void StrPairT<xchar>::SetInternedStr( const xchar* str )
{
    Reset();
    _start = const_cast<xchar*>(str);
}

template<typename xchar>
void StrPairT<xchar>::SetStr( const xchar* str, int flags )
{
    Reset();
    size_t len = strlen( str );
    TIXMLASSERT( _start == 0 );
    _start = new xchar[ len+1 ];
    memcpy( _start, str, ( len+1 ) * sizeof(xchar));
    _end = _start + len;
    _flags = flags | NEEDS_DELETE;
}

template<typename xchar>
xchar* StrPairT<xchar>::ParseText( xchar* p, const xchar* endTag, int strFlags )
{
    TIXMLASSERT( endTag && *endTag );

    xchar* start = p;
    xchar  endChar = *endTag;
    size_t length = strlen( endTag );

    // Inner loop of text parsing.
    while ( *p ) {
        if ( *p == endChar && strncmp( p, endTag, length ) == 0 ) {
            Set( start, p, strFlags );
            return p + length;
        }
        ++p;
    }
    return 0;
}

template<typename xchar>
xchar* StrPairT<xchar>::ParseName( xchar* p )
{
    if ( !p || !(*p) ) {
        return 0;
    }
    if ( !XMLUtilT<xchar>::IsNameStartChar( *p ) ) {
        return 0;
    }

    xchar* const start = p;
    ++p;
    while ( *p && XMLUtilT<xchar>::IsNameChar( *p ) ) {
        ++p;
    }

    Set( start, p, 0 );
    return p;
}

template<typename xchar>
void StrPairT<xchar>::CollapseWhitespace()
{
    // Adjusting _start would cause undefined behavior on delete[]
    TIXMLASSERT( ( _flags & NEEDS_DELETE ) == 0 );
    // Trim leading space.
    _start = XMLUtilT<xchar>::SkipWhiteSpace( _start );

    if ( *_start ) {
        xchar* p = _start;	// the read pointer
        xchar* q = _start;	// the write pointer

        while( *p ) {
            if ( XMLUtilT<xchar>::IsWhiteSpace( *p )) {
                p = XMLUtilT<xchar>::SkipWhiteSpace( p );
                if ( *p == 0 ) {
                    break;    // don't write to q; this trims the trailing space.
                }
                *q = xchar(' ');
                ++q;
            }
            *q = *p;
            ++q;
            ++p;
        }
        *q = 0;
    }
}

template<typename xchar>
const xchar* StrPairT<xchar>::GetStr()
{
    TIXMLASSERT( _start );
    TIXMLASSERT( _end );
    if ( _flags & NEEDS_FLUSH ) {
        *_end = 0;
        _flags ^= NEEDS_FLUSH;

        if ( _flags ) {
            xchar* p = _start;	// the read pointer
            xchar* q = _start;	// the write pointer

            while( p < _end ) {
                if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == CR ) {
                    // CR-LF pair becomes LF
                    // CR alone becomes LF
                    // LF-CR becomes LF
                    if ( *(p+1) == LF ) {
                        p += 2;
                    }
                    else {
                        ++p;
                    }
                    *q++ = LF;
                }
                else if ( (_flags & NEEDS_NEWLINE_NORMALIZATION) && *p == LF ) {
                    if ( *(p+1) == CR ) {
                        p += 2;
                    }
                    else {
                        ++p;
                    }
                    *q++ = LF;
                }
                else if ( (_flags & NEEDS_ENTITY_PROCESSING) && *p == '&' ) {
                    // Entities handled by tinyXML2:
                    // - special entities in the entity table [in/out]
                    // - numeric character reference [in]
                    //   &#20013; or &#x4e2d;

                    if ( *(p+1) == '#' ) {
                        const int buflen = 10;
                        xchar buf[buflen] = { 0 };
                        int len = 0;
                        xchar* adjusted = const_cast<xchar*>( XMLUtilT<xchar>::GetCharacterRef( p, buf, &len ) );
                        if ( adjusted == 0 ) {
                            *q = *p;
                            ++p;
                            ++q;
                        }
                        else {
                            TIXMLASSERT( 0 <= len && len <= buflen );
                            TIXMLASSERT( q + len <= adjusted );
                            p = adjusted;
                            memcpy( q, buf, len * sizeof(xchar));
                            q += len;
                        }
                    }
                    else {
                        bool entityFound = false;
                        for( int i = 0; i < NUM_ENTITIES; ++i ) {
                            const Entity& entity = entities[i];
                            if ( strncmp/*<xchar>*/( p + 1, entity.pattern, entity.length ) == 0
                                    && *( p + entity.length + 1 ) == ';' ) {
                                // Found an entity - convert.
                                *q = entity.value;
                                ++q;
                                p += entity.length + 2;
                                entityFound = true;
                                break;
                            }
                        }
                        if ( !entityFound ) {
                            // fixme: treat as error?
                            ++p;
                            ++q;
                        }
                    }
                }
                else {
                    *q = *p;
                    ++p;
                    ++q;
                }
            }
            *q = 0;
        }
        // The loop below has plenty going on, and this
        // is a less useful mode. Break it out.
        if ( _flags & NEEDS_WHITESPACE_COLLAPSING ) {
            CollapseWhitespace();
        }
        _flags = (_flags & NEEDS_DELETE);
    }
    TIXMLASSERT( _start );
    return _start;
}




// --------- XMLUtil ----------- //
template<typename xchar>
const char* XMLUtilT<xchar>::ReadBOM( const char* p, bool* bom )
{
    TIXMLASSERT( p );
    TIXMLASSERT( bom );
    *bom = false;
    const unsigned char* pu = reinterpret_cast<const unsigned char*>(p);
    // Check for BOM:
    if (    *(pu+0) == TIXML_UTF_LEAD_0
            && *(pu+1) == TIXML_UTF_LEAD_1
            && *(pu+2) == TIXML_UTF_LEAD_2 ) {
        *bom = true;
        p += 3;
    }
	else if (  *(pu+0) == TIXML_UNICODE_LEAD_0
            && *(pu+1) == TIXML_UNICODE_LEAD_1 ) {
        *bom = true;
        p += 2;
    }
    TIXMLASSERT( p );
    return p;
}

template<typename xchar>
void XMLUtilT<xchar>::ConvertUTF32ToUTF8( unsigned long input, xchar* output, int* length )
{
    const unsigned long BYTE_MASK = 0xBF;
    const unsigned long BYTE_MARK = 0x80;
    const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

    if (input < 0x80) {
        *length = 1;
    }
    else if ( input < 0x800 ) {
        *length = 2;
    }
    else if ( input < 0x10000 ) {
        *length = 3;
    }
    else if ( input < 0x200000 ) {
        *length = 4;
    }
    else {
        *length = 0;    // This code won't convert this correctly anyway.
        return;
    }

    output += *length;

    // Scary scary fall throughs.
    switch (*length) {
        case 4:
            --output;
            *output = (xchar)((input | BYTE_MARK) & BYTE_MASK);
            input >>= 6;
        case 3:
            --output;
            *output = (xchar)((input | BYTE_MARK) & BYTE_MASK);
            input >>= 6;
        case 2:
            --output;
            *output = (xchar)((input | BYTE_MARK) & BYTE_MASK);
            input >>= 6;
        case 1:
            --output;
            *output = (xchar)(input | FIRST_BYTE_MARK[*length]);
            break;
        default:
            TIXMLASSERT( false );
    }
}

template<typename xchar>
const xchar* XMLUtilT<xchar>::GetCharacterRef( const xchar* p, xchar* value, int* length )
{
    // Presume an entity, and pull it out.
    *length = 0;

    if ( *(p+1) == xchar('#') && *(p+2) ) {
        unsigned long ucs = 0;
        TIXMLASSERT( sizeof( ucs ) >= 4 );
        ptrdiff_t delta = 0;
        unsigned mult = 1;
        static const xchar SEMICOLON = xchar(';');

        if ( *(p+2) == xchar('x') ) {
            // Hexadecimal.
            const xchar* q = p+3;
            if ( !(*q) ) {
                return 0;
            }

            q = strchr( q, SEMICOLON );

            if ( !q ) {
                return 0;
            }
            TIXMLASSERT( *q == SEMICOLON );

            delta = q-p;
            --q;

            while ( *q != xchar('x') ) {
                unsigned int digit = 0;

                if ( *q >= xchar('0') && *q <= xchar('9') ) {
                    digit = *q - xchar('0');
                }
                else if ( *q >= xchar('a') && *q <= xchar('f') ) {
                    digit = *q - xchar('a') + 10;
                }
                else if ( *q >= xchar('A') && *q <= xchar('F') ) {
                    digit = *q - xchar('A') + 10;
                }
                else {
                    return 0;
                }
                TIXMLASSERT( digit >= 0 && digit < 16);
                TIXMLASSERT( digit == 0 || mult <= UINT_MAX / digit );
                const unsigned int digitScaled = mult * digit;
                TIXMLASSERT( ucs <= ULONG_MAX - digitScaled );
                ucs += digitScaled;
                TIXMLASSERT( mult <= UINT_MAX / 16 );
                mult *= 16;
                --q;
            }
        }
        else {
            // Decimal.
            const xchar* q = p+2;
            if ( !(*q) ) {
                return 0;
            }

            q = strchr( q, SEMICOLON );

            if ( !q ) {
                return 0;
            }
            TIXMLASSERT( *q == SEMICOLON );

            delta = q-p;
            --q;

            while ( *q != xchar('#') ) {
                if ( *q >= xchar('0') && *q <= xchar('9') ) {
                    const unsigned int digit = *q - xchar('0');
                    TIXMLASSERT( digit >= 0 && digit < 10);
                    TIXMLASSERT( digit == 0 || mult <= UINT_MAX / digit );
                    const unsigned int digitScaled = mult * digit;
                    TIXMLASSERT( ucs <= ULONG_MAX - digitScaled );
                    ucs += digitScaled;
                }
                else {
                    return 0;
                }
                TIXMLASSERT( mult <= UINT_MAX / 10 );
                mult *= 10;
                --q;
            }
        }
        // convert the UCS to UTF-8
        ConvertUTF32ToUTF8( ucs, value, length );
        return p + delta + 1;
    }
    return p+1;
}

template< >
void XMLUtilT<char>::ToStr( int v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%d", v );
}

template< >
void XMLUtilT<wchar_t>::ToStr( int v, wchar_t* buffer, int bufferSize )
{
    TIXML_SNWPRINTF( buffer, bufferSize, L"%d", v );
}

template< >
void XMLUtilT<char>::ToStr( unsigned v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%u", v );
}

template< >
void XMLUtilT<wchar_t>::ToStr( unsigned v, wchar_t* buffer, int bufferSize )
{
    TIXML_SNWPRINTF( buffer, bufferSize, L"%u", v );
}

template< >
void XMLUtilT<char>::ToStr( bool v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%d", v ? 1 : 0 );
}

template< >
void XMLUtilT<wchar_t>::ToStr( bool v, wchar_t* buffer, int bufferSize )
{
    TIXML_SNWPRINTF( buffer, bufferSize, L"%d", v ? 1 : 0 );
}

/*
	ToStr() of a number is a very tricky topic.
	https://github.com/leethomason/tinyxml2/issues/106
*/
template< >
void XMLUtilT<char>::ToStr( float v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%.8g", v );
}

template< >
void XMLUtilT<wchar_t>::ToStr( float v, wchar_t* buffer, int bufferSize )
{
    TIXML_SNWPRINTF( buffer, bufferSize, L"%.8g", v );
}

template< >
void XMLUtilT<char>::ToStr( double v, char* buffer, int bufferSize )
{
    TIXML_SNPRINTF( buffer, bufferSize, "%.17g", v );
}

template< >
void XMLUtilT<wchar_t>::ToStr( double v, wchar_t* buffer, int bufferSize )
{
    TIXML_SNWPRINTF( buffer, bufferSize, L"%.17g", v );
}

template< >
bool XMLUtilT<char>::ToInt( const char* str, int* value )
{
    if ( TIXML_SSCANF( str, "%d", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToInt( const wchar_t* str, int* value )
{
    if ( TIXML_SWSCANF( str, L"%d", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToUnsigned( const char* str, unsigned* value )
{
    if ( TIXML_SSCANF( str, "%u", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToUnsigned( const wchar_t* str, unsigned* value )
{
    if ( TIXML_SWSCANF( str, L"%u", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToInt8( const char* str, char* value )
{
    short shortValue = 0;
    if ( TIXML_SSCANF( str, "%hd", &shortValue ) == 1 ) {
        *value = (char) shortValue;
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToInt8( const wchar_t* str, char* value )
{
    short shortValue = 0;
    if ( TIXML_SWSCANF( str, L"%hd", &shortValue ) == 1 ) {
        *value = (char) shortValue;
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToUnsigned8( const char* str, unsigned char* value )
{
    unsigned short shortValue = 0;
    if ( TIXML_SSCANF( str, "%hu", &shortValue ) == 1 ) {
        *value = (unsigned char) shortValue;
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToUnsigned8( const wchar_t* str, unsigned char* value )
{
    unsigned short shortValue = 0;
    if ( TIXML_SWSCANF( str, L"%hu", &shortValue ) == 1 ) {
        *value = (unsigned char) shortValue;
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToInt16( const char* str, short* value )
{
    if ( TIXML_SSCANF( str, "%hd", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToInt16( const wchar_t* str, short* value )
{
    if ( TIXML_SWSCANF( str, L"%hd", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToUnsigned16( const char* str, unsigned short* value )
{
    if ( TIXML_SSCANF( str, "%hu", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToUnsigned16( const wchar_t* str, unsigned short* value )
{
    if ( TIXML_SWSCANF( str, L"%hu", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToInt64( const char* str, long long* value )
{
    if ( TIXML_SSCANF( str, "%lld", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToInt64( const wchar_t* str, long long* value )
{
    if ( TIXML_SWSCANF( str, L"%lld", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToUnsigned64( const char* str, unsigned long long* value )
{
    if ( TIXML_SSCANF( str, "%llu", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToUnsigned64( const wchar_t* str, unsigned long long* value )
{
    if ( TIXML_SWSCANF( str, L"%llu", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToBool( const char* str, bool* value )
{
    int ival = 0;
    if ( ToInt( str, &ival )) {
        *value = (ival==0) ? false : true;
        return true;
    }
    if ( StringEqual( str, "true" ) ) {
        *value = true;
        return true;
    }
    else if ( StringEqual( str, "false" ) ) {
        *value = false;
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToBool( const wchar_t* str, bool* value )
{
    int ival = 0;
    if ( ToInt( str, &ival )) {
        *value = (ival==0) ? false : true;
        return true;
    }
    if ( StringEqual( str, L"true" ) ) {
        *value = true;
        return true;
    }
    else if ( StringEqual( str, L"false" ) ) {
        *value = false;
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToFloat( const char* str, float* value )
{
    if ( TIXML_SSCANF( str, "%f", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToFloat( const wchar_t* str, float* value )
{
    if ( TIXML_SWSCANF( str, L"%f", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<char>::ToDouble( const char* str, double* value )
{
    if ( TIXML_SSCANF( str, "%lf", value ) == 1 ) {
        return true;
    }
    return false;
}

template< >
bool XMLUtilT<wchar_t>::ToDouble( const wchar_t* str, double* value )
{
    if ( TIXML_SWSCANF( str, L"%lf", value ) == 1 ) {
        return true;
    }
    return false;
}


template<typename xchar>
xchar* XMLDocumentT<xchar>::Identify( xchar* p, XMLNodeT<xchar>** node )
{
    TIXMLASSERT( node );
    TIXMLASSERT( p );
    xchar* const start = p;
    p = XMLUtilT<xchar>::SkipWhiteSpace( p );
    if( !*p ) {
        *node = 0;
        TIXMLASSERT( p );
        return p;
    }

    // These strings define the matching patterns:
    static const xchar xmlHeader[]		= { '<', '?', 0 };
    static const xchar commentHeader[]	= { '<', '!', '-', '-', 0 };
    static const xchar cdataHeader[]		= { '<', '!', '[', 'C', 'D', 'A', 'T', 'A', '[', 0 };
    static const xchar dtdHeader[]		= { '<', '!', 0 };
    static const xchar elementHeader[]	= { '<', 0 };	// and a header for everything else; check last.

    static const int xmlHeaderLen		= 2;
    static const int commentHeaderLen	= 4;
    static const int cdataHeaderLen		= 9;
    static const int dtdHeaderLen		= 2;
    static const int elementHeaderLen	= 1;

    TIXMLASSERT( sizeof( XMLComment ) == sizeof( XMLUnknownT<xchar> ) );		// use same memory pool
    TIXMLASSERT( sizeof( XMLComment ) == sizeof( XMLDeclarationT<xchar> ) );	// use same memory pool
    XMLNodeT<xchar>* returnNode = 0;
    if ( XMLUtilT<xchar>::StringEqual( p, xmlHeader, xmlHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLDeclaration ) == _commentPool.ItemSize() );
        returnNode = new (_commentPool.Alloc()) XMLDeclarationT<xchar>( this );
        returnNode->_memPool = &_commentPool;
        p += xmlHeaderLen;
    }
    else if ( XMLUtilT<xchar>::StringEqual( p, commentHeader, commentHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLCommentT<xchar> ) == _commentPool.ItemSize() );
        returnNode = new (_commentPool.Alloc()) XMLCommentT<xchar>( this );
        returnNode->_memPool = &_commentPool;
        p += commentHeaderLen;
    }
    else if ( XMLUtilT<xchar>::StringEqual( p, cdataHeader, cdataHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLTextT<xchar> ) == _textPool.ItemSize() );
        XMLTextT<xchar>* text = new (_textPool.Alloc()) XMLTextT<xchar>( this );
        returnNode = text;
        returnNode->_memPool = &_textPool;
        p += cdataHeaderLen;
        text->SetCData( true );
    }
    else if ( XMLUtilT<xchar>::StringEqual( p, dtdHeader, dtdHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLUnknownT<xchar> ) == _commentPool.ItemSize() );
        returnNode = new (_commentPool.Alloc()) XMLUnknownT<xchar>( this );
        returnNode->_memPool = &_commentPool;
        p += dtdHeaderLen;
    }
    else if ( XMLUtilT<xchar>::StringEqual( p, elementHeader, elementHeaderLen ) ) {
        TIXMLASSERT( sizeof( XMLElementT<xchar> ) == _elementPool.ItemSize() );
        returnNode = new (_elementPool.Alloc()) XMLElementT<xchar>( this );
        returnNode->_memPool = &_elementPool;
        p += elementHeaderLen;
    }
    else {
        TIXMLASSERT( sizeof( XMLTextT<xchar> ) == _textPool.ItemSize() );
        returnNode = new (_textPool.Alloc()) XMLTextT<xchar>( this );
        returnNode->_memPool = &_textPool;
        p = start;	// Back it up, all the text counts.
    }

    TIXMLASSERT( returnNode );
    TIXMLASSERT( p );
    *node = returnNode;
    return p;
}


template<typename xchar>
bool XMLDocumentT<xchar>::Accept( XMLVisitorT<xchar>* visitor ) const
{
    TIXMLASSERT( visitor );
    if ( visitor->VisitEnter( *this ) ) {
        for ( const XMLNodeT<xchar>* node=FirstChild(); node; node=node->NextSibling() ) {
            if ( !node->Accept( visitor ) ) {
                break;
            }
        }
    }
    return visitor->VisitExit( *this );
}


// --------- XMLNode ----------- //
template<typename xchar>
XMLNodeT<xchar>::XMLNodeT( XMLDocumentT<xchar>* doc ) :
    _document( doc ),
    _parent( 0 ),
    _firstChild( 0 ), _lastChild( 0 ),
    _prev( 0 ), _next( 0 ),
    _memPool( 0 )
{
}

template<typename xchar>
XMLNodeT<xchar>::~XMLNodeT()
{
    DeleteChildren();
    if ( _parent ) {
        _parent->Unlink( this );
    }
}

template<typename xchar>
const xchar* XMLNodeT<xchar>::Value() const 
{
    // Catch an edge case: XMLDocuments don't have a a Value. Carefully return nullptr.
    if ( this->ToDocument() )
        return 0;
    return _value.GetStr();
}

template<typename xchar>
void XMLNodeT<xchar>::SetValue( const xchar* str, bool staticMem )
{
    if ( staticMem ) {
        _value.SetInternedStr( str );
    }
    else {
        _value.SetStr( str );
    }
}

template<typename xchar>
void XMLNodeT<xchar>::DeleteChildren()
{
    while( _firstChild ) {
        TIXMLASSERT( _lastChild );
        TIXMLASSERT( _firstChild->_document == _document );
        XMLNodeT<xchar>* node = _firstChild;
        Unlink( node );

        DeleteNode( node );
    }
    _firstChild = _lastChild = 0;
}

template<typename xchar>
void XMLNodeT<xchar>::Unlink( XMLNodeT<xchar>* child )
{
    TIXMLASSERT( child );
    TIXMLASSERT( child->_document == _document );
    TIXMLASSERT( child->_parent == this );
    if ( child == _firstChild ) {
        _firstChild = _firstChild->_next;
    }
    if ( child == _lastChild ) {
        _lastChild = _lastChild->_prev;
    }

    if ( child->_prev ) {
        child->_prev->_next = child->_next;
    }
    if ( child->_next ) {
        child->_next->_prev = child->_prev;
    }
	child->_parent = 0;
}

template<typename xchar>
void XMLNodeT<xchar>::DeleteChild( XMLNodeT<xchar>* node )
{
    TIXMLASSERT( node );
    TIXMLASSERT( node->_document == _document );
    TIXMLASSERT( node->_parent == this );
    DeleteNode( node );
}

template<typename xchar>
XMLNodeT<xchar>* XMLNodeT<xchar>::InsertEndChild( XMLNodeT<xchar>* addThis )
{
    TIXMLASSERT( addThis );
    if ( addThis->_document != _document ) {
        TIXMLASSERT( false );
        return 0;
    }
    InsertChildPreamble( addThis );

    if ( _lastChild ) {
        TIXMLASSERT( _firstChild );
        TIXMLASSERT( _lastChild->_next == 0 );
        _lastChild->_next = addThis;
        addThis->_prev = _lastChild;
        _lastChild = addThis;

        addThis->_next = 0;
    }
    else {
        TIXMLASSERT( _firstChild == 0 );
        _firstChild = _lastChild = addThis;

        addThis->_prev = 0;
        addThis->_next = 0;
    }
    addThis->_parent = this;
    return addThis;
}

template<typename xchar>
XMLNodeT<xchar>* XMLNodeT<xchar>::InsertFirstChild( XMLNodeT<xchar>* addThis )
{
    TIXMLASSERT( addThis );
    if ( addThis->_document != _document ) {
        TIXMLASSERT( false );
        return 0;
    }
    InsertChildPreamble( addThis );

    if ( _firstChild ) {
        TIXMLASSERT( _lastChild );
        TIXMLASSERT( _firstChild->_prev == 0 );

        _firstChild->_prev = addThis;
        addThis->_next = _firstChild;
        _firstChild = addThis;

        addThis->_prev = 0;
    }
    else {
        TIXMLASSERT( _lastChild == 0 );
        _firstChild = _lastChild = addThis;

        addThis->_prev = 0;
        addThis->_next = 0;
    }
    addThis->_parent = this;
    return addThis;
}

template<typename xchar>
XMLNodeT<xchar>* XMLNodeT<xchar>::InsertAfterChild( XMLNodeT<xchar>* afterThis, XMLNodeT<xchar>* addThis )
{
    TIXMLASSERT( addThis );
    if ( addThis->_document != _document ) {
        TIXMLASSERT( false );
        return 0;
    }

    TIXMLASSERT( afterThis );

    if ( afterThis->_parent != this ) {
        TIXMLASSERT( false );
        return 0;
    }

    if ( afterThis->_next == 0 ) {
        // The last node or the only node.
        return InsertEndChild( addThis );
    }
    InsertChildPreamble( addThis );
    addThis->_prev = afterThis;
    addThis->_next = afterThis->_next;
    afterThis->_next->_prev = addThis;
    afterThis->_next = addThis;
    addThis->_parent = this;
    return addThis;
}


template<typename xchar>
const XMLElementT<xchar>* XMLNodeT<xchar>::FirstChildElement( const xchar* name ) const
{
    for( const XMLNodeT<xchar>* node = _firstChild; node; node = node->_next ) {
        const XMLElementT<xchar>* element = node->ToElement();
        if ( element ) {
            if ( !name || XMLUtilT<xchar>::StringEqual( element->Name(), name ) ) {
                return element;
            }
        }
    }
    return 0;
}

template<typename xchar>
const XMLElementT<xchar>* XMLNodeT<xchar>::LastChildElement( const xchar* name ) const
{
    for( const XMLNodeT<xchar>* node = _lastChild; node; node = node->_prev ) {
        const XMLElementT<xchar>* element = node->ToElement();
        if ( element ) {
            if ( !name || XMLUtilT<xchar>::StringEqual( element->Name(), name ) ) {
                return element;
            }
        }
    }
    return 0;
}

template<typename xchar>
const XMLElementT<xchar>* XMLNodeT<xchar>::NextSiblingElement( const xchar* name ) const
{
    for( const XMLNodeT<xchar>* node = _next; node; node = node->_next ) {
        const XMLElementT<xchar>* element = node->ToElement();
        if ( element
                && (!name || XMLUtilT<xchar>::StringEqual( name, element->Name() ))) {
            return element;
        }
    }
    return 0;
}

template<typename xchar>
const XMLElementT<xchar>* XMLNodeT<xchar>::PreviousSiblingElement( const xchar* name ) const
{
    for( const XMLNodeT<xchar>* node = _prev; node; node = node->_prev ) {
        const XMLElementT<xchar>* element = node->ToElement();
        if ( element
                && (!name || XMLUtilT<xchar>::StringEqual( name, element->Name() ))) {
            return element;
        }
    }
    return 0;
}

template<typename xchar>
xchar* XMLNodeT<xchar>::ParseDeep( xchar* p, StrPairT<xchar>* parentEnd )
{
    // This is a recursive method, but thinking about it "at the current level"
    // it is a pretty simple flat list:
    //		<foo/>
    //		<!-- comment -->
    //
    // With a special case:
    //		<foo>
    //		</foo>
    //		<!-- comment -->
    //
    // Where the closing element (/foo) *must* be the next thing after the opening
    // element, and the names must match. BUT the tricky bit is that the closing
    // element will be read by the child.
    //
    // 'endTag' is the end tag for this node, it is returned by a call to a child.
    // 'parentEnd' is the end tag for the parent, which is filled in and returned.

    while( p && *p ) {
        XMLNodeT<xchar>* node = 0;

        p = _document->Identify( p, &node );
        if ( node == 0 ) {
            break;
        }

        StrPairT<xchar> endTag;
        p = node->ParseDeep( p, &endTag );
        if ( !p ) {
            DeleteNode( node );
            if ( !_document->Error() ) {
                _document->SetError( XML_ERROR_PARSING, 0, 0 );
            }
            break;
        }

        XMLDeclarationT<xchar>* decl = node->ToDeclaration();
        if ( decl ) {
                // A declaration can only be the first child of a document.
                // Set error, if document already has children.
                if ( !_document->NoChildren() ) {
                        _document->SetError( XML_ERROR_PARSING_DECLARATION, decl->Value(), 0);
                        DeleteNode( decl );
                        break;
                }
        }

        XMLElementT<xchar>* ele = node->ToElement();
        if ( ele ) {
            // We read the end tag. Return it to the parent.
            if ( ele->ClosingType() == XMLElementT<xchar>::CLOSING ) {
                if ( parentEnd ) {
                    ele->_value.TransferTo( parentEnd );
                }
                node->_memPool->SetTracked();   // created and then immediately deleted.
                DeleteNode( node );
                return p;
            }

            // Handle an end tag returned to this level.
            // And handle a bunch of annoying errors.
            bool mismatch = false;
            if ( endTag.Empty() ) {
                if ( ele->ClosingType() == XMLElementT<xchar>::OPEN ) {
                    mismatch = true;
                }
            }
            else {
                if ( ele->ClosingType() != XMLElementT<xchar>::OPEN ) {
                    mismatch = true;
                }
                else if ( !XMLUtilT<xchar>::StringEqual( endTag.GetStr(), ele->Name() ) ) {
                    mismatch = true;
                }
            }
            if ( mismatch ) {
                _document->SetError( XML_ERROR_MISMATCHED_ELEMENT, ele->Name(), 0 );
                DeleteNode( node );
                break;
            }
        }
        InsertEndChild( node );
    }
    return 0;
}

template<typename xchar>
void XMLNodeT<xchar>::DeleteNode( XMLNodeT<xchar>* node )
{
    if ( node == 0 ) {
        return;
    }
    MemPool* pool = node->_memPool;
    node->~XMLNodeT();
    pool->Free( node );
}

template<typename xchar>
void XMLNodeT<xchar>::InsertChildPreamble( XMLNodeT<xchar>* insertThis ) const
{
    TIXMLASSERT( insertThis );
    TIXMLASSERT( insertThis->_document == _document );

    if ( insertThis->_parent )
        insertThis->_parent->Unlink( insertThis );
    else
        insertThis->_memPool->SetTracked();
}

// --------- XMLText ---------- //
template <typename xchar>
xchar* XMLTextT<xchar>::ParseDeep( xchar* p, StrPairT<xchar>* )
{
    const xchar* start = p;
    if ( this->CData() ) {
		xchar Tag1[] = {']', ']', '>', 0};
        p = _value.ParseText( p, Tag1, StrPairT<xchar>::NEEDS_NEWLINE_NORMALIZATION );
        if ( !p ) {
            _document->SetError( XML_ERROR_PARSING_CDATA, start, 0 );
        }
        return p;
    }
    else {
        int flags = _document->ProcessEntities() ? StrPairT<xchar>::TEXT_ELEMENT : StrPairT<xchar>::TEXT_ELEMENT_LEAVE_ENTITIES;
        if ( _document->WhitespaceMode() == COLLAPSE_WHITESPACE ) {
            flags |= StrPairT<xchar>::NEEDS_WHITESPACE_COLLAPSING;
        }
		xchar Tag2[] = {'<', 0};
        p = _value.ParseText( p, Tag2, flags );
        if ( p && *p ) {
            return p-1;
        }
        if ( !p ) {
            _document->SetError( XML_ERROR_PARSING_TEXT, start, 0 );
        }
    }
    return 0;
}

template <typename xchar>
XMLNodeT<xchar>* XMLTextT<xchar>::ShallowClone( XMLDocumentT<xchar>* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLTextT<xchar>* text = doc->NewText( Value() );	// fixme: this will always allocate memory. Intern?
    text->SetCData( this->CData() );
    return text;
}

template <typename xchar>
bool XMLTextT<xchar>::ShallowEqual( const XMLNodeT<xchar>* compare ) const
{
    const XMLTextT<xchar>* text = compare->ToText();
    return ( text && XMLUtilT<xchar>::StringEqual( text->Value(), Value() ) );
}

template <typename xchar>
bool XMLTextT<xchar>::Accept( XMLVisitorT<xchar>* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}


// --------- XMLComment ---------- //

template <typename xchar>
XMLCommentT<xchar>::XMLCommentT( XMLDocumentT<xchar>* doc ) : XMLNodeT<xchar>( doc )
{
}


template <typename xchar>
XMLCommentT<xchar>::~XMLCommentT()
{
}


template <typename xchar>
xchar* XMLCommentT<xchar>::ParseDeep( xchar* p, StrPairT<xchar>* )
{
    // Comment parses as text.
    const xchar* start = p;
	xchar cmtendTag[] = {'-', '-', '>', 0};
	p = _value.ParseText( p, cmtendTag, StrPairT<xchar>::COMMENT );
    if ( p == 0 ) {
        _document->SetError( XML_ERROR_PARSING_COMMENT, start, 0 );
    }
    return p;
}


template <typename xchar>
XMLNodeT<xchar>* XMLCommentT<xchar>::ShallowClone( XMLDocumentT<xchar>* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLCommentT<xchar>* comment = doc->NewComment( Value() );	// fixme: this will always allocate memory. Intern?
    return comment;
}


template <typename xchar>
bool XMLCommentT<xchar>::ShallowEqual( const XMLNodeT<xchar>* compare ) const
{
    TIXMLASSERT( compare );
    const XMLCommentT<xchar>* comment = compare->ToComment();
    return ( comment && XMLUtilT<xchar>::StringEqual( comment->Value(), Value() ));
}


template <typename xchar>
bool XMLCommentT<xchar>::Accept( XMLVisitorT<xchar>* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}


// --------- XMLDeclaration ---------- //

template <typename xchar>
XMLDeclarationT<xchar>::XMLDeclarationT( XMLDocumentT<xchar>* doc ) : XMLNodeT<xchar>( doc )
{
}


template <typename xchar>
XMLDeclarationT<xchar>::~XMLDeclarationT()
{
    //printf( "~XMLDeclaration\n" );
}


template <typename xchar>
xchar* XMLDeclarationT<xchar>::ParseDeep( xchar* p, StrPairT<xchar>* )
{
    // Declaration parses as text.
    const xchar* start = p;
	xchar endTag[] = {'?', '>', 0};
	p = _value.ParseText( p, endTag, StrPairT<xchar>::NEEDS_NEWLINE_NORMALIZATION );
    if ( p == 0 ) {
        _document->SetError( XML_ERROR_PARSING_DECLARATION, start, 0 );
    }
    return p;
}


template <typename xchar>
XMLNodeT<xchar>* XMLDeclarationT<xchar>::ShallowClone( XMLDocumentT<xchar>* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLDeclarationT<xchar>* dec = doc->NewDeclaration( Value() );	// fixme: this will always allocate memory. Intern?
    return dec;
}


template <typename xchar>
bool XMLDeclarationT<xchar>::ShallowEqual( const XMLNodeT<xchar>* compare ) const
{
    TIXMLASSERT( compare );
    const XMLDeclarationT<xchar>* declaration = compare->ToDeclaration();
    return ( declaration && XMLUtilT<xchar>::StringEqual( declaration->Value(), Value() ));
}



template <typename xchar>
bool XMLDeclarationT<xchar>::Accept( XMLVisitorT<xchar>* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}

// --------- XMLUnknown ---------- //
template <typename xchar>
XMLUnknownT<xchar>::XMLUnknownT( XMLDocumentT<xchar>* doc ) : XMLNodeT<xchar>( doc )
{
}

template <typename xchar>
XMLUnknownT<xchar>::~XMLUnknownT()
{
}

template <typename xchar>
xchar* XMLUnknownT<xchar>::ParseDeep( xchar* p, StrPairT<xchar>* )
{
    // Unknown parses as text.
    const xchar* start = p;
	xchar endTag[] = {'>', 0};
	p = _value.ParseText( p, endTag, StrPairT<xchar>::NEEDS_NEWLINE_NORMALIZATION );
    if ( !p ) {
        _document->SetError( XML_ERROR_PARSING_UNKNOWN, start, 0 );
    }
    return p;
}

template <typename xchar>
XMLNodeT<xchar>* XMLUnknownT<xchar>::ShallowClone( XMLDocumentT<xchar>* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLUnknownT<xchar>* text = doc->NewUnknown( Value() );	// fixme: this will always allocate memory. Intern?
    return text;
}

template <typename xchar>
bool XMLUnknownT<xchar>::ShallowEqual( const XMLNodeT<xchar>* compare ) const
{
    TIXMLASSERT( compare );
    const XMLUnknownT<xchar>* unknown = compare->ToUnknown();
    return ( unknown && XMLUtilT<xchar>::StringEqual( unknown->Value(), Value() ));
}

template <typename xchar>
bool XMLUnknownT<xchar>::Accept( XMLVisitorT<xchar>* visitor ) const
{
    TIXMLASSERT( visitor );
    return visitor->Visit( *this );
}

// --------- XMLAttribute ---------- //

template <typename xchar>
const xchar* XMLAttributeT<xchar>::Name() const 
{
    return _name.GetStr();
}

template <typename xchar>
const xchar* XMLAttributeT<xchar>::Value() const 
{
    return _value.GetStr();
}

template <typename xchar>
xchar* XMLAttributeT<xchar>::ParseDeep( xchar* p, bool processEntities )
{
    // Parse using the name rules: bug fix, was using ParseText before
    p = _name.ParseName( p );
    if ( !p || !*p ) {
        return 0;
    }

    // Skip white space before =
    p = XMLUtilT<xchar>::SkipWhiteSpace( p );
    if ( *p != '=' ) {
        return 0;
    }

    ++p;	// move up to opening quote
    p = XMLUtilT<xchar>::SkipWhiteSpace( p );
    if ( *p != '\"' && *p != '\'' ) {
        return 0;
    }

    xchar endTag[2] = { *p, 0 };
    ++p;	// move past opening quote

    p = _value.ParseText( p, endTag, processEntities ? StrPairT<xchar>::ATTRIBUTE_VALUE : StrPairT<xchar>::ATTRIBUTE_VALUE_LEAVE_ENTITIES );
    return p;
}


template <typename xchar>
void XMLAttributeT<xchar>::SetName( const xchar* n )
{
    _name.SetStr( n );
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryIntValue( int* value ) const
{
    if ( XMLUtilT<xchar>::ToInt( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryUnsignedValue( unsigned int* value ) const
{
    if ( XMLUtilT<xchar>::ToUnsigned( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryInt8Value( char* value ) const
{
    if ( XMLUtilT<xchar>::ToInt8( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryUnsigned8Value( unsigned char* value ) const
{
    if ( XMLUtilT<xchar>::ToUnsigned8( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryInt16Value( short* value ) const
{
    if ( XMLUtilT<xchar>::ToInt16( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryUnsigned16Value( unsigned short* value ) const
{
    if ( XMLUtilT<xchar>::ToUnsigned16( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryInt64Value( long long* value ) const
{
    if ( XMLUtilT<xchar>::ToInt64( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryUnsigned64Value( unsigned long long* value ) const
{
    if ( XMLUtilT<xchar>::ToUnsigned64( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryBoolValue( bool* value ) const
{
    if ( XMLUtilT<xchar>::ToBool( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryFloatValue( float* value ) const
{
    if ( XMLUtilT<xchar>::ToFloat( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
XMLError XMLAttributeT<xchar>::QueryDoubleValue( double* value ) const
{
    if ( XMLUtilT<xchar>::ToDouble( Value(), value )) {
        return XML_NO_ERROR;
    }
    return XML_WRONG_ATTRIBUTE_TYPE;
}


template <typename xchar>
void XMLAttributeT<xchar>::SetAttribute( const xchar* v )
{
    _value.SetStr( v );
}


template <typename xchar>
void XMLAttributeT<xchar>::SetAttribute( int v )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    _value.SetStr( buf );
}


template <typename xchar>
void XMLAttributeT<xchar>::SetAttribute( unsigned v )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    _value.SetStr( buf );
}


template <typename xchar>
void XMLAttributeT<xchar>::SetAttribute( bool v )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    _value.SetStr( buf );
}

template <typename xchar>
void XMLAttributeT<xchar>::SetAttribute( double v )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    _value.SetStr( buf );
}

template <typename xchar>
void XMLAttributeT<xchar>::SetAttribute( float v )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    _value.SetStr( buf );
}


// --------- XMLElement ---------- //
template <typename xchar>
XMLElementT<xchar>::XMLElementT( XMLDocumentT<xchar>* doc ) : XMLNodeT<xchar>( doc ),
    _closingType( 0 ),
    _rootAttribute( 0 )
{
}

template <typename xchar>
XMLElementT<xchar>::~XMLElementT()
{
    while( _rootAttribute ) {
        XMLAttributeT<xchar>* next = _rootAttribute->_next;
        DeleteAttribute( _rootAttribute );
        _rootAttribute = next;
    }
}

template <typename xchar>
const XMLAttributeT<xchar>* XMLElementT<xchar>::FindAttribute( const xchar* name ) const
{
    for( XMLAttributeT<xchar>* a = _rootAttribute; a; a = a->_next ) {
        if ( XMLUtilT<xchar>::StringEqual( a->Name(), name ) ) {
            return a;
        }
    }
    return 0;
}

template <typename xchar>
const xchar* XMLElementT<xchar>::Attribute( const xchar* name, const xchar* value ) const
{
    const XMLAttributeT<xchar>* a = FindAttribute( name );
    if ( !a ) {
        return 0;
    }
    if ( !value || XMLUtilT<xchar>::StringEqual( a->Value(), value )) {
        return a->Value();
    }
    return 0;
}

template <typename xchar>
const xchar* XMLElementT<xchar>::GetText() const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        return FirstChild()->Value();
    }
    return 0;
}

template <typename xchar>
void	XMLElementT<xchar>::SetText( const xchar* inText )
{
	if ( FirstChild() && FirstChild()->ToText() )
		FirstChild()->SetValue( inText );
	else {
		XMLTextT<xchar>*	theText = GetDocument()->NewText( inText );
		InsertFirstChild( theText );
	}
}

template <typename xchar>
void XMLElementT<xchar>::SetText( int v ) 
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}

template <typename xchar>
void XMLElementT<xchar>::SetText( unsigned v ) 
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}

template <typename xchar>
void XMLElementT<xchar>::SetText( bool v ) 
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}

template <typename xchar>
void XMLElementT<xchar>::SetText( float v ) 
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}

template <typename xchar>
void XMLElementT<xchar>::SetText( double v ) 
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    SetText( buf );
}

template <typename xchar>
XMLError XMLElementT<xchar>::QueryIntText( int* ival ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const xchar* t = FirstChild()->Value();
        if ( XMLUtilT<xchar>::ToInt( t, ival ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}

template <typename xchar>
XMLError XMLElementT<xchar>::QueryUnsignedText( unsigned* uval ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const xchar* t = FirstChild()->Value();
        if ( XMLUtilT<xchar>::ToUnsigned( t, uval ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}

template <typename xchar>
XMLError XMLElementT<xchar>::QueryBoolText( bool* bval ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const xchar* t = FirstChild()->Value();
        if ( XMLUtilT<xchar>::ToBool( t, bval ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}

template <typename xchar>
XMLError XMLElementT<xchar>::QueryDoubleText( double* dval ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const xchar* t = FirstChild()->Value();
        if ( XMLUtilT<xchar>::ToDouble( t, dval ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}

template <typename xchar>
XMLError XMLElementT<xchar>::QueryFloatText( float* fval ) const
{
    if ( FirstChild() && FirstChild()->ToText() ) {
        const xchar* t = FirstChild()->Value();
        if ( XMLUtilT<xchar>::ToFloat( t, fval ) ) {
            return XML_SUCCESS;
        }
        return XML_CAN_NOT_CONVERT_TEXT;
    }
    return XML_NO_TEXT_NODE;
}


template <typename xchar>
XMLAttributeT<xchar>* XMLElementT<xchar>::FindOrCreateAttribute( const xchar* name )
{
    XMLAttributeT<xchar>* last = 0;
    XMLAttributeT<xchar>* attrib = 0;
    for( attrib = _rootAttribute;
            attrib;
            last = attrib, attrib = attrib->_next ) {
        if ( XMLUtilT<xchar>::StringEqual( attrib->Name(), name ) ) {
            break;
        }
    }
    if ( !attrib ) {
        TIXMLASSERT( sizeof( XMLAttribute ) == _document->_attributePool.ItemSize() );
        attrib = new (_document->_attributePool.Alloc() ) XMLAttributeT<xchar>();
        attrib->_memPool = &_document->_attributePool;
        if ( last ) {
            last->_next = attrib;
        }
        else {
            _rootAttribute = attrib;
        }
        attrib->SetName( name );
        attrib->_memPool->SetTracked(); // always created and linked.
    }
    return attrib;
}

template <typename xchar>
void XMLElementT<xchar>::DeleteAttribute( const xchar* name )
{
    XMLAttributeT<xchar>* prev = 0;
    for( XMLAttributeT<xchar>* a=_rootAttribute; a; a=a->_next ) {
        if ( XMLUtilT<xchar>::StringEqual( name, a->Name() ) ) {
            if ( prev ) {
                prev->_next = a->_next;
            }
            else {
                _rootAttribute = a->_next;
            }
            DeleteAttribute( a );
            break;
        }
        prev = a;
    }
}

template <typename xchar>
xchar* XMLElementT<xchar>::ParseAttributes( xchar* p )
{
    const xchar* start = p;
    XMLAttributeT<xchar>* prevAttribute = 0;

    // Read the attributes.
    while( p ) {
        p = XMLUtilT<xchar>::SkipWhiteSpace( p );
        if ( !(*p) ) {
            _document->SetError( XML_ERROR_PARSING_ELEMENT, start, Name() );
            return 0;
        }

        // attribute.
        if (XMLUtilT<xchar>::IsNameStartChar( *p ) ) {
            TIXMLASSERT( sizeof( XMLAttribute ) == _document->_attributePool.ItemSize() );
            XMLAttributeT<xchar>* attrib = new (_document->_attributePool.Alloc() ) XMLAttributeT<xchar>();
            attrib->_memPool = &_document->_attributePool;
			attrib->_memPool->SetTracked();

            p = attrib->ParseDeep( p, _document->ProcessEntities() );
            if ( !p || Attribute( attrib->Name() ) ) {
                DeleteAttribute( attrib );
                _document->SetError( XML_ERROR_PARSING_ATTRIBUTE, start, p );
                return 0;
            }
            // There is a minor bug here: if the attribute in the source xml
            // document is duplicated, it will not be detected and the
            // attribute will be doubly added. However, tracking the 'prevAttribute'
            // avoids re-scanning the attribute list. Preferring performance for
            // now, may reconsider in the future.
            if ( prevAttribute ) {
                prevAttribute->_next = attrib;
            }
            else {
                _rootAttribute = attrib;
            }
            prevAttribute = attrib;
        }
        // end of the tag
        else if ( *p == '>' ) {
            ++p;
            break;
        }
        // end of the tag
        else if ( *p == '/' && *(p+1) == '>' ) {
            _closingType = CLOSED;
            return p+2;	// done; sealed element.
        }
        else {
            _document->SetError( XML_ERROR_PARSING_ELEMENT, start, p );
            return 0;
        }
    }
    return p;
}

template <typename xchar>
void XMLElementT<xchar>::DeleteAttribute( XMLAttributeT<xchar>* attribute )
{
    if ( attribute == 0 ) {
        return;
    }
    MemPool* pool = attribute->_memPool;
    attribute->~XMLAttributeT();
    pool->Free( attribute );
}

//
//	<ele></ele>
//	<ele>foo<b>bar</b></ele>
//
template <typename xchar>
xchar* XMLElementT<xchar>::ParseDeep( xchar* p, StrPairT<xchar>* strPair )
{
    // Read the element name.
    p = XMLUtilT<xchar>::SkipWhiteSpace( p );

    // The closing element is the </element> form. It is
    // parsed just like a regular element then deleted from
    // the DOM.
    if ( *p == '/' ) {
        _closingType = CLOSING;
        ++p;
    }

    p = _value.ParseName( p );
    if ( _value.Empty() ) {
        return 0;
    }

    p = ParseAttributes( p );
    if ( !p || !*p || _closingType ) {
        return p;
    }

    p = XMLNodeT<xchar>::ParseDeep( p, strPair );
    return p;
}


template <typename xchar>
XMLNodeT<xchar>* XMLElementT<xchar>::ShallowClone( XMLDocumentT<xchar>* doc ) const
{
    if ( !doc ) {
        doc = _document;
    }
    XMLElementT<xchar>* element = doc->NewElement( Value() );					// fixme: this will always allocate memory. Intern?
    for( const XMLAttributeT<xchar>* a=FirstAttribute(); a; a=a->Next() ) {
        element->SetAttribute( a->Name(), a->Value() );					// fixme: this will always allocate memory. Intern?
    }
    return element;
}

template <typename xchar>
bool XMLElementT<xchar>::ShallowEqual( const XMLNodeT<xchar>* compare ) const
{
    TIXMLASSERT( compare );
    const XMLElementT<xchar>* other = compare->ToElement();
    if ( other && XMLUtilT<xchar>::StringEqual( other->Name(), Name() )) {

        const XMLAttributeT<xchar>* a=FirstAttribute();
        const XMLAttributeT<xchar>* b=other->FirstAttribute();

        while ( a && b ) {
            if ( !XMLUtilT<xchar>::StringEqual( a->Value(), b->Value() ) ) {
                return false;
            }
            a = a->Next();
            b = b->Next();
        }
        if ( a || b ) {
            // different count
            return false;
        }
        return true;
    }
    return false;
}

template <typename xchar>
bool XMLElementT<xchar>::Accept( XMLVisitorT<xchar>* visitor ) const
{
    TIXMLASSERT( visitor );
    if ( visitor->VisitEnter( *this, _rootAttribute ) ) {
        for ( const XMLNodeT<xchar>* node=FirstChild(); node; node=node->NextSibling() ) {
            if ( !node->Accept( visitor ) ) {
                break;
            }
        }
    }
    return visitor->VisitExit( *this );
}


// --------- XMLDocument ----------- //

// Warning: List must match 'enum XMLError'
template<typename xchar>
const char* XMLDocumentT<xchar>::_errorNames[XML_ERROR_COUNT] = {
    "XML_SUCCESS",
    "XML_NO_ATTRIBUTE",
    "XML_WRONG_ATTRIBUTE_TYPE",
    "XML_ERROR_FILE_NOT_FOUND",
    "XML_ERROR_FILE_COULD_NOT_BE_OPENED",
    "XML_ERROR_FILE_READ_ERROR",
    "XML_ERROR_ELEMENT_MISMATCH",
    "XML_ERROR_PARSING_ELEMENT",
    "XML_ERROR_PARSING_ATTRIBUTE",
    "XML_ERROR_IDENTIFYING_TAG",
    "XML_ERROR_PARSING_TEXT",
    "XML_ERROR_PARSING_CDATA",
    "XML_ERROR_PARSING_COMMENT",
    "XML_ERROR_PARSING_DECLARATION",
    "XML_ERROR_PARSING_UNKNOWN",
    "XML_ERROR_EMPTY_DOCUMENT",
    "XML_ERROR_MISMATCHED_ELEMENT",
    "XML_ERROR_PARSING",
    "XML_CAN_NOT_CONVERT_TEXT",
    "XML_NO_TEXT_NODE"
};

template<typename xchar>
XMLDocumentT<xchar>::XMLDocumentT( bool processEntities, Whitespace whitespace ) :
    XMLNodeT( 0 ),
    _writeBOM( false ),
    _processEntities( processEntities ),
    _errorID( XML_NO_ERROR ),
    _whitespace( whitespace ),
    _errorStr1( 0 ),
    _errorStr2( 0 ),
    _charBuffer( 0 )
{
    // avoid VC++ C4355 warning about 'this' in initializer list (C4355 is off by default in VS2012+)
    _document = this;
}

template<typename xchar>
XMLDocumentT<xchar>::~XMLDocumentT()
{
    Clear();
}

template<typename xchar>
void XMLDocumentT<xchar>::Clear()
{
    DeleteChildren();

#ifdef DEBUG
    const bool hadError = Error();
#endif
    _errorID = XML_NO_ERROR;
    _errorStr1 = 0;
    _errorStr2 = 0;

    delete [] _charBuffer;
    _charBuffer = 0;

#if 0
    _textPool.Trace( "text" );
    _elementPool.Trace( "element" );
    _commentPool.Trace( "comment" );
    _attributePool.Trace( "attribute" );
#endif
    
#ifdef DEBUG
    if ( !hadError ) {
        TIXMLASSERT( _elementPool.CurrentAllocs()   == _elementPool.Untracked() );
        TIXMLASSERT( _attributePool.CurrentAllocs() == _attributePool.Untracked() );
        TIXMLASSERT( _textPool.CurrentAllocs()      == _textPool.Untracked() );
        TIXMLASSERT( _commentPool.CurrentAllocs()   == _commentPool.Untracked() );
    }
#endif
}

template<typename xchar>
XMLElementT<xchar>* XMLDocumentT<xchar>::NewElement( const xchar* name )
{
    TIXMLASSERT( sizeof( XMLElement ) == _elementPool.ItemSize() );
    XMLElementT<xchar>* ele = new (_elementPool.Alloc()) XMLElementT<xchar>( this );
    ele->_memPool = &_elementPool;
    ele->SetName( name );
    return ele;
}

template<typename xchar>
XMLCommentT<xchar>* XMLDocumentT<xchar>::NewComment( const xchar* str )
{
    TIXMLASSERT( sizeof( XMLComment ) == _commentPool.ItemSize() );
    XMLCommentT<xchar>* comment = new (_commentPool.Alloc()) XMLCommentT<xchar>( this );
    comment->_memPool = &_commentPool;
    comment->SetValue( str );
    return comment;
}

template<typename xchar>
XMLTextT<xchar>* XMLDocumentT<xchar>::NewText( const xchar* str )
{
    TIXMLASSERT( sizeof( XMLTextT<xchar> ) == _textPool.ItemSize() );
    XMLTextT<xchar>* text = new (_textPool.Alloc()) XMLTextT<xchar>( this );
    text->_memPool = &_textPool;
    text->SetValue( str );
    return text;
}

template<typename xchar>
XMLDeclarationT<xchar>* XMLDocumentT<xchar>::NewDeclaration( const xchar* str )
{
    TIXMLASSERT( sizeof( XMLDeclaration ) == _commentPool.ItemSize() );
    XMLDeclarationT<xchar>* dec = new (_commentPool.Alloc()) XMLDeclarationT<xchar>( this );
    dec->_memPool = &_commentPool;
	
	xchar xmlTag[] = {'x', 'm', 'l', ' ', 'v', 'e', 'r', 's', 'i', 'o', 'n', '=', 
					'"', '1', '.', '0', '"', ' ', 'e', 'n', 'c', 'o', 'd', 'i', 
					'n', 'g', '=', '"', 'U', 'T', 'F', '-', '8', '"', 0};
    dec->SetValue( str ? str : xmlTag );
    return dec;
}

template<typename xchar>
XMLUnknownT<xchar>* XMLDocumentT<xchar>::NewUnknown( const xchar* str )
{
    TIXMLASSERT( sizeof( XMLUnknown ) == _commentPool.ItemSize() );
    XMLUnknownT<xchar>* unk = new (_commentPool.Alloc()) XMLUnknownT<xchar>( this );
    unk->_memPool = &_commentPool;
    unk->SetValue( str );
    return unk;
}

static FILE* callfopen( const char* filepath, const char* mode )
{
    TIXMLASSERT( filepath );
    TIXMLASSERT( mode );
#if defined(_MSC_VER) && (_MSC_VER >= 1400 ) && (!defined WINCE)
    FILE* fp = 0;
    errno_t err = fopen_s( &fp, filepath, mode );
    if ( err ) {
        return 0;
    }
#else
    FILE* fp = fopen( filepath, mode );
#endif
    return fp;
}


static FILE* callfopen( const wchar_t* filepath, const wchar_t* mode )
{
    TIXMLASSERT( filepath );
    TIXMLASSERT( mode );
#if defined(_MSC_VER) && (_MSC_VER >= 1400 ) && (!defined WINCE)
    FILE* fp = 0;
    errno_t err = _wfopen_s( &fp, filepath, mode );
    if ( err ) {
        return 0;
    }
#else
    FILE* fp = _wfopen( filepath, mode );
#endif
    return fp;
}

template<typename xchar>
void XMLDocumentT<xchar>::DeleteNode( XMLNodeT<xchar>* node )	{
    TIXMLASSERT( node );
    TIXMLASSERT(node->_document == this );
    if (node->_parent) {
        node->_parent->DeleteChild( node );
    }
    else {
        // Isn't in the tree.
        // Use the parent delete.
        // Also, we need to mark it tracked: we 'know'
        // it was never used.
        node->_memPool->SetTracked();
        // Call the static XMLNodeT version:
        XMLNodeT<xchar>::DeleteNode(node);
    }
}

template<typename xchar>
XMLError XMLDocumentT<xchar>::LoadFile( const xchar* filename )
{
    Clear();
	xchar mode[] = {'r', 'b', 0};
    FILE* fp = callfopen( filename, mode );
    if ( !fp ) {
        SetError( XML_ERROR_FILE_NOT_FOUND, filename, 0 );
        return _errorID;
    }
    LoadFile( fp );
    fclose( fp );
    return _errorID;
}

template<typename xchar>
XMLError XMLDocumentT<xchar>::LoadFile( FILE* fp )
{
    Clear();

    fseek( fp, 0, SEEK_SET );
    if ( fgetc( fp ) == EOF && ferror( fp ) != 0 ) {
        SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
        return _errorID;
    }

    fseek( fp, 0, SEEK_END );
    const long filelength = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    if ( filelength == -1L ) {
        SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
        return _errorID;
    }

    if ( (unsigned long)filelength >= (size_t)-1 ) {
        // Cannot handle files which won't fit in buffer together with null terminator
        SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
        return _errorID;
    }

    if ( filelength == 0 ) {
        SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
        return _errorID;
    }

    const size_t size = filelength;
    TIXMLASSERT( _charBuffer == 0 );
    _charBuffer = new char[size+1*sizeof(xchar)];
    size_t read = fread( _charBuffer, 1, size, fp );
    if ( read != size ) {
        SetError( XML_ERROR_FILE_READ_ERROR, 0, 0 );
        return _errorID;
    }

    _charBuffer[size] = 0;
	_charBuffer[size+sizeof(xchar)-1] = 0;

    Parse();
    return _errorID;
}

template< >
XMLError XMLDocumentT<char>::SaveFile( const char* filename, bool compact )
{
	char mode[] = {'w', 0};
    FILE* fp = callfopen( filename, mode );
    if ( !fp ) {
        SetError( XML_ERROR_FILE_COULD_NOT_BE_OPENED, filename, 0 );
        return _errorID;
    }
    SaveFile(fp, compact);
    fclose( fp );
    return _errorID;
}

template< >
XMLError XMLDocumentT<wchar_t>::SaveFile( const wchar_t* filename, bool compact )
{
	wchar_t mode[] = {'w', ',', ' ', 'c', 'c', 's', '=', 'U', 'N', 'I', 'C', 'O', 'D', 'E', 0};
    FILE* fp = callfopen( filename, mode );
    if ( !fp ) {
        SetError( XML_ERROR_FILE_COULD_NOT_BE_OPENED, filename, 0 );
        return _errorID;
    }
	_writeBOM = false; //UNICODE ccs flag automatically writes BOM.
    SaveFile(fp, compact);
    fclose( fp );
    return _errorID;
}

template<typename xchar>
XMLError XMLDocumentT<xchar>::SaveFile( FILE* fp, bool compact )
{
    // Clear any error from the last save, otherwise it will get reported
    // for *this* call.
    SetError( XML_NO_ERROR, 0, 0 );
    XMLPrinterT<xchar> stream( fp, compact );
    Print( &stream );
    return _errorID;
}

template<typename xchar>
XMLError XMLDocumentT<xchar>::Parse( const xchar* p, size_t len )
{
    Clear();

    if ( len == 0 || !p || !*p ) {
        SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
        return _errorID;
    }
    if ( len == (size_t)(-1) ) {
        len = strlen( p );
    }
    TIXMLASSERT( _charBuffer == 0 );
    _charBuffer = new char[ (len+1)*sizeof(xchar) ];
    memcpy( _charBuffer, p, len*sizeof(xchar) );
    _charBuffer[len*sizeof(xchar)] = 0;
	_charBuffer[len*sizeof(xchar)+sizeof(xchar)-1] = 0;

    Parse();
    if ( Error() ) {
        // clean up now essentially dangling memory.
        // and the parse fail can put objects in the
        // pools that are dead and inaccessible.
        DeleteChildren();
        _elementPool.Clear();
        _attributePool.Clear();
        _textPool.Clear();
        _commentPool.Clear();
    }
    return _errorID;
}

template<typename xchar>
void XMLDocumentT<xchar>::Print( XMLPrinterT<xchar>* streamer ) const
{
    if ( streamer ) {
        Accept( streamer );
    }
    else {
        XMLPrinterT<xchar> stdoutStreamer( stdout );
        Accept( &stdoutStreamer );
    }
}

template<typename xchar>
void XMLDocumentT<xchar>::SetError( XMLError error, const xchar* str1, const xchar* str2 )
{
    TIXMLASSERT( error >= 0 && error < XML_ERROR_COUNT );
    _errorID = error;
    _errorStr1 = str1;
    _errorStr2 = str2;
}

template<typename xchar>
const char* XMLDocumentT<xchar>::ErrorName() const
{
	TIXMLASSERT( _errorID >= 0 && _errorID < XML_ERROR_COUNT );
    const char* errorName = _errorNames[_errorID];
    TIXMLASSERT( errorName && errorName[0] );
    return errorName;
}

template< >
void XMLDocumentT<char>::PrintError() const
{
    if ( Error() ) {
        static const int LEN = 20;
        char buf1[LEN] = { 0 };
        char buf2[LEN] = { 0 };

        if ( _errorStr1 ) {
            TIXML_SNPRINTF( buf1, LEN, "%s", _errorStr1 );
        }
        if ( _errorStr2 ) {
            TIXML_SNPRINTF( buf2, LEN, "%s", _errorStr2 );
        }

        // Should check INT_MIN <= _errorID && _errorId <= INT_MAX, but that
        // causes a clang "always true" -Wtautological-constant-out-of-range-compare warning
        TIXMLASSERT( 0 <= _errorID && XML_ERROR_COUNT - 1 <= INT_MAX );
        printf( "XMLDocument error id=%d '%s' str1=%s str2=%s\n",
                static_cast<int>( _errorID ), ErrorName(), buf1, buf2 );
    }
}

template< >
void XMLDocumentT<wchar_t>::PrintError() const
{
    if ( Error() ) {
        static const int LEN = 20;
        wchar_t buf1[LEN] = { 0 };
        wchar_t buf2[LEN] = { 0 };

        if ( _errorStr1 ) {
            TIXML_SNWPRINTF( buf1, LEN, L"%s", _errorStr1 );
        }
        if ( _errorStr2 ) {
            TIXML_SNWPRINTF( buf2, LEN, L"%s", _errorStr2 );
        }

        // Should check INT_MIN <= _errorID && _errorId <= INT_MAX, but that
        // causes a clang "always true" -Wtautological-constant-out-of-range-compare warning
        TIXMLASSERT( 0 <= _errorID && XML_ERROR_COUNT - 1 <= INT_MAX );
        wprintf( L"XMLDocument error id=%d '%S' str1=%s str2=%s\n",
                static_cast<int>( _errorID ), ErrorName(), buf1, buf2 );
    }
}

template<typename xchar>
void XMLDocumentT<xchar>::Parse()
{
    TIXMLASSERT( NoChildren() ); // Clear() must have been called previously
    TIXMLASSERT( _charBuffer );
    char* p = _charBuffer;
    p = XMLUtilT<char>::SkipWhiteSpace( p );
    p = const_cast<char*>( XMLUtilT<char>::ReadBOM( p, &_writeBOM ) );
    if ( !*p ) {
        SetError( XML_ERROR_EMPTY_DOCUMENT, 0, 0 );
        return;
    }
    ParseDeep((xchar*)p, 0 );
}


template<typename xchar>
XMLPrinterT<xchar>::XMLPrinterT( FILE* file, bool compact, int depth ) :
    _elementJustOpened( false ),
    _firstElement( true ),
    _fp( file ),
    _depth( depth ),
    _textDepth( -1 ),
    _processEntities( true ),
    _compactMode( compact )
{
    for( int i=0; i<ENTITY_RANGE; ++i ) {
        _entityFlag[i] = false;
        _restrictedEntityFlag[i] = false;
    }
    for( int i=0; i<NUM_ENTITIES; ++i ) {
        const char entityValue = entities[i].value;
        TIXMLASSERT( 0 <= entityValue && entityValue < ENTITY_RANGE );
        _entityFlag[ (unsigned char)entityValue ] = true;
    }
    _restrictedEntityFlag[(unsigned char)'&'] = true;
    _restrictedEntityFlag[(unsigned char)'<'] = true;
    _restrictedEntityFlag[(unsigned char)'>'] = true;	// not required, but consistency is nice
    _buffer.Push( 0 );
}


template< >
void XMLPrinterT<char>::Print( const char* format, ... )
{
    va_list     va;
    va_start( va, format );

    if ( _fp ) {
        vfprintf( _fp, format, va );
    }
    else {
        const int len = TIXML_VSCPRINTF( format, va );
        // Close out and re-start the va-args
        va_end( va );
        TIXMLASSERT( len >= 0 );
        va_start( va, format );
        TIXMLASSERT( _buffer.Size() > 0 && _buffer[_buffer.Size() - 1] == 0 );
        char* p = _buffer.PushArr( len ) - 1;	// back up over the null terminator.
		TIXML_VSNPRINTF( p, len+1, format, va );
    }
    va_end( va );
}

template< >
void XMLPrinterT<wchar_t>::Print( const wchar_t* format, ... )
{
    va_list     va;
    va_start( va, format );

    if ( _fp ) {
        vfwprintf( _fp, format, va );
    }
    else {
        int len = TIXML_VSCWPRINTF( format, va );
        // Close out and re-start the va-args
        va_end( va );
        va_start( va, format );
        TIXMLASSERT( _buffer.Size() > 0 && _buffer[_buffer.Size() - 1] == 0 );
        wchar_t* p = _buffer.PushArr( len ) - 1;	// back up over the null terminator.
		TIXML_VSNWPRINTF( p, len+1, format, va );
    }
    va_end( va );
}

template<typename xchar>
void XMLPrinterT<xchar>::PrintSpace( int depth )
{
	xchar space[] = {' ', ' ', ' ', ' ', 0};
    for( int i=0; i<depth; ++i ) {
        Print( space );
    }
}


template<typename xchar>
void XMLPrinterT<xchar>::PrintString( const xchar* p, bool restricted )
{
    // Look for runs of bytes between entities to print.
    const xchar* q = p;

    if ( _processEntities ) {
        const bool* flag = restricted ? _restrictedEntityFlag : _entityFlag;
        while ( *q ) {
            TIXMLASSERT( p <= q );
            // Remember, char is sometimes signed. (How many times has that bitten me?)
            if ( *q > 0 && *q < ENTITY_RANGE ) {
                // Check for entities. If one is found, flush
                // the stream up until the entity, write the
                // entity, and keep looking.
                if ( flag[(unsigned char)(*q)] ) {
                    while ( p < q ) {
                        const size_t delta = q - p;
                        // %.*s accepts type int as "precision"
                        const int toPrint = ( INT_MAX < delta ) ? INT_MAX : (int)delta;
						xchar format[] = {'%', '.', '*', 's', 0};
                        Print( format, toPrint, p );
                        p += toPrint;
                    }
                    bool entityPatternPrinted = false;
                    for( int i=0; i<NUM_ENTITIES; ++i ) {
                        if ( entities[i].value == *q ) {
							xchar format[] = {'&', '%', 's', ';', 0};
							xchar* pattern = new xchar[entities[i].length+1];
							for (int j = 0; j < entities[i].length; j++)
								pattern[j] = entities[i].pattern[j];
							pattern[entities[i].length] = 0;
							Print( format, pattern);
                            entityPatternPrinted = true;
                            break;
                        }
                    }
                    if ( !entityPatternPrinted ) {
                        // TIXMLASSERT( entityPatternPrinted ) causes gcc -Wunused-but-set-variable in release
                        TIXMLASSERT( false );
                    }
                    ++p;
                }
            }
            ++q;
            TIXMLASSERT( p <= q );
        }
    }
    // Flush the remaining string. This will be the entire
    // string if an entity wasn't found.
    TIXMLASSERT( p <= q );
    if ( !_processEntities || ( p < q ) ) {
		xchar format[] = {'%', 's', 0};
        Print( format, p );
    }
}


template< >
void XMLPrinterT<char>::PushHeader( bool writeBOM, bool writeDec )
{
    if ( writeBOM ) {
        static const unsigned char bom[] = { TIXML_UTF_LEAD_0, TIXML_UTF_LEAD_1, TIXML_UTF_LEAD_2, 0 };
		
        Print( "%s", bom );
    }
    if ( writeDec ) {		
		char xmlTag[] = {'x', 'm', 'l', ' ', 'v', 'e', 'r', 's', 'i', 'o', 'n', '=', 
						'"', '1', '.', '0', '"'};
        PushDeclaration( xmlTag );
    }
}

template< >
void XMLPrinterT<wchar_t>::PushHeader( bool writeBOM, bool writeDec )
{
    if ( writeBOM ) {
        static const wchar_t bom = TIXML_UNICODE_LEAD_0 | TIXML_UNICODE_LEAD_1 << 8;
		wchar_t format[] = {'%', 'c', 0};
        Print( format, bom );
    }
    if ( writeDec ) {		
		wchar_t xmlTag[] = {'x', 'm', 'l', ' ', 'v', 'e', 'r', 's', 'i', 'o', 'n', '=', 
						'"', '1', '.', '0', '"'};
        PushDeclaration( xmlTag );
    }
}


template<typename xchar>
void XMLPrinterT<xchar>::OpenElement( const xchar* name, bool compactMode )
{
    SealElementIfJustOpened();
    _stack.Push( name );

	xchar newLine[] = {'\n', 0};
    if ( _textDepth < 0 && !_firstElement && !compactMode ) {
        Print( newLine );
    }
    if ( !compactMode ) {
        PrintSpace( _depth );
    }
	xchar format[] = {'<', '%', 's', 0};
    Print( format, name );
    _elementJustOpened = true;
    _firstElement = false;
    ++_depth;
}


template<typename xchar>
void XMLPrinterT<xchar>::PushAttribute( const xchar* name, const xchar* value )
{
    TIXMLASSERT( _elementJustOpened );
	xchar format[] = {' ', '%', 's', '=', '"' , 0};
    Print( format, name );
    PrintString( value, false );
	xchar quote[] = {'"', 0};
    Print( quote );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushAttribute( const xchar* name, int v )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    PushAttribute( name, buf );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushAttribute( const xchar* name, unsigned v )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    PushAttribute( name, buf );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushAttribute( const xchar* name, bool v )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    PushAttribute( name, buf );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushAttribute( const xchar* name, double v )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( v, buf, BUF_SIZE );
    PushAttribute( name, buf );
}


template<typename xchar>
void XMLPrinterT<xchar>::CloseElement( bool compactMode )
{
    --_depth;
    const xchar* name = _stack.Pop();
	xchar ending[] = {'/', '>', 0};
	xchar newLine[] = {'\n', 0};
	xchar format[] =  {'<', '/', '%', 's', '>', 0};

    if ( _elementJustOpened ) {
        Print( ending );
    }
    else {
        if ( _textDepth < 0 && !compactMode) {
            Print( newLine );
            PrintSpace( _depth );
        }
        Print( format, name );
    }

    if ( _textDepth == _depth ) {
        _textDepth = -1;
    }
    if ( _depth == 0 && !compactMode) {
        Print( newLine );
    }
    _elementJustOpened = false;
}


template<typename xchar>
void XMLPrinterT<xchar>::SealElementIfJustOpened()
{
    if ( !_elementJustOpened ) {
        return;
    }
    _elementJustOpened = false;
	xchar ending[] = {'>', 0};
    Print( ending );
}

template<typename xchar>
void XMLPrinterT<xchar>::PushText( const xchar* text, bool cdata )
{
    _textDepth = _depth-1;
	
	xchar format[] = {'<', '!', '[', 'C', 'D', 'A', 'T', 'A', '[', '%', 's', ']', ']', '>', 0};
    SealElementIfJustOpened();
    if ( cdata ) {
        Print( format, text );
    }
    else {
        PrintString( text, true );
    }
}

template<typename xchar>
void XMLPrinterT<xchar>::PushText( int value )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( value, buf, BUF_SIZE );
    PushText( buf, false );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushText( unsigned value )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( value, buf, BUF_SIZE );
    PushText( buf, false );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushText( bool value )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( value, buf, BUF_SIZE );
    PushText( buf, false );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushText( float value )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( value, buf, BUF_SIZE );
    PushText( buf, false );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushText( double value )
{
    xchar buf[BUF_SIZE];
    XMLUtilT<xchar>::ToStr( value, buf, BUF_SIZE );
    PushText( buf, false );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushComment( const xchar* comment )
{
    SealElementIfJustOpened();
	xchar newLine[] = {'\n', 0};
    if ( _textDepth < 0 && !_firstElement && !_compactMode) {
        Print( newLine );
        PrintSpace( _depth );
    }
    _firstElement = false;
	xchar format[] = {'<', '!', '-', '-', '%', 's', '-', '-', '>', 0};
    Print( format, comment );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushDeclaration( const xchar* value )
{
    SealElementIfJustOpened();
	xchar newLine[] = {'\n', 0};
    if ( _textDepth < 0 && !_firstElement && !_compactMode) {
        Print( newLine );
        PrintSpace( _depth );
    }
    _firstElement = false;
	xchar format[] = {'<', '?', '%', 's', '?', '>', 0};
    Print( format, value );
}


template<typename xchar>
void XMLPrinterT<xchar>::PushUnknown( const xchar* value )
{
    SealElementIfJustOpened();
	xchar newLine[] = {'\n', 0};
    if ( _textDepth < 0 && !_firstElement && !_compactMode) {
        Print( newLine );
        PrintSpace( _depth );
    }
    _firstElement = false;
	xchar format[] = {'<', '!', '%', 's', '>', 0};
    Print( format, value );
}


template<typename xchar>
bool XMLPrinterT<xchar>::VisitEnter( const XMLDocumentT<xchar>& doc )
{
    _processEntities = doc.ProcessEntities();
    if ( doc.HasBOM() ) {
        PushHeader( true, false );
    }
    return true;
}


template<typename xchar>
bool XMLPrinterT<xchar>::VisitEnter( const XMLElementT<xchar>& element, const XMLAttributeT<xchar>* attribute )
{
    const XMLElementT<xchar>* parentElem = 0;
    if ( element.Parent() ) {
        parentElem = element.Parent()->ToElement();
    }
    const bool compactMode = parentElem ? CompactMode( *parentElem ) : _compactMode;
    OpenElement( element.Name(), compactMode );
    while ( attribute ) {
        PushAttribute( attribute->Name(), attribute->Value() );
        attribute = attribute->Next();
    }
    return true;
}


template<typename xchar>
bool XMLPrinterT<xchar>::VisitExit( const XMLElementT<xchar>& element )
{
    CloseElement( CompactMode(element) );
    return true;
}


template<typename xchar>
bool XMLPrinterT<xchar>::Visit( const XMLTextT<xchar>& text )
{
    PushText( text.Value(), text.CData() );
    return true;
}


template<typename xchar>
bool XMLPrinterT<xchar>::Visit( const XMLCommentT<xchar>& comment )
{
    PushComment( comment.Value() );
    return true;
}

template<typename xchar>
bool XMLPrinterT<xchar>::Visit( const XMLDeclarationT<xchar>& declaration )
{
    PushDeclaration( declaration.Value() );
    return true;
}


template<typename xchar>
bool XMLPrinterT<xchar>::Visit( const XMLUnknownT<xchar>& unknown )
{
    PushUnknown( unknown.Value() );
    return true;
}

}   // namespace tinyxml2

