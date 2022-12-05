#include "gtest/gtest.h"

#include "AddrLookup.h"

TEST(AddrLookup, localhost)
{
    in_addr_t addr {};
    EXPECT_EQ(0, sockets::lookupHost("localhost",addr));
}

TEST(AddrLookup, ipv4addr)
{
    in_addr_t addr {};
    EXPECT_EQ(0, sockets::lookupHost("127.0.0.1",addr));
}

TEST(AddrLookup, ipv6addr)
{
    in_addr_t addr {};
    EXPECT_EQ(-1, sockets::lookupHost("fc00::6dc4:3ef1:351f:7a3",addr));
}

TEST(AddrLookup, invalidname)
{
    in_addr_t addr {};
    EXPECT_EQ(-1, sockets::lookupHost("noname",addr));

}

TEST(AddrLookup, localhost6)
{
    in_addr_t addr {};
    EXPECT_EQ(0, sockets::lookupHost("localhost6",addr));
}