#include "AddrLookup.h"
#include "MockSocketCore.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::AtLeast;
using ::testing::Return;
using ::testing::NotNull;
using ::testing::_;
using ::testing::SetArgPointee;
using ::testing::DoAll;

TEST(AddrLookup, badhost)
{
    const char *name = "badhost";
    in_addr_t addr {};
    MockSocketCore core;
    sockets::AddrLookup<MockSocketCore> lookup(core);
    EXPECT_CALL(core, GetAddrInfo(name,_, NotNull(),NotNull())).WillOnce(Return(-1));
    EXPECT_EQ(-1, lookup.lookupHost(name,addr));
}

TEST(AddrLookup, localhost)
{
    const char *name = "localhost";
    in_addr_t addr { };
    in_addr_t expected { 0x100007f };
    MockSocketCore core;
    struct addrinfo res;
    struct sockaddr theAddr = { 0, "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    sockets::AddrLookup<MockSocketCore> lookup(core);
    EXPECT_CALL(core, GetAddrInfo(name,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_EQ(0, lookup.lookupHost(name,addr));
    EXPECT_EQ(expected,addr);
}

TEST(AddrLookup, ipv4addr)
{
    const char *name = "127.0.0.1";
    in_addr_t addr { };
    in_addr_t expected { 0x100007f };
    MockSocketCore core;
    struct addrinfo res;
    struct sockaddr theAddr = { 0, "\000\000\177\000\000\001" };
    res.ai_addr = &theAddr;
    sockets::AddrLookup<MockSocketCore> lookup(core);
    EXPECT_CALL(core, GetAddrInfo(name,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(&res), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_EQ(0, lookup.lookupHost(name,addr));
    EXPECT_EQ(expected,addr);
}

TEST(AddrLookup, badresult)
{
    const char *name = "127.0.0.1";
    in_addr_t addr { };
    MockSocketCore core;
    sockets::AddrLookup<MockSocketCore> lookup(core);
    EXPECT_CALL(core, GetAddrInfo(name,_, NotNull(),_)).WillOnce(DoAll(SetArgPointee<3>(nullptr), Return(0)));
    EXPECT_CALL(core, FreeAddrInfo(_));
    EXPECT_EQ(-1, lookup.lookupHost(name,addr));
}
