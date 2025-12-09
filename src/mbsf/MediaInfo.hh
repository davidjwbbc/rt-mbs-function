#ifndef _MBSF_MEDIA_INFO_HH_
#define _MBSF_MEDIA_INFO_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Service Info class
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
 * Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
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

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include <memory>
#include <tuple>
#include <mutex>
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/MbsServiceInfo.h"
#include "openapi/model/MbsMediaInfo.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class MBSDistributionSessionInfo;
    class MbsServiceInfo;
    class MbsMediaInfo;
}

using fiveg_mag_reftools::CJson;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::Ssm;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::PacketDistrMethInfo;
using reftools::mbsf::PktDistributionOperatingMode;
using reftools::mbsf::PktIngestMethod;
using reftools::mbsf::MbStfIngestAddr;
using reftools::mbsf::MbsMediaInfo;
using reftools::mbsf::MediaType;

MBSF_NAMESPACE_START

class MBSMFMBSSession;

class MediaInfo {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;

    MediaInfo(CJson &json, bool as_request);
    MediaInfo(const std::shared_ptr<MbsMediaInfo> &mbs_media_info);
    MediaInfo() = delete;
    MediaInfo(MediaInfo &&other) = delete;
    MediaInfo(const MediaInfo &other) = delete;
    MediaInfo &operator=(MediaInfo &&other) = delete;
    MediaInfo &operator=(const MediaInfo &other) = delete;

    virtual ~MediaInfo();

    CJson json(bool as_request) const;

    const std::shared_ptr<MbsMediaInfo> &getMbsMediaInfo() const {return m_mbsMediaInfo;};
    const std::optional<std::shared_ptr< MediaType > > &getMediaType() const {return m_mbsMediaInfo->getMbsMedType();};
    const std::optional<std::string > &getMaximumReqBandwidthDownlink() const {return m_mbsMediaInfo->getMaxReqMbsBwDl();};
    const std::optional<std::string > &getMinimumReqBandwidthDownlink() const {return m_mbsMediaInfo->getMinReqMbsBwDl();};
    void codecs(mb_smf_sc_mbs_media_info_t *media_info);

    mb_smf_sc_mbs_media_info_t *populateMediaInfo();
    mb_smf_sc_mbs_media_type_e lookup();
    uint64_t *bandwidth(const std::optional<std::string > &bandwidth);

private:
    std::shared_ptr<MbsMediaInfo> m_mbsMediaInfo;
    static const std::unordered_map<std::string, mb_smf_sc_mbs_media_type_e>& mediaType();


};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_MEDIA_INFO_HH_ */
