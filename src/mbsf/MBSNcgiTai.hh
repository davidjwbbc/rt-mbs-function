#ifndef _MBSF_NCGI_TAI_HH_
#define _MBSF_NCGI_TAI_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS NCGI TAI class
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
#include "openapi/model/NcgiTai.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class NcgiTai;
    class PlmnId;
}

using fiveg_mag_reftools::CJson;
using reftools::mbsf::NcgiTai;
using reftools::mbsf::Ncgi;
using reftools::mbsf::Tai;

MBSF_NAMESPACE_START

class MBSMFMBSSession;

class MBSNcgiTai {
public:

    MBSNcgiTai(CJson &json, bool as_request);
    MBSNcgiTai(const std::shared_ptr<NcgiTai> &ncgi_tai);
    MBSNcgiTai() = delete;
    MBSNcgiTai(MBSNcgiTai &&other) = delete;
    MBSNcgiTai(const MBSNcgiTai &other) = delete;
    MBSNcgiTai &operator=(MBSNcgiTai &&other) = delete;
    MBSNcgiTai &operator=(const MBSNcgiTai &other) = delete;

    virtual ~MBSNcgiTai();

    CJson json(bool as_request) const;

    const std::shared_ptr<NcgiTai> &getNcgiTai() const {return m_ncgiTai;};
    const std::shared_ptr< Tai > &getTai() const {return m_ncgiTai->getTai();};
    const NcgiTai::CellListType &getCellList() const {return m_ncgiTai->getCellList();};

    mb_smf_sc_ncgi_tai_t *populateNcgiTai();
    void ncgis(mb_smf_sc_ncgi_tai_t *ncgi_tai);

private:
    std::shared_ptr<NcgiTai> m_ncgiTai;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_NCGI_TAI_HH_ */
