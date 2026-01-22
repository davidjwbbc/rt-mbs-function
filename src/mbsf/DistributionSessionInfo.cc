/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Distribution Session Info class
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

// Open5GS includes
#include "ogs-app.h"
#include "ogs-sbi.h"

// standard template library includes
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstdint>
#include <iostream>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "UniqueMBSSessionId.hh"
#include "UserDataIngSession.hh"
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/FECConfig.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/MbsServiceInfo.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/NrRedCapUeInfo.h"
#include "openapi/model/ObjectDistrMethInfo.h"
#include "openapi/model/PacketDistrMethInfo.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "DistributionSessionInfo.hh"

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::ExternalMbsServiceArea;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::MbsServiceArea;
using reftools::mbsf::MbsServiceInfo;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MBSUserDataIngSession;

MBSF_NAMESPACE_START

DistributionSessionInfo::DistributionSessionInfo(CJson &json, bool as_request)
    :m_mbsDistributionSessionInfo(new MBSDistributionSessionInfo(json, as_request))
{
}

DistributionSessionInfo::DistributionSessionInfo(const std::shared_ptr<MBSDistributionSessionInfo> &mbs_distribution_session_info)
    :m_mbsDistributionSessionInfo(mbs_distribution_session_info)
{
}

DistributionSessionInfo::~DistributionSessionInfo()
{
}

CJson DistributionSessionInfo::json(bool as_request = false) const
{
    return m_mbsDistributionSessionInfo->toJSON(as_request);
}

std::shared_ptr<MBSDistributionSessionInfo> &DistributionSessionInfo::updateMBSDistributionSessionInfo(
    std::shared_ptr<MBSDistributionSessionInfo> new_mbs_dist_session_infos)
{

    // --------------------------------------------------------------------
    // 1. Unrestricted updates
    // --------------------------------------------------------------------
    m_mbsDistributionSessionInfo->setMbsServInfo(std::move(new_mbs_dist_session_infos->getMbsServInfo()));

    m_mbsDistributionSessionInfo->setMbsFSAId(std::move(new_mbs_dist_session_infos->getMbsFSAId()));

    m_mbsDistributionSessionInfo->setTgtServAreas(std::move(new_mbs_dist_session_infos->getTgtServAreas()));

    // --------------------------------------------------------------------
    // 2. Conditional updates – only when the session is INACTIVE
    // --------------------------------------------------------------------
    std::optional<std::shared_ptr< DistSessionState > > dist_session_state = m_mbsDistributionSessionInfo->getMbsDistSessState();

    if(dist_session_state.has_value() && dist_session_state.value()->getValue() == DistSessionState::VAL_INACTIVE) {
        // ----- Max Continuous Bit Rate -----
        m_mbsDistributionSessionInfo->setMaxContBitRate(std::move(new_mbs_dist_session_infos->getMaxContBitRate()));

        // ----- Max Continuous Delay -----
        m_mbsDistributionSessionInfo->setMaxContDelay(std::move(new_mbs_dist_session_infos->getMaxContDelay()));

        // ----- Distribution Method -----
        m_mbsDistributionSessionInfo->setDistrMethod(std::move(new_mbs_dist_session_infos->getDistrMethod()));

        // ----- FEC Config -----
        m_mbsDistributionSessionInfo->setFecConfig(std::move(new_mbs_dist_session_infos->getFecConfig()));

        // ----- Object Distribution Method Info -----
        m_mbsDistributionSessionInfo->setObjDistrInfo(std::move(new_mbs_dist_session_infos->getObjDistrInfo()));

        // ----- Packet Distribution Method Info -----
        m_mbsDistributionSessionInfo->setPckDistrInfo(std::move(new_mbs_dist_session_infos->getPckDistrInfo()));

        // ----- Traffic Marking Info -----
        m_mbsDistributionSessionInfo->setTrafficMarkingInfo(std::move(new_mbs_dist_session_infos->getTrafficMarkingInfo()));

        // ----- External Target Service Areas -----
        m_mbsDistributionSessionInfo->setExtTgtServAreas(std::move(new_mbs_dist_session_infos->getExtTgtServAreas()));

        // ----- Multiplexed Service Flag -----
        m_mbsDistributionSessionInfo->setMultiplexedServFlag(std::move(new_mbs_dist_session_infos->getMultiplexedServFlag()));

        // ----- Restricted Flag -----
        m_mbsDistributionSessionInfo->setRestrictedFlag(std::move(new_mbs_dist_session_infos->getRestrictedFlag()));

        // ----- NR RedCap UE Info -----
        m_mbsDistributionSessionInfo->setNrRedCapUeInfo(std::move(new_mbs_dist_session_infos->getNrRedCapUeInfo()));

        // ----- Associated Session Id -----
        m_mbsDistributionSessionInfo->setAssociatedSessionId(std::move(new_mbs_dist_session_infos->getAssociatedSessionId()));
    }

    m_mbsDistributionSessionInfo->setMbsDistSessState(std::move(new_mbs_dist_session_infos->getMbsDistSessState()));

    return m_mbsDistributionSessionInfo;
}

UniqueMbsSessionId DistributionSessionInfo::getUniqueMbsSessionId() const
{
    const auto &mbs_session_id = getMbsSessionId();
    if (!mbs_session_id) return UniqueMbsSessionId();
    const auto &mbs_service_area = getTgtServAreas();
    const auto &ext_mbs_service_area = getExtTgtServAreas();
    return UniqueMbsSessionId(!!mbs_session_id->getSsm(), mbs_session_id, mbs_service_area, ext_mbs_service_area);
}

std::shared_ptr<MbsSessionId> DistributionSessionInfo::getMbsSessionId() const
{
    if (!m_mbsDistributionSessionInfo) return nullptr;
    const auto &mbs_session_id = m_mbsDistributionSessionInfo->getMbsSessionId();
    if (!mbs_session_id) return nullptr;
    return mbs_session_id.value();
}

std::shared_ptr<MbsServiceArea> DistributionSessionInfo::getTgtServAreas() const
{
    if (!m_mbsDistributionSessionInfo) return nullptr;
    const auto &mbs_service_area = m_mbsDistributionSessionInfo->getTgtServAreas();
    if (!mbs_service_area) return nullptr;
    return mbs_service_area.value();
}

std::shared_ptr<ExternalMbsServiceArea> DistributionSessionInfo::getExtTgtServAreas() const
{
    if (!m_mbsDistributionSessionInfo) return nullptr;
    const auto &ext_mbs_service_area = m_mbsDistributionSessionInfo->getExtTgtServAreas();
    if (!ext_mbs_service_area) return nullptr;
    return ext_mbs_service_area.value();
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

