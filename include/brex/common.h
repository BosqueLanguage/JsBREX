#pragma once

#include <string>
#include <optional>
#include <vector>
#include <map>
#include <set>

#include "json.hpp"
typedef nlohmann::json json;

#ifdef BREX_DEBUG
#define BREX_ABORT(msg) brex::processAssert(__FILE__, __LINE__, msg)
#define BREX_ASSERT(condition, msg) if(!(condition)) { brex::processAssert(__FILE__, __LINE__, msg); }
#else
#define BREX_ABORT(msg) processAbort(__FILE__, __LINE__, msg)
#define BREX_ASSERT(condition, msg)
#endif

#define UTF8_IS_SINGLEBYTE_ENCODING(byte) (((byte) & 0b10000000) == 0)
#define UTF8_IS_MULTIBYTE_ENCODING(byte) (((byte) & 0b10000000) != 0)

#define UTF8_CHARCODE_USES_SINGLEBYTE_ENCODING(cc) ((cc) <= 0x7F)
#define UTF8_CHARCODE_USES_MULTIBYTE_ENCODING(cc) ((cc) > 0x7F)

namespace brex
{
    typedef std::u8string UnicodeString;
    typedef char8_t UnicodeStringChar;

    typedef std::string CString;
    typedef char CStringChar;

    typedef uint32_t RegexChar;

    struct SingleCharRange
    {
        RegexChar low;
        RegexChar high;
    };

#ifdef BREX_DEBUG
    void processAssert(const char* file, int line, const char* msg) __attribute__ ((noreturn));
#else
    void processAbort(const char* file, int line, const char* msg) __attribute__ ((noreturn));
#endif

    class UnicodeRegexIterator
    {
    public:
        UnicodeString* sstr;

        int64_t spos; //the first position where the iterator is valid (inclusive)
        int64_t epos; //the last position where the iterator is valid (exclusive)

        int64_t curr;

        UnicodeRegexIterator() : sstr(nullptr), spos(0), epos(-1), curr(0) {;}
        UnicodeRegexIterator(UnicodeString* sstr) : sstr(sstr), spos(0), epos(sstr->size() - 1), curr(0) {;}
        UnicodeRegexIterator(UnicodeString* sstr, int64_t spos, int64_t epos, int64_t curr) : sstr(sstr), spos(spos), epos(epos), curr(curr) {;}
        ~UnicodeRegexIterator() = default;

        UnicodeRegexIterator(const UnicodeRegexIterator& other) = default;
        UnicodeRegexIterator(UnicodeRegexIterator&& other) = default;

        UnicodeRegexIterator& operator=(const UnicodeRegexIterator& other) = default;
        UnicodeRegexIterator& operator=(UnicodeRegexIterator&& other) = default;

        int64_t charCodeByteCount() const;
        int64_t charCodeByteCountReverse() const;
        RegexChar toRegexCharCodeFromBytes() const;

        inline bool valid() const
        {
            return (this->spos <= this->curr) & (this->curr <= this->epos);
        }

        inline void inc()
        {
            //if this is a multibyte char then advance by the number of bytes -- fast path on single byte
            if(UTF8_IS_SINGLEBYTE_ENCODING(this->sstr->at(this->curr))) {
                this->curr++;
            }
            else {
                this->curr += this->charCodeByteCount();
            }
        }

        inline void dec()
        {
            //if this is a multibyte char then advance by the number of bytes -- fast path on single byte
            if(UTF8_IS_SINGLEBYTE_ENCODING(this->sstr->at(this->curr))) {
                this->curr--;
            }
            else {
                this->curr -= this->charCodeByteCountReverse();
            }
        }

        inline RegexChar get() const
        {
            //if this is a multibyte char then decode the number of bytes -- fast path on single byte
            if(UTF8_CHARCODE_USES_SINGLEBYTE_ENCODING(this->sstr->at(this->curr))) {
                return this->sstr->at(this->curr);
            }
            else {
                return this->toRegexCharCodeFromBytes();
            }
        }
    };

    class CRegexIterator
    {
    public:
        const CString* sstr;
        
        int64_t spos; //the first position where the iterator is valid (inclusive)
        int64_t epos; //the last position where the iterator is valid (exclusive)

        int64_t curr;

        CRegexIterator() : sstr(nullptr), spos(0), epos(-1), curr(0) {;}
        CRegexIterator(const CString* sstr) : sstr(sstr), spos(0), epos(sstr->size() - 1), curr(0) {;}
        CRegexIterator(const CString* sstr, int64_t spos, int64_t epos, int64_t curr) : sstr(sstr), spos(spos), epos(epos), curr(curr) {;}
        ~CRegexIterator() = default;

        CRegexIterator(const CRegexIterator& other) = default;
        CRegexIterator(CRegexIterator&& other) = default;

