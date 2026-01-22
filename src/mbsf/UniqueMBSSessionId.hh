#ifndef _MBSF_UNIQUE_MBS_SESSION_ID_HH_
#define _MBSF_UNIQUE_MBS_SESSION_ID_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS: MBS Session Id used for uniqueness checks
 ******************************************************************************
 * Copyright: (C)2025 British Broadcasting Corporation
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

#include "mb-smf-service-consumer.h"

#include <memory>

#include "common.hh"

namespace reftools::mbsf {
    class ExternalMbsServiceArea;
    class MbsServiceArea;
    class MbsSessionId;
}

MBSF_NAMESPACE_START

/** Unique MBS Session Id Class
 *
 * This class combines all the fields that denote the unique identifier for an MBS Session.
 *
 * This combines the MBS Service Area(s) and MBS Session Id along with comparitor rules to test for equality and ordering.
 * MBS Session Id's only need to be unique within the same area.
 */
class UniqueMbsSessionId {
public:
    UniqueMbsSessionId();
    UniqueMbsSessionId(bool tmgi_requested, const std::shared_ptr<reftools::mbsf::MbsSessionId> &mbs_session_id,
                       const std::shared_ptr<reftools::mbsf::MbsServiceArea> &mbs_service_area,
                       const std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> &ext_mbs_service_area);
    UniqueMbsSessionId(UniqueMbsSessionId &&other);
    UniqueMbsSessionId(const UniqueMbsSessionId &other);
    UniqueMbsSessionId &operator=(UniqueMbsSessionId &&other);
    UniqueMbsSessionId &operator=(const UniqueMbsSessionId &other);
    virtual ~UniqueMbsSessionId();

    std::strong_ordering operator<=>(const UniqueMbsSessionId &other) const;
    bool operator==(const UniqueMbsSessionId &other) const { return (*this <=> other) == std::strong_ordering::equal; };
    bool operator!=(const UniqueMbsSessionId &other) const { return (*this <=> other) != std::strong_ordering::equal; };
    bool operator<(const UniqueMbsSessionId &other) const { return (*this <=> other) == std::strong_ordering::less; };
    bool operator>(const UniqueMbsSessionId &other) const { return (*this <=> other) == std::strong_ordering::greater; };
    bool operator<=(const UniqueMbsSessionId &other) const { return (*this <=> other) != std::strong_ordering::greater; };
    bool operator>=(const UniqueMbsSessionId &other) const { return (*this <=> other) != std::strong_ordering::less; };

    std::string repr() const;
    std::size_t hash() const;

    operator bool() const { return !!m_mbsSessionId; };

private:
    bool m_tmgiRequested;
    std::shared_ptr<reftools::mbsf::MbsSessionId> m_mbsSessionId;
    std::shared_ptr<reftools::mbsf::MbsServiceArea> m_mbsServiceArea;
    std::shared_ptr<reftools::mbsf::ExternalMbsServiceArea> m_extMBSServiceArea;
};

MBSF_NAMESPACE_STOP

namespace std {
template <> struct hash<MBSF_NAMESPACE_NAME(UniqueMbsSessionId)> {
    std::size_t operator()(const MBSF_NAMESPACE_NAME(UniqueMbsSessionId) &mbs_session_id) const;
};
}

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_UNIQUE_MBS_SESSION_ID_HH_ */

