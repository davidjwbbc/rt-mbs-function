#ifndef _MBSF_MULTIPART_MIME_HH_
#define _MBSF_MULTIPART_MIME_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MultipartMime class
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

#include <optional>
#include <string>
#include <vector>

#include "common.hh"
#include <DocrootHTTPRequestHandler.hh>

MBSF_NAMESPACE_START

class MultipartMime {
public:
    enum MultipartType {
        MIXED,
        RELATED,
        ALTERNATIVE
    };

    MultipartMime(MultipartType typ = MIXED);

    MultipartMime(const MultipartMime &other) = delete;
    MultipartMime(MultipartMime &&other) = delete;

    virtual ~MultipartMime() {};

    MultipartMime &operator=(const MultipartMime &other) = delete;
    MultipartMime &operator=(MultipartMime &&other) = delete;

    void addFile(const std::filesystem::path &rootdir, const std::filesystem::path &filename, const std::optional<std::string> &disposition_type = std::nullopt);

    const std::vector<char> &body() const { return m_body; };
    const std::map<std::string, std::string> &headers() const { return m_headers; };

private:
    void __insertSep();
    void __insertFooterSep();
    void __removeFooterSep();

    std::map<std::string, std::string> m_headers;
    std::vector<char> m_body;
    std::string m_separator;
    std::vector<char>::iterator m_bodyFooterSepPos;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_MULTIPART_MIME_HH_ */
