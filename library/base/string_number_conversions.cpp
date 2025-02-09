
#include "string_number_conversions.h"

#include <limits>

#include "logging.h"

namespace base
{

template<typename STR, typename INT, typename UINT, bool NEG>
struct IntToStringT
{
    // 避免无符号类型对输入取反的编译器警告.
    // 例如下面的代码:
    //     template<typename INT>
    //     INT abs(INT value) { return value<0 ? -value : value; }
    // 即使INT是无符号类型, value<0永远不成立, 取反操作不可能被执行, 编译器
    // 仍然会产生警告. 这里使用一些特例化技巧...
    template<typename INT2, typename UINT2, bool NEG2>
    struct ToUnsignedT {};

    template<typename INT2, typename UINT2>
    struct ToUnsignedT<INT2, UINT2, false>
    {
        static UINT2 ToUnsigned(INT2 value)
        {
            return static_cast<UINT2>(value);
        }
    };

    template<typename INT2, typename UINT2>
    struct ToUnsignedT<INT2, UINT2, true>
    {
        static UINT2 ToUnsigned(INT2 value)
        {
            return static_cast<UINT2>(value<0 ? -value : value);
        }
    };

    // 模板与上面的模板非常类似, 用于测试整数是否为负.
    template<typename INT2, bool NEG2>
    struct TestNegT {};
    template<typename INT2>
    struct TestNegT<INT2, false>
    {
        static bool TestNeg(INT2 value)
        {
            // value是无符号的, 不可能为负.
            return false;
        }
    };
    template<typename INT2>
    struct TestNegT<INT2, true>
    {
        static bool TestNeg(INT2 value)
        {
            return value < 0;
        }
    };

    static STR IntToString(INT value)
    {
        // 每比特需要大约0.3字节(log10(2) ~= 0.3), 或者说每字节大约需要2.4字节
        // (log10(2**8) ~= 2.4). 向上取整每字节需要分配3个字符, 为'-'预留一位.
        const int kOutputBufSize = 3*sizeof(INT) + 1;

        // 分配整个字符串, 从后向前写, 最后求子串.
        STR outbuf(kOutputBufSize, 0);

        bool is_neg = TestNegT<INT, NEG>::TestNeg(value);
        UINT res = ToUnsignedT<INT, UINT, NEG>::ToUnsigned(value);

        for(typename STR::iterator it=outbuf.end(); ;)
        {
            --it;
            DCHECK(it != outbuf.begin());
            *it = static_cast<typename STR::value_type>((res%10) + '0');
            res /= 10;

            // 完成
            if(res == 0)
            {
                if(is_neg)
                {
                    --it;
                    DCHECK(it != outbuf.begin());
                    *it = static_cast<typename STR::value_type>('-');
                }
                return STR(it, outbuf.end());
            }
        }
        NOTREACHED();
        return STR();
    }
};

// 工具类, 用于转换一个字符到给定base的数字.
template<typename CHAR, int BASE, bool BASE_LTE_10> class BaseCharToDigit {};

// base <= 10的特例化.
template<typename CHAR, int BASE> class BaseCharToDigit<CHAR, BASE, true>
{
public:
    static bool Convert(CHAR c, uint8* digit)
    {
        if(c>='0' && c<'0'+BASE)
        {
            *digit = c - '0';
            return true;
        }
        return false;
    }
};

// 10 < base <= 36的特例化.
template<typename CHAR, int BASE> class BaseCharToDigit<CHAR, BASE, false>
{
public:
    static bool Convert(CHAR c, uint8* digit)
    {
        if(c>='0' && c<='9')
        {
            *digit = c - '0';
        }
        else if(c>='a' && c<'a'+BASE-10)
        {
            *digit = c - 'a' + 10;
        }
        else if(c>='A' && c<'A'+BASE-10)
        {
            *digit = c - 'A' + 10;
        }
        else
        {
            return false;
        }
        return true;
    }
};

template<int BASE, typename CHAR> bool CharToDigit(CHAR c, uint8* digit)
{
    return BaseCharToDigit<CHAR, BASE, BASE <= 10>::Convert(c, digit);
}

// 在string_util.h中有一个针对wchars的IsWhitespace, 但这个是局部独立的,如果替换
// 将会局部依赖. 虽然是待定的, 但是暂时不要尝试引入.
template<typename CHAR> class WhitespaceHelper {};

template<> class WhitespaceHelper<char>
{
public:
    static bool Invoke(char c)
    {
        return 0 != isspace(static_cast<unsigned char>(c));
    }
};

template<> class WhitespaceHelper<char16>
{
public:
    static bool Invoke(char16 c)
    {
        return 0 != iswspace(c);
    }
};

template<typename CHAR> bool LocalIsWhitespace(CHAR c)
{
    return WhitespaceHelper<CHAR>::Invoke(c);
}

// IteratorRangeToNumberTraits should provide:
//  - a typedef for iterator_type, the iterator type used as input.
//  - a typedef for value_type, the target numeric type.
//  - static functions min, max (returning the minimum and maximum permitted
//    values)
//  - constant kBase, the base in which to interpret the input
template<typename IteratorRangeToNumberTraits>
class IteratorRangeToNumber
{
public:
    typedef IteratorRangeToNumberTraits traits;
    typedef typename traits::iterator_type const_iterator;
    typedef typename traits::value_type value_type;

