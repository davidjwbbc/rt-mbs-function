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

#include <inttypes.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cstring>
#include <format>
#include <memory>
#include <stdexcept>

#include "common.hh"

#include "SockAddr.hh"

HTTPXPP_NAMESPACE_START

SockAddr::SockAddr()
    :m_sockaddr()
{
}

SockAddr::SockAddr(int family, const std::string &hostname, in_port_t port)
    :m_sockaddr()
{
    struct addrinfo *ai = nullptr;
    auto port_name = std::format("{}", port);
    getaddrinfo(hostname.c_str(), port_name.c_str(), nullptr, &ai);

    for (auto *it = ai; it; it = it->ai_next) {
        if (it->ai_family == AF_INET || it->ai_family == AF_INET6) {
            m_sockaddr.reset(new sockaddr_storage);
            memcpy(m_sockaddr.get(), it->ai_addr, it->ai_addrlen);
            break;
        }
    } 

    if (ai) freeaddrinfo(ai);
}

SockAddr::SockAddr(const SockAddr &other)
    :m_sockaddr()
{
    if (other.m_sockaddr) {
        if (other.m_sockaddr->ss_family == AF_INET) {
            m_sockaddr.reset(new sockaddr_storage);
            memcpy(m_sockaddr.get(), other.m_sockaddr.get(), sizeof(sockaddr_in));
        } else if (other.m_sockaddr->ss_family == AF_INET6) {
            m_sockaddr.reset(new sockaddr_storage);
            memcpy(m_sockaddr.get(), other.m_sockaddr.get(), sizeof(sockaddr_in6));
        } else {
            throw std::out_of_range("Address type not handled");
        }
    }
}

SockAddr::SockAddr(const struct sockaddr &sa)
    :m_sockaddr()
{
    if (sa.sa_family == AF_INET) {
        m_sockaddr.reset(new sockaddr_storage);
        memcpy(m_sockaddr.get(), &sa, sizeof(sockaddr_in));
    } else if (sa.sa_family == AF_INET6) {
        m_sockaddr.reset(new sockaddr_storage);
        memcpy(m_sockaddr.get(), &sa, sizeof(sockaddr_in6));
    } else {
        throw std::out_of_range("Address type not handled");
    }
}

SockAddr::SockAddr(const struct sockaddr_in &sin)
    :m_sockaddr(new sockaddr_storage)
{
    memcpy(m_sockaddr.get(), &sin, sizeof(sockaddr_in));
}

SockAddr::SockAddr(const struct sockaddr_in6 &sin6)
    :m_sockaddr(new sockaddr_storage)
{
    memcpy(m_sockaddr.get(), &sin6, sizeof(sockaddr_in6));
}

SockAddr::SockAddr(SockAddr &&other)
    :m_sockaddr(std::move(other.m_sockaddr))
{
}

SockAddr::SockAddr(struct sockaddr *sa)
    :m_sockaddr(reinterpret_cast<sockaddr_storage*>(sa))
{
}

SockAddr::SockAddr(struct sockaddr_in *sin)
    :m_sockaddr(reinterpret_cast<sockaddr_storage*>(sin))
{
}

SockAddr::SockAddr(struct sockaddr_in6 *sin6)
    :m_sockaddr(reinterpret_cast<sockaddr_storage*>(sin6))
{
}

SockAddr &SockAddr::operator=(const SockAddr &other)
{
    if (!other.m_sockaddr) {
        m_sockaddr.reset();
    } else if (other.m_sockaddr->ss_family == AF_INET) {
        m_sockaddr.reset(new sockaddr_storage);
        memcpy(m_sockaddr.get(), other.m_sockaddr.get(), sizeof(sockaddr_in));
    } else if (other.m_sockaddr->ss_family == AF_INET6) {
        m_sockaddr.reset(new sockaddr_storage);
        memcpy(m_sockaddr.get(), other.m_sockaddr.get(), sizeof(sockaddr_in6));
    } else {
        throw std::out_of_range("Address type not handled");
    }
    return *this;
}

SockAddr &SockAddr::operator=(SockAddr &&other)
{
    m_sockaddr = std::move(other.m_sockaddr);
    return *this;
}

SockAddr &SockAddr::operator=(const struct sockaddr &sa)
{
    if (sa.sa_family == AF_INET) {
        m_sockaddr.reset(new sockaddr_storage);
        memcpy(m_sockaddr.get(), &sa, sizeof(sockaddr_in));
    } else if (sa.sa_family == AF_INET6) {
        m_sockaddr.reset(new sockaddr_storage);
        memcpy(m_sockaddr.get(), &sa, sizeof(sockaddr_in6));
    } else {
        throw std::out_of_range("Address type not handled");
    }
    return *this;
}

SockAddr &SockAddr::operator=(const struct sockaddr_in &sin)
{
    m_sockaddr.reset(new sockaddr_storage);
    memcpy(m_sockaddr.get(), &sin, sizeof(sockaddr_in));
    return *this;
}

SockAddr &SockAddr::operator=(const struct sockaddr_in6 &sin6)
{
    m_sockaddr.reset(new sockaddr_storage);
    memcpy(m_sockaddr.get(), &sin6, sizeof(sockaddr_in6));
    return *this;
}