        CRegexIterator& operator=(const CRegexIterator& other) = default;
        CRegexIterator& operator=(CRegexIterator&& other) = default;
        
        inline bool valid() const
        {
            return (this->spos <= this->curr) & (this->curr <= this->epos);
        }

        inline void inc()
        {
            this->curr++;
        }

        inline void dec()
        {
            this->curr--;
        }

        inline RegexChar get() const
        {
            return (RegexChar)this->sstr->at(this->curr);
        }
    };

    std::string processRegexCharToBsqStandard(RegexChar c);
    std::string processRegexCharsToBsqStandard(const std::vector<RegexChar>& sv);

    std::string processRegexCharToSMT(RegexChar c);
    std::string processRegexCharsToSMT(const std::vector<RegexChar>& sv);

    size_t charCodeByteCount(const uint8_t* buff);
    RegexChar toRegexCharCodeFromBytes(const uint8_t* buff, size_t length);

    bool isHexEscapePrefix(const uint8_t* s, const uint8_t* e);
    std::optional<RegexChar> decodeHexEscapeAsRegex(const uint8_t* s, const uint8_t* e);
    std::optional<UnicodeString> decodeHexEscapeAsUnicode(const uint8_t* s, const uint8_t* e);
    std::optional<CString> decodeHexEscapeAsC(const uint8_t* s, const uint8_t* e);
    std::vector<uint8_t> extractRegexCharToBytes(RegexChar rc); //utf8 encoded but no escaping

    //Take a bytebuffer (of utf8 bytes) with escapes and convert to/from a UnicodeString
    std::pair<std::optional<UnicodeString>, std::optional<std::u8string>> unescapeUnicodeString(const uint8_t* bytes, size_t length);
    std::pair<std::optional<UnicodeString>, std::optional<std::u8string>> unescapeUnicodeStringLiteralInclMultiline(const uint8_t* bytes, size_t length);
    std::vector<uint8_t> escapeUnicodeString(const UnicodeString& sv);

    //Take a bytebuffer (of char bytes) with escapes and convert to/from an CString
    std::pair<std::optional<CString>, std::optional<std::u8string>> unescapeCString(const uint8_t* bytes, size_t length);
    std::pair<std::optional<CString>, std::optional<std::u8string>> unescapeCStringLiteralInclMultiline(const uint8_t* bytes, size_t length);
    std::vector<uint8_t> escapeCString(const CString& sv);

    //Take a bytebuffer regex literal (of utf8 bytes or chat bytes) with escapes and convert to/from a vector of RegexChars
    std::optional<std::vector<RegexChar>> unescapeUnicodeRegexLiteral(const uint8_t* bytes, size_t length);
    std::optional<std::vector<RegexChar>> unescapeCRegexLiteral(const uint8_t* bytes, size_t length);

    //Take a bytebuffer regex char range element (of utf8 bytes or char bytes) with escapes and convert to a RegexChar
    std::optional<RegexChar> unescapeSingleUnicodeRegexChar(const uint8_t* s, const uint8_t* e);
    std::optional<RegexChar> unescapeSingleCRegexChar(const uint8_t* s, const uint8_t* e);

    std::vector<uint8_t> escapeSingleUnicodeRegexChar(RegexChar c);
    std::vector<uint8_t> escapeRegexUnicodeLiteralCharBuffer(const std::vector<RegexChar>& sv);

    std::vector<uint8_t> escapeSingleCRegexChar(RegexChar c);
    std::vector<uint8_t> escapeRegexCLiteralCharBuffer(const std::vector<RegexChar>& sv);

    //In the parser if we find an invalid literal character or code then generate the right way to escape it for a nice error msg
    std::u8string parserGenerateDiagnosticUnicodeEscapeName(uint8_t c);
    std::u8string parserGenerateDiagnosticCEscapeName(uint8_t c);
    std::u8string parserGenerateDiagnosticEscapeCode(uint8_t c);

    //If we have decode failures then go through and generate nice messages for them
    std::vector<std::u8string> parserValidateEscapeSequences(bool ischar, const uint8_t* s, const uint8_t* e);

    //Scan the string and ensure that there are no multibyte chars that have messed up encodings -- or that the string is only char chars
    std::optional<std::u8string> parserValidateUTF8ByteEncoding(const uint8_t* s, const uint8_t* e);
    std::optional<std::u8string> parserValidateAllCEncoding(const uint8_t* s, const uint8_t* e);

    std::optional<std::u8string> parserValidateUTF8ByteEncoding_SingleChar(const uint8_t* s, const uint8_t* epos);
    std::optional<std::u8string> parserValidateAllCEncoding_SingleChar(const uint8_t* s, const uint8_t* epos);
}