    // Generalized iterator-range-to-number conversion.
    static bool Invoke(const_iterator begin,
                       const_iterator end,
                       value_type* output)
    {
        bool valid = true;

        while(begin!=end && LocalIsWhitespace(*begin))
        {
            valid = false;
            ++begin;
        }

        if(begin!=end && *begin=='-')
        {
            if(!Negative::Invoke(begin + 1, end, output))
            {
                valid = false;
            }
        }
        else
        {
            if(begin!=end && *begin=='+')
            {
                ++begin;
            }
            if(!Positive::Invoke(begin, end, output))
            {
                valid = false;
            }
        }

        return valid;
    }

private:
    // Sign provides:
    //  - a static function, CheckBounds, that determines whether the next digit
    //    causes an overflow/underflow
    //  - a static function, Increment, that appends the next digit appropriately
    //    according to the sign of the number being parsed.
    template<typename Sign>
    class Base
    {
    public:
        static bool Invoke(const_iterator begin, const_iterator end,
                           typename traits::value_type* output)
        {
            *output = 0;

            if(begin == end)
            {
                return false;
            }

            // Note: no performance difference was found when using template
            // specialization to remove this check in bases other than 16
            if(traits::kBase==16 && end-begin>=2 && *begin=='0' &&
                    (*(begin+1)=='x' || *(begin+1)=='X'))
            {
                begin += 2;
            }

            for(const_iterator current=begin; current!=end; ++current)
            {
                uint8 new_digit = 0;

                if(!CharToDigit<traits::kBase>(*current, &new_digit))
                {
                    return false;
                }

                if(current != begin)
                {
                    if(!Sign::CheckBounds(output, new_digit))
                    {
                        return false;
                    }
                    *output *= traits::kBase;
                }

                Sign::Increment(new_digit, output);
            }
            return true;
        }
    };

    class Positive : public Base<Positive>
    {
    public:
        static bool CheckBounds(value_type* output, uint8 new_digit)
        {
            if(*output>static_cast<value_type>(traits::max()/traits::kBase) ||
                    (*output==static_cast<value_type>(traits::max()/traits::kBase) &&
                     new_digit>traits::max()%traits::kBase))
            {
                *output = traits::max();
                return false;
            }
            return true;
        }
        static void Increment(uint8 increment, value_type* output)
        {
            *output += increment;
        }
    };

