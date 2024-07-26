#include <stdexcept>
#include <utility>
#include <gtest/gtest.h>
#include "BigInt.h"

TEST(BigIntDefaultCtor, IsZero)
{
    EXPECT_TRUE(BigInt() == BigInt(0));
    EXPECT_TRUE(BigInt() == BigInt(-5) + BigInt(5));
    EXPECT_TRUE(BigInt() == BigInt("0"));
}

TEST(BigIntIntCtor, HandlesNegativeNumbers)
{
    EXPECT_TRUE(BigInt(-1'423'786'792) == BigInt(-1'423'786'834) + BigInt(42));
    EXPECT_TRUE(BigInt(-42) == BigInt(42) - BigInt(84));
}

TEST(BigIntIntCtor, HandlesLargeNumbers)
{
    EXPECT_TRUE(BigInt(930'350'724'101'083'004) == BigInt(930'350'724) * BigInt(1'000'000'000) + BigInt(101'083'004));
}

TEST(BigIntStringCtor, HandlesNegativeNumbers)
{
    EXPECT_TRUE(BigInt("-1423786792") == BigInt("-1423786834") + BigInt("42"));
    EXPECT_TRUE(BigInt("-42") == BigInt("42") - BigInt("84"));
}

TEST(BigIntStringCtor, HandlesLargeNumbers)
{
    EXPECT_TRUE(BigInt("930350724101083004") == BigInt("930350724") * BigInt("1000000000") + BigInt("101083004"));
}

TEST(BigIntStringCtor, ThrowsExceptionOnInvalidArgument)
{
    EXPECT_THROW(BigInt(""), std::invalid_argument);
    EXPECT_THROW(BigInt("-"), std::invalid_argument);
    EXPECT_THROW(BigInt("foo"), std::invalid_argument);
    EXPECT_THROW(BigInt("0x42"), std::invalid_argument);
    EXPECT_THROW(BigInt("123456789012345678901234567890x"), std::invalid_argument);
}

TEST(BigIntAddOps, AddAssignWorks)
{
    // if using the same object for the two arguments the rhs gets copy constructed as a temporary
    BigInt acc1("75755724578284142547987951683356371041");
    acc1 += acc1;
    EXPECT_TRUE(acc1 == BigInt("151511449156568285095975903366712742082"));
    // carry past lim is pop_back'ed with negative rhs
    acc1 += BigInt("-151511449156568285095975903366712742082");
    EXPECT_TRUE(acc1 == BigInt(0));
    // acc2 has this replaced with acc1 as it is an rvalue and has higher capacity
    BigInt acc2(42);
    acc2 += std::move(acc1);
    EXPECT_TRUE(acc2 == BigInt(42));
    // flip to negative
    acc2 += BigInt(-43);
    EXPECT_TRUE(acc2 == BigInt(-1));
    // if it doesn't go positive internally add an additional -1 at the next chunk and set negative bit back
    BigInt acc3(-4293984256);
    acc3 += BigInt(-4279238656);
    EXPECT_TRUE(acc3 == BigInt(-8573222912));
    // repeat addChunk from small negative rhs
    BigInt acc4("275872115513967702182504904667760619899");
    acc4 += BigInt(-1);
    EXPECT_TRUE(acc4 == BigInt("275872115513967702182504904667760619898"));
}

TEST(BigIntAddOps, InfixAddWorks)
{
    // left bigger than right
    BigInt lhs(8761890128033252764);
    BigInt rhs(649602511);
    EXPECT_TRUE(lhs + rhs == BigInt(8761890128682855275));
    // right bigger than left
    lhs = BigInt(2811295173);
    rhs = BigInt(754751497079698868);
    EXPECT_TRUE(lhs + rhs == BigInt(754751499890994041));
    // right moved
    lhs = BigInt(4402506586766798590);
    EXPECT_TRUE(lhs + BigInt(1150734779629110894) == BigInt(5553241366395909484));
    // left moved
    rhs = BigInt(4398547354252609520);
    EXPECT_TRUE(BigInt(4140871994740157499) + rhs == BigInt(8539419348992767019));
    // both moved left bigger
    EXPECT_TRUE(BigInt(8669068799261902808) + BigInt(2084842186) == BigInt(8669068801346744994));
    // both moved right bigger
    EXPECT_TRUE(BigInt(1864966085) + BigInt(2326226595802122250) == BigInt(2326226597667088335));
}
