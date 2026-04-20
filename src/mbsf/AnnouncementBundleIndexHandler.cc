/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: AnnouncementBundleIndexHandler class
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

#include <dirent.h>

#include <filesystem>
#include <list>
#include <string>
#include <vector>

#include "ogs-core.h"
#include "ogs-sbi.h"

#include "common.hh"
#include <DocrootHTTPRequestHandler.hh>
#include <HTTPResponse.hh>
#include "MultipartMime.hh"

#include "AnnouncementBundleIndexHandler.hh"

HTTPXPP_NAMESPACE_USING(DocrootHTTPRequestHandler);
HTTPXPP_NAMESPACE_USING(HTTPResponse);

MBSF_NAMESPACE_START

static std::list<std::filesystem::path> get_bundle_filenames(const std::filesystem::path &dir_path /* const std::string &user_data_ing_sess_id*/);

HTTPResponse AnnouncementBundleIndexHandler::makeResponseForDir(const std::string &dir_path, const std::string &url_path)
{
    if (url_path == "/" || url_path.starts_with("/.")) {
        return HTTPResponse().statusCode(403);
    }

    std::filesystem::path dp(dir_path);
    if (!dp.has_filename()) dp = dp.parent_path();
    auto user_data_ing_sess_id = dp.filename().string();

    auto bundle_files = get_bundle_filenames(dp /*user_data_ing_sess_id*/);
    MultipartMime bundle(MultipartMime::RELATED);

    /* bool disposition_inline = true; */
    for (const auto &filename : bundle_files) {
        bundle.addFile(dp, filename /*, disposition_inline?"inline":"attachment"*/);
        /* disposition_inline = false; */
    }

    HTTPResponse resp(bundle.body());
    for (const auto &hdr : bundle.headers()) {
        resp.addHeader(hdr.first, hdr.second);
    }

    return resp;
}

static std::list<std::filesystem::path> get_bundle_filenames(const std::filesystem::path &dir_path)
{
    std::list<std::filesystem::path> result;

    auto *dirp = opendir(dir_path.string().c_str());
    if (dirp) {
        for (auto *entry = readdir(dirp); entry; entry = readdir(dirp)) {
            if (entry->d_type == DT_REG) {           // Only include regular files in this dir
                std::string name(entry->d_name);
                if (name.starts_with('.')) continue; // ignore hidden files
                if (name == "announcement.json") {
                    result.emplace_front(name);
                } else {
                    result.emplace_back(name);
                }
            }
        }
        closedir(dirp);
    }

    return result;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
