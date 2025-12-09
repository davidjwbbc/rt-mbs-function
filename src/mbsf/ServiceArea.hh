#ifndef _MBSF_SERVICE_AREA_HH_
#define _MBSF_SERVICE_AREA_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Service Area class
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
#include "openapi/model/MbsServiceArea.h"
#include "common.hh"

namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class MbsServiceArea;
}

using fiveg_mag_reftools::CJson;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::Ncgi;
using reftools::mbsf::NcgiTai;
using reftools::mbsf::Tai;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

class MBSMFMBSSession;

class ServiceArea {
public:

    ServiceArea(CJson &json, bool as_request);
    ServiceArea(const std::shared_ptr<MbsServiceArea> &mbs_service_area);
    ServiceArea() = delete;
    ServiceArea(ServiceArea &&other) = delete;
    ServiceArea(const ServiceArea &other) = delete;
    ServiceArea &operator=(ServiceArea &&other) = delete;
    ServiceArea &operator=(const ServiceArea &other) = delete;

    virtual ~ServiceArea();

    CJson json(bool as_request) const;

    const std::shared_ptr<MbsServiceArea> &getMbsServiceArea() const {return m_mbsServiceArea;};
    const reftools::mbsf::MbsServiceArea::NcgiListType &getNcgiList() const {return m_mbsServiceArea->getNcgiList();};
    const reftools::mbsf::MbsServiceArea::TaiListType &getTaiList() const {return m_mbsServiceArea->getTaiList();};

    mb_smf_sc_mbs_service_area_t *populateServiceArea();
    void ncgiTais(mb_smf_sc_mbs_service_area_t *mbs_service_area);
    void tais(mb_smf_sc_mbs_service_area_t *mbs_service_area);

private:
    std::shared_ptr<MbsServiceArea> m_mbsServiceArea;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_SERVICE_AREA_HH_ */
