#include <stdexcept>
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
    EXPECT_THROW(BigInt("abcd"), std::invalid_argument);
    EXPECT_THROW(BigInt("0x42"), std::invalid_argument);
    EXPECT_THROW(BigInt("123456789012345678901234567890x"), std::invalid_argument);
}