    class Negative : public Base<Negative>
    {
    public:
        static bool CheckBounds(value_type* output, uint8 new_digit)
        {
            if(*output<traits::min()/traits::kBase ||
                    (*output==traits::min()/traits::kBase &&
                     new_digit>0-traits::min()%traits::kBase))
            {
                *output = traits::min();
                return false;
            }
            return true;
        }
        static void Increment(uint8 increment, value_type* output)
        {
            *output -= increment;
        }
    };
};

template<typename ITERATOR, typename VALUE, int BASE>
class BaseIteratorRangeToNumberTraits
{
public:
    typedef ITERATOR iterator_type;
    typedef VALUE value_type;
    static value_type min()
    {
        return std::numeric_limits<value_type>::min();
    }
    static value_type max()
    {
        return std::numeric_limits<value_type>::max();
    }
    static const int kBase = BASE;
};

typedef BaseIteratorRangeToNumberTraits<std::string::const_iterator, int, 10>
IteratorRangeToIntTraits;
typedef BaseIteratorRangeToNumberTraits<string16::const_iterator, int, 10>
WideIteratorRangeToIntTraits;
typedef BaseIteratorRangeToNumberTraits<std::string::const_iterator, int64, 10>
IteratorRangeToInt64Traits;
typedef BaseIteratorRangeToNumberTraits<string16::const_iterator, int64, 10>
WideIteratorRangeToInt64Traits;

typedef BaseIteratorRangeToNumberTraits<const char*, int, 10>
CharBufferToIntTraits;
typedef BaseIteratorRangeToNumberTraits<const char16*, int, 10>
WideCharBufferToIntTraits;
typedef BaseIteratorRangeToNumberTraits<const char*, int64, 10>
CharBufferToInt64Traits;
typedef BaseIteratorRangeToNumberTraits<const char16*, int64, 10>
WideCharBufferToInt64Traits;

template<typename ITERATOR>
class BaseHexIteratorRangeToIntTraits
    : public BaseIteratorRangeToNumberTraits<ITERATOR, int, 16>
{
public:
    // Allow parsing of 0xFFFFFFFF, which is technically an overflow
    static unsigned int max()
    {
        return std::numeric_limits<unsigned int>::max();
    }
};

typedef BaseHexIteratorRangeToIntTraits<std::string::const_iterator>
HexIteratorRangeToIntTraits;
typedef BaseHexIteratorRangeToIntTraits<const char*>
HexCharBufferToIntTraits;

template<typename STR>
bool HexStringToBytesT(const STR& input, std::vector<uint8>* output)
{
    DCHECK_EQ(output->size(), 0u);
    size_t count = input.size();
    if(count==0 || (count%2)!=0)
    {
        return false;
    }
    for(uintptr_t i=0; i<count/2; ++i)
    {
        uint8 msb = 0; // most significant 4 bits
        uint8 lsb = 0; // least significant 4 bits
        if(!CharToDigit<16>(input[i*2], &msb) ||
                !CharToDigit<16>(input[i*2+1], &lsb))
        {
            return false;
        }
        output->push_back((msb << 4) | lsb);
    }
    return true;
}

std::string IntToString(int value)
{
    return IntToStringT<std::string, int, unsigned int, true>::
           IntToString(value);
}

string16 IntToString16(int value)
{
    return IntToStringT<string16, int, unsigned int, true>::
           IntToString(value);
}

std::string UintToString(unsigned int value)
{
    return IntToStringT<std::string, unsigned int, unsigned int, false>::
           IntToString(value);
}

string16 UintToString16(unsigned int value)
{
    return IntToStringT<string16, unsigned int, unsigned int, false>::
           IntToString(value);
}

std::string Int64ToString(int64 value)
{
    return IntToStringT<std::string, int64, uint64, true>::
           IntToString(value);
}

string16 Int64ToString16(int64 value)
{
    return IntToStringT<string16, int64, uint64, true>::IntToString(value);
}

std::string Uint64ToString(uint64 value)
{
    return IntToStringT<std::string, uint64, uint64, false>::
           IntToString(value);
}

string16 Uint64ToString16(uint64 value)
{
    return IntToStringT<string16, uint64, uint64, false>::
           IntToString(value);
}

std::string DoubleToString(double value)
{
    // 32字节的缓存区足够.
    char buffer[32];
    sprintf_s(buffer, 32, "%f", value);
    return std::string(buffer);
}

bool StringToInt(const std::string& input, int* output)
{
    return IteratorRangeToNumber<IteratorRangeToIntTraits>::Invoke(input.begin(),
            input.end(), output);
}

bool StringToInt(std::string::const_iterator begin,
                 std::string::const_iterator end,
                 int* output)
{
    return IteratorRangeToNumber<IteratorRangeToIntTraits>::Invoke(begin,
            end, output);
}

bool StringToInt(const char* begin, const char* end, int* output)
{
    return IteratorRangeToNumber<CharBufferToIntTraits>::Invoke(begin,
            end, output);
}

bool StringToInt(const string16& input, int* output)
{
    return IteratorRangeToNumber<WideIteratorRangeToIntTraits>::Invoke(
               input.begin(), input.end(), output);
}

bool StringToInt(string16::const_iterator begin,
                 string16::const_iterator end,
                 int* output)
{
    return IteratorRangeToNumber<WideIteratorRangeToIntTraits>::Invoke(begin,
            end, output);
}

bool StringToInt(const char16* begin, const char16* end, int* output)
{
    return IteratorRangeToNumber<WideCharBufferToIntTraits>::Invoke(begin,
            end, output);
}

bool StringToInt64(const std::string& input, int64* output)
{
    return IteratorRangeToNumber<IteratorRangeToInt64Traits>::Invoke(
               input.begin(), input.end(), output);
}

bool StringToInt64(std::string::const_iterator begin,
                   std::string::const_iterator end,
                   int64* output)
{
    return IteratorRangeToNumber<IteratorRangeToInt64Traits>::Invoke(begin,
            end, output);
}

bool StringToInt64(const char* begin, const char* end, int64* output)
{
    return IteratorRangeToNumber<CharBufferToInt64Traits>::Invoke(begin,
            end, output);
}

bool StringToInt64(const string16& input, int64* output)
{
    return IteratorRangeToNumber<WideIteratorRangeToInt64Traits>::Invoke(
               input.begin(), input.end(), output);
}

bool StringToInt64(string16::const_iterator begin,
                   string16::const_iterator end,
                   int64* output)
{
    return IteratorRangeToNumber<WideIteratorRangeToInt64Traits>::Invoke(begin,
            end, output);
}

bool StringToInt64(const char16* begin, const char16* end, int64* output)
{
    return IteratorRangeToNumber<WideCharBufferToInt64Traits>::Invoke(begin,
            end, output);
}

bool StringToDouble(const std::string& input, double* output)
{
    errno = 0; // Thread-safe?  It is on at least Mac, Linux, and Windows.
    char* endptr = NULL;
    *output = strtod(input.c_str(), &endptr);

    // Cases to return false:
    //  - If errno is ERANGE, there was an overflow or underflow.
    //  - If the input string is empty, there was nothing to parse.
    //  - If endptr does not point to the end of the string, there are either
    //    characters remaining in the string after a parsed number, or the string
    //    does not begin with a parseable number.  endptr is compared to the
    //    expected end given the string's stated length to correctly catch cases
    //    where the string contains embedded NUL characters.
    //  - If the first character is a space, there was leading whitespace
    return errno==0 && !input.empty() &&
           input.c_str()+input.length()==endptr && !isspace(input[0]);
}

// Note: if you need to add String16ToDouble, first ask yourself if it's
// really necessary. If it is, probably the best implementation here is to
// convert to 8-bit and then use the 8-bit version.

// Note: if you need to add an iterator range version of StringToDouble, first
// ask yourself if it's really necessary. If it is, probably the best
// implementation here is to instantiate a string and use the string version.

std::string HexEncode(const void* bytes, size_t size)
{
    static const char kHexChars[] = "0123456789ABCDEF";

    // 每字节输出2个十六进制字符.
    std::string ret(size * 2, '\0');

    for(size_t i=0; i<size; ++i)
    {
        char b = reinterpret_cast<const char*>(bytes)[i];
        ret[(i*2)] = kHexChars[(b>>4)&0xf];
        ret[(i*2)+1] = kHexChars[b&0xf];
    }
    return ret;
}

bool HexStringToInt(const std::string& input, int* output)
{
    return IteratorRangeToNumber<HexIteratorRangeToIntTraits>::Invoke(
               input.begin(), input.end(), output);
}

bool HexStringToInt(std::string::const_iterator begin,
                    std::string::const_iterator end,
                    int* output)
{
    return IteratorRangeToNumber<HexIteratorRangeToIntTraits>::Invoke(begin,
            end, output);
}

bool HexStringToInt(const char* begin, const char* end, int* output)
{
    return IteratorRangeToNumber<HexCharBufferToIntTraits>::Invoke(begin,
            end, output);
}

bool HexStringToBytes(const std::string& input, std::vector<uint8>* output)
{
    return HexStringToBytesT(input, output);
}

} //namespace base