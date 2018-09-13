// test cases from TestInt128.cc, re-edited to run without Google Test 
#include <iostream>
#include "Int128.h"

using namespace std;

namespace int128 {
// declarations of standalone methods from Int128.cc  Why not static?
// Why not declared in Int128.h?
    int64_t fls(uint32_t x);
    void shiftArrayLeft(uint32_t* array, int64_t length, int64_t bits);
    void shiftArrayRight(uint32_t* array, int64_t length, int64_t bits);
    void fixDivisionSigns(Int128 &result, Int128 &remainder,
                          bool dividendWasNegative, bool divisorWasNegative);
    void buildFromArray(Int128& value, uint32_t* array, int64_t length);
    Int128 singleDivide(uint32_t* dividend, int64_t dividendLength,
                        uint32_t divisor, Int128& remainder,
                        bool dividendWasNegative, bool divisorWasNegative);

    void rep(int tnum, bool success)
    {
        if (success)
            cout << "test " << tnum << " passed" << endl;
        else
            cout << "test " << tnum << " FAILED" << endl; 
    }

    void EXPECT_TRUE(int tnum, bool result)
    {
        if (result)
            cout << "test " << tnum << " passed" << endl;
        else
            cout << "test " << tnum << " FAILED" << endl; 
    }

    void EXPECT_FALSE(int tnum, bool result)
    {
        if (!result)
            cout << "test " << tnum << " passed" << endl;
        else
            cout << "test " << tnum << " FAILED" << endl; 
    }

    void mytests()
    {
        Int128 x = 12;
        Int128 y = 13;
        x += y;
        int tnum = 100;
        rep(tnum++, (x.toLong() == 25));
        rep(tnum++, ("0x00000000000000000000000000000019"== x.toHexString()));
        y -= 1;
        rep(tnum++, ("0x0000000000000000000000000000000c"== y.toHexString()));
        rep(tnum++, (12 == y.toLong()));
        rep(tnum++, (0== y.getHighBits()));
        rep(tnum++,(12== y.getLowBits()));
        y -= 20;
        rep(tnum++, ("0xfffffffffffffffffffffffffffffff8"== y.toHexString()));
        rep(tnum++, (-8== y.toLong()));
        rep(tnum++, (-1== y.getHighBits()));
        rep(tnum++, (static_cast<uint64_t>(-8)== y.getLowBits()));
        cout << "static_cast<uint64_t>(-8) == " << hex << static_cast<uint64_t>(-8) << dec
             << " (hex) == " << static_cast<uint64_t>(-8) << endl;
        Int128 z;
        rep(tnum++, (0== z.toLong()));

        tnum = 200; //testLogic
        Int128 n = Int128(0x00000000100000002, 0x0000000400000008);
        n |= Int128(0x0000001000000020, 0x0000004000000080);
        rep(tnum++, ("0x00000011000000220000004400000088"== n.toHexString()));
        n =  Int128(0x0000111100002222, 0x0000333300004444);
        n &= Int128(0x0000f00000000f00, 0x000000f00000000f);
        rep(tnum++, ( "0x00001000000002000000003000000004"== n.toHexString()));


        tnum = 300; // testSingleDivide
        Int128 remainder;
        uint32_t dividend[4];

        dividend[0] = 23;
        Int128 result = singleDivide(dividend, 1, 5, remainder, true, false);
        rep(tnum++, (-4== result.toLong()));
        rep(tnum++, (-3== remainder.toLong()));

        dividend[0] = 0x100;
        dividend[1] = 0x120;
        dividend[2] = 0x140;
        dividend[3] = 0x160;
        result = singleDivide(dividend, 4, 0x20, remainder, false, false);
        rep(tnum++, ("0x00000008000000090000000a0000000b"== result.toHexString()));
        rep(tnum++, (0== remainder.toLong()));

        dividend[0] = 0x101;
        dividend[1] = 0x122;
        dividend[2] = 0x143;
        dividend[3] = 0x164;
        result = singleDivide(dividend, 4, 0x20, remainder, false, false);
        rep(tnum++, ("0x00000008080000091000000a1800000b"== result.toHexString()));
        rep(tnum++, (4== remainder.toLong()));

        dividend[0] = 0x12345678;
        dividend[1] = 0x9abcdeff;
        dividend[2] = 0xfedcba09;
        dividend[3] = 0x87654321;
        result = singleDivide(dividend, 4, 123, remainder, false, false);
        rep(tnum++, ("0x0025e390971c97aaaaa84c7077bc23ed"== result.toHexString()));
        rep(tnum++, (0x42== remainder.toLong()));


        tnum = 400; //testDivide)
        {
            Int128 dividend;
            Int128 result;
            Int128 remainder;

            dividend = 0x12345678;
            result = dividend.divide(0x123456789abcdef0, remainder);
            rep(tnum++, (0 == result.toLong()));
            rep(tnum++, (0x12345678== remainder.toLong())); 

            // EXPECT_THROW(dividend.divide(0, remainder),
            // std::runtime_error);
            try {
                dividend.divide(0, remainder);
                cout << "test " << tnum++ << " FAILED - exception not thrown" << endl;
            }
            catch(std::runtime_error) {
                cout << "test " << tnum++ << " succeeded - exception thrown" << endl;
            }

            dividend = Int128(0x123456789abcdeff, 0xfedcba0987654321);
            result = dividend.divide(123, remainder);
            rep(tnum++, ("0x0025e390971c97aaaaa84c7077bc23ed"== result.toHexString()));
            rep(tnum++, (0x42== remainder.toLong()));

            dividend = Int128(0x111111112fffffff, 0xeeeeeeeedddddddd);
            result = dividend.divide(0x1111111123456789, remainder);
            rep(tnum++, ("0x000000000000000100000000beeeeef7" == result.toHexString()));
            rep(tnum++, ("0x0000000000000000037d3b3d60479aae"== remainder.toHexString()));

            dividend = 1234234662345;
            result = dividend.divide(642337, remainder);
            rep(tnum++, (1921475== result.toLong()));
            rep(tnum++, (175270== remainder.toLong()));

            dividend = Int128(0x42395ADC0534AB4C, 0x59D109ADF9892FCA);
            result = dividend.divide(0x1234F09DC19A, remainder);
            rep(tnum++, ("0x000000000003a327c1348bccd2f06c27"== result.toHexString()));
            rep(tnum++, ("0x000000000000000000000cacef73b954"== remainder.toHexString()));

            dividend = Int128(0xfffffffffffffff, 0xf000000000000000);
            result = dividend.divide(Int128(0, 0x1000000000000000), remainder);
            rep(tnum++, ("0x0000000000000000ffffffffffffffff"== result.toHexString()));
            rep(tnum++, (0== remainder.toLong()));

            dividend = Int128(0x4000000000000000, 0);
            result = dividend.divide(Int128(0, 0x400000007fffffff), remainder);
            rep(tnum++, ("0x0000000000000000fffffffe00000007"== result.toHexString()));
            rep(tnum++, ("0x00000000000000003ffffffa80000007"== remainder.toHexString()));
        }

        tnum = 500;// testToString
        {
            Int128 num = Int128(0x123456789abcdef0, 0xfedcba0987654321);
            rep(tnum++, ("24197857203266734881846307133640229665"== num.toString()));

            num = Int128(0, 0xab54a98ceb1f0ad2);
            rep(tnum++, ("12345678901234567890"== num.toString()));

            num = 12345678;
            rep(tnum++, ("12345678"== num.toString()));

            num = -1234;
            rep(tnum++, ("-1234"== num.toString()));

            num = Int128(0x13f20d9c2, 0xfff89d38e1c70cb1);
            rep(tnum++, ("98765432109876543210987654321"== num.toString()));
            num.negate();
            rep(tnum++, ("-98765432109876543210987654321"== num.toString()));

            num = Int128("10000000000000000000000000000000000000");
            rep(tnum++, ("10000000000000000000000000000000000000" == num.toString()));

            num = Int128("-1234");
            rep(tnum++, ("-1234"== num.toString()));

            num = Int128("-12345678901122334455667788990011122233");
            rep(tnum++, ("-12345678901122334455667788990011122233" == num.toString()));
        }

        tnum = 600; //testToDecimalString)
        {
            Int128 num = Int128("98765432109876543210987654321098765432");
            rep(tnum++, ("98765432109876543210987654321098765432" ==
                         num.toDecimalString(0)));
            rep(tnum++, ("987654321098765432109876543210987.65432" ==
                         num.toDecimalString(5)));
            num.negate();
            rep(tnum++, ("-98765432109876543210987654321098765432"==
                         num.toDecimalString(0)));
            rep(tnum++, ("-987654321098765432109876543210987.65432"==
                         num.toDecimalString(5)));
            num = 123;
            rep(tnum++, ("12.3"== num.toDecimalString(1)));
            rep(tnum++, ("0.123"== num.toDecimalString(3)));
            rep(tnum++, ("0.0123"== num.toDecimalString(4)));
            rep(tnum++, ("0.00123"== num.toDecimalString(5)));

            num = -123;
            rep(tnum++, ("-123"== num.toDecimalString(0)));
            rep(tnum++, ("-12.3"== num.toDecimalString(1)));
            rep(tnum++, ("-0.123" ==num.toDecimalString(3)));
            rep(tnum++, ("-0.0123"== num.toDecimalString(4)));
            rep(tnum++, ("-0.00123"== num.toDecimalString(5)));
        }


        tnum = 700; //testInt128Scale) 
        Int128 num = Int128(10);
        bool overflow = false;

        num = scaleUpInt128ByPowerOfTen(num, 0, overflow);
        EXPECT_FALSE(tnum++,overflow);
        rep(tnum++, (Int128(10)== num));

        num = scaleUpInt128ByPowerOfTen(num, 5, overflow);
        EXPECT_FALSE(tnum++,overflow);
        rep(tnum++, (Int128(1000000)== num));

        num = scaleUpInt128ByPowerOfTen(num, 5, overflow);
        EXPECT_FALSE(tnum++,overflow);
        rep(tnum++, (Int128(100000000000l)== num));

        num = scaleUpInt128ByPowerOfTen(num, 20, overflow);
        EXPECT_FALSE(tnum++,overflow);
        rep(tnum++, (Int128("10000000000000000000000000000000")== num));

        scaleUpInt128ByPowerOfTen(num, 10, overflow);
        EXPECT_TRUE(tnum++,overflow);

        scaleUpInt128ByPowerOfTen(Int128::maximumValue(), 0, overflow);
        EXPECT_FALSE(tnum++,overflow);

        scaleUpInt128ByPowerOfTen(Int128::maximumValue(), 1, overflow);
        EXPECT_TRUE(tnum++,overflow);

        num = scaleDownInt128ByPowerOfTen(Int128(10001), 0);
        rep(tnum++, (Int128(10001)== num));

        num = scaleDownInt128ByPowerOfTen(Int128(10001), 2);
        rep(tnum++, (Int128(100)== num));

        num = scaleDownInt128ByPowerOfTen(Int128(10000), 5);
        rep(tnum++, (Int128(0)== num));
    }


}  // namespace int128

int main_for_unit_test()
{
    int128::mytests();
    return 0;
}