bool SockAddr::operator==(const SockAddr &other) const
{
    if (m_sockaddr == other.m_sockaddr) return true;
    if (!m_sockaddr || !other.m_sockaddr) return false;
    if (m_sockaddr->ss_family != other.m_sockaddr->ss_family) return false;
    if (m_sockaddr->ss_family == AF_INET) {
        return memcmp(m_sockaddr.get(), other.m_sockaddr.get(), sizeof(sockaddr_in)) == 0;
    } else if (m_sockaddr->ss_family == AF_INET6) {
        return memcmp(m_sockaddr.get(), other.m_sockaddr.get(), sizeof(sockaddr_in6)) == 0;
    }

    throw std::out_of_range("Address type not handled");
}

bool SockAddr::operator==(const struct sockaddr &other) const
{
    if (!m_sockaddr) return false;
    if (m_sockaddr->ss_family != other.sa_family) return false;
    if (m_sockaddr->ss_family == AF_INET) {
        return memcmp(m_sockaddr.get(), &other, sizeof(sockaddr_in)) == 0;
    } else if (m_sockaddr->ss_family == AF_INET6) {
        return memcmp(m_sockaddr.get(), &other, sizeof(sockaddr_in6)) == 0;
    }

    throw std::out_of_range("Address type not handled");
}

bool SockAddr::operator==(const struct sockaddr_in &other) const
{
    if (!m_sockaddr) return false;
    if (m_sockaddr->ss_family != AF_INET) return false;
    return memcmp(m_sockaddr.get(), &other, sizeof(sockaddr_in)) == 0;
}

bool SockAddr::operator==(const struct sockaddr_in6 &other) const
{
    if (!m_sockaddr) return false;
    if (m_sockaddr->ss_family != AF_INET6) return false;
    return memcmp(m_sockaddr.get(), &other, sizeof(sockaddr_in6)) == 0;
}

SockAddr::operator const struct sockaddr*() const
{
    return reinterpret_cast<const struct sockaddr*>(m_sockaddr.get());
}

SockAddr::operator const struct sockaddr_in*() const
{
    if (!m_sockaddr || m_sockaddr->ss_family != AF_INET) {
        throw std::out_of_range("Cast to sockaddr_in failed, wrong address family");
    }
    return reinterpret_cast<const struct sockaddr_in*>(m_sockaddr.get());
}

SockAddr::operator const struct sockaddr_in6*() const
{
    if (!m_sockaddr || m_sockaddr->ss_family != AF_INET6) {
        throw std::out_of_range("Cast to sockaddr_in6 failed, wrong address family");
    }
    return reinterpret_cast<const struct sockaddr_in6*>(m_sockaddr.get());
}

uint16_t SockAddr::port() const
{
    if (!m_sockaddr) {
        throw std::out_of_range("Unable to get port number from empty address");
    } else if (m_sockaddr->ss_family == AF_INET) {
        return std::reinterpret_pointer_cast<sockaddr_in>(m_sockaddr)->sin_port;
    } else if (m_sockaddr->ss_family == AF_INET6) {
        return std::reinterpret_pointer_cast<sockaddr_in6>(m_sockaddr)->sin6_port;
    }
    throw std::out_of_range("Unable to get port number from unhandled address type");
}

const struct in_addr &SockAddr::ipv4Address() const
{
    if (m_sockaddr && m_sockaddr->ss_family == AF_INET) {
        return std::reinterpret_pointer_cast<sockaddr_in>(m_sockaddr)->sin_addr;
    }
    throw std::out_of_range("Address is not IPv4");
}

const struct in6_addr &SockAddr::ipv6Address() const
{
    if (m_sockaddr && m_sockaddr->ss_family == AF_INET6) {
        return std::reinterpret_pointer_cast<sockaddr_in6>(m_sockaddr)->sin6_addr;
    }
    throw std::out_of_range("Address is not IPv6");
}

SockAddr::operator std::string() const
{
    char ipaddr[INET6_ADDRSTRLEN];
    in_port_t port;
    if (family() == AF_INET) {
        auto *ipv4 = reinterpret_cast<const struct sockaddr_in*>(m_sockaddr.get());
        inet_ntop(AF_INET, &ipv4->sin_addr, ipaddr, sizeof(ipaddr));
        port = ntohs(ipv4->sin_port);
        return std::format("{}:{}", ipaddr, port);
    } else if (family() == AF_INET6) {
        auto *ipv6 = reinterpret_cast<const struct sockaddr_in6*>(m_sockaddr.get());
        inet_ntop(AF_INET6, &ipv6->sin6_addr, ipaddr, sizeof(ipaddr));
        port = ntohs(ipv6->sin6_port);
        return std::format("[{}]:{}", ipaddr, port);
    }
    throw std::out_of_range("SockAddr can only convert IPv4 and IPv6 addresses");
}

std::size_t SockAddr::hash() const
{
    const char *ptr = reinterpret_cast<const char*>(m_sockaddr.get());
    if (family() == AF_INET) {
        return std::hash<std::string_view>{}(std::string_view(ptr, ptr+sizeof(struct sockaddr_in)));
    } else if (family() == AF_INET6) {
        return std::hash<std::string_view>{}(std::string_view(ptr, ptr+sizeof(struct sockaddr_in6)));
    }
    return std::hash<std::string_view>{}(std::string_view(ptr, ptr+sizeof(struct sockaddr_storage)));
}

HTTPXPP_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
