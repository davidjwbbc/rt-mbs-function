/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Arp class
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
#include "UserDataIngSession.hh"
#include "openapi/model/Arp.h"
#include "openapi/model/PreemptionCapability.h"
#include "openapi/model/PreemptionVulnerability.h"
#include "openapi/model/FECConfig.h"
#include "openapi/model/DistSessionState.h"
#include "openapi/model/ExternalMbsServiceArea.h"
#include "openapi/model/MbsServiceArea.h"
#include "openapi/model/PacketDistrMethInfo.h"
#include "openapi/model/DistributionMethod.h"
#include "openapi/model/NrRedCapUeInfo.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/AssociatedSessionId.h"
#include "openapi/model/ObjectDistrMethInfo.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "AllocationRetentionPriority.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::Arp;
using fiveg_mag_reftools::ModelException;
using reftools::mbsf::PreemptionCapability;
using reftools::mbsf::PreemptionVulnerability;

MBSF_NAMESPACE_START

AllocationRetentionPriority::AllocationRetentionPriority(CJson &json, bool as_request)
    :m_arp(new Arp(json, as_request))
{
}

AllocationRetentionPriority::AllocationRetentionPriority(const std::shared_ptr<Arp> &arp)
    :m_arp(arp)
{
}

AllocationRetentionPriority::~AllocationRetentionPriority()
{
}

CJson AllocationRetentionPriority::json(bool as_request = false) const
{
    return m_arp->toJSON(as_request);
}

const std::unordered_map<std::string, mb_smf_sc_preemption_capability_e> AllocationRetentionPriority::preemptionCap = {
        // MB-SMF Preempt Cap mapping
    { "NOT_PREEMPT", ARP_PREEMPT_CAPABILITY_NOT_PREEMPT},
    { "MAY_PREEMPT", ARP_PREEMPT_CAPABILITY_MAY_PREEMPT}
};

const std::unordered_map<std::string, mb_smf_sc_preemption_vulnerability_e> AllocationRetentionPriority::preemptionVuln = {
    { "NOT_PREEMPTABLE", ARP_PREEMPT_VULNERABILITY_NOT_PREEMPTABLE},
    { "PREEMPTABLE", ARP_PREEMPT_VULNERABILITY_PREEMPTABLE}
};

mb_smf_sc_arp_t *AllocationRetentionPriority::populateArp() {
    mb_smf_sc_arp_t *arp = mb_smf_sc_arp_new();
    arp->priority_level = static_cast<uint8_t>(getPriorityLevel());
    arp->preemption_capability = preemptionCapabilityLookup();
    arp->preemption_vulnerability = preemptionVulnerabilityLookup();
    return arp;
}

mb_smf_sc_preemption_capability_e AllocationRetentionPriority::preemptionCapabilityLookup() {
    const std::shared_ptr< PreemptionCapability > &preemption_capability = getPreemptionCapability();
    const std::string &preempt_capability = preemption_capability->getString();
    auto it = preemptionCap.find(preempt_capability);
    if (it == preemptionCap.end()) return ARP_PREEMPT_CAPABILITY_NULL;
    return it->second;
}

mb_smf_sc_preemption_vulnerability_e AllocationRetentionPriority::preemptionVulnerabilityLookup() {
    const std::shared_ptr< PreemptionVulnerability > &preemption_vulnerability = getPreemptionVulnerability();
    const std::string &preempt_vulnerability = preemption_vulnerability->getString();
    auto it = preemptionVuln.find(preempt_vulnerability);
    if (it == preemptionVuln.end()) return ARP_PREEMPT_VULNERABILITY_NULL;
    return it->second;

}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

