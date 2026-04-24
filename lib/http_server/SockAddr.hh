#ifndef _HTTPXPP_SOCK_ADDR_HH_
#define _HTTPXPP_SOCK_ADDR_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: HTTPx Server: SockAddr class
 ******************************************************************************
 * Copyright: (C)2026 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 * License: 5G-MAG Public License v1
 *
 * Licensed under the License terms and conditions for use, reproduction, and
 * distribution of 5G-MAG software (the “License”).  You may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 * https://www.5g-mag.com/reference-tools.  Unless required by applicable law or
 * agreed to in writing, software distributed under the License is distributed on
 * an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied.
 *
 * See the License for the specific language governing permissions and limitations
 * under the License.
 */

#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>

#include <format>
#include <memory>

#include "common.hh"

HTTPXPP_NAMESPACE_START

class SockAddr {
public:
    SockAddr();
    SockAddr(int family, const std::string &hostname, in_port_t port);
    SockAddr(const SockAddr &other);
    SockAddr(const struct sockaddr &sockaddr);
    SockAddr(const struct sockaddr_in &sockaddr);
    SockAddr(const struct sockaddr_in6 &sockaddr);

    SockAddr(SockAddr &&other);
    SockAddr(struct sockaddr *sockaddr);
    SockAddr(struct sockaddr_in *sockaddr);
    SockAddr(struct sockaddr_in6 *sockaddr);

    virtual ~SockAddr() {};

    SockAddr &operator=(const SockAddr &other);
    SockAddr &operator=(SockAddr &&other);
    SockAddr &operator=(const struct sockaddr &sockaddr);
    SockAddr &operator=(const struct sockaddr_in &sockaddr);
    SockAddr &operator=(const struct sockaddr_in6 &sockaddr);

    bool operator==(const SockAddr &other) const;
    bool operator==(const struct sockaddr &other) const;
    bool operator==(const struct sockaddr_in &other) const;
    bool operator==(const struct sockaddr_in6 &other) const;

    operator const struct sockaddr*() const;
    operator const struct sockaddr_in*() const;
    operator const struct sockaddr_in6*() const;
    operator std::string() const;

    bool isIPv4() const { return m_sockaddr && m_sockaddr->ss_family == AF_INET; };
    bool isIPv6() const { return m_sockaddr && m_sockaddr->ss_family == AF_INET6; };

    int family() const { return m_sockaddr?m_sockaddr->ss_family:AF_UNSPEC; };
    uint16_t port() const;
    const struct in_addr &ipv4Address() const;
    const struct in6_addr &ipv6Address() const;

    std::size_t hash() const;

private:
    std::shared_ptr<sockaddr_storage> m_sockaddr;
};

HTTPXPP_NAMESPACE_STOP

namespace std {
    template <>
    struct formatter<HTTPXPP_NAMESPACE_NAME(SockAddr), char> {
        template <class ParseContext>
        constexpr ParseContext::iterator parse(ParseContext& ctx) {
            return ctx.begin();
        };
        template <class FmtContext>
        FmtContext::iterator format(const HTTPXPP_NAMESPACE_NAME(SockAddr) &sock_addr, FmtContext& ctx) const {
            return std::format_to(ctx.out(), "{}", static_cast<std::string>(sock_addr));
        };
    };

    template <>
    struct hash<HTTPXPP_NAMESPACE_NAME(SockAddr)> {
        std::size_t operator()(const HTTPXPP_NAMESPACE_NAME(SockAddr) &sock_addr) const { return sock_addr.hash(); };
    };
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _HTTPXPP_SOCK_ADDR_HH_ */
