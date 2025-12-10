/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Civic Address class
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
#include "openapi/model/CivicAddress.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "CivicAddr.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::CivicAddress;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

static char *populate_mb_smf_civic_addr_fields(const std::optional<std::string> &civic_addr);

CivicAddr::CivicAddr(CJson &json, bool as_request)
    :m_civicAddress(new CivicAddress(json, as_request))
{
}

CivicAddr::CivicAddr(const std::shared_ptr<CivicAddress> &civic_address)
    :m_civicAddress(civic_address)
{
}

CivicAddr::~CivicAddr()
{
}

CJson CivicAddr::json(bool as_request = false) const
{
    return m_civicAddress->toJSON(as_request);
}

 mb_smf_sc_civic_address_t *CivicAddr::populateCivicAddress()
{
    mb_smf_sc_civic_address_t *civic_addr = mb_smf_sc_civic_address_new();

    populateAddr(civic_addr->a);

    civic_addr->country = populate_mb_smf_civic_addr_fields(getCountry());
    civic_addr->prd = populate_mb_smf_civic_addr_fields(getPRD());
    civic_addr->pod = populate_mb_smf_civic_addr_fields(getPOD());
    civic_addr->sts = populate_mb_smf_civic_addr_fields(getSTS());
    civic_addr->hno = populate_mb_smf_civic_addr_fields(getHNO());
    civic_addr->hns = populate_mb_smf_civic_addr_fields(getHNS());
    civic_addr->lmk = populate_mb_smf_civic_addr_fields(getLMK());
    civic_addr->loc = populate_mb_smf_civic_addr_fields(getLOC());
    civic_addr->nam = populate_mb_smf_civic_addr_fields(getNAM());
    civic_addr->pc = populate_mb_smf_civic_addr_fields(getPC());
    civic_addr->bld = populate_mb_smf_civic_addr_fields(getBLD());
    civic_addr->unit = populate_mb_smf_civic_addr_fields(getUNIT());
    civic_addr->flr = populate_mb_smf_civic_addr_fields(getFLR());
    civic_addr->room = populate_mb_smf_civic_addr_fields(getROOM());
    civic_addr->plc = populate_mb_smf_civic_addr_fields(getPLC());
    civic_addr->pcn = populate_mb_smf_civic_addr_fields(getPCN());
    civic_addr->pobox = populate_mb_smf_civic_addr_fields(getPOBOX());
    civic_addr->addcode = populate_mb_smf_civic_addr_fields(getADDCODE());
    civic_addr->seat = populate_mb_smf_civic_addr_fields(getSEAT());
    civic_addr->rd = populate_mb_smf_civic_addr_fields(getRD());
    civic_addr->rdsec = populate_mb_smf_civic_addr_fields(getRDSEC());
    civic_addr->rdbr = populate_mb_smf_civic_addr_fields(getRDBR());
    civic_addr->rdsubbr = populate_mb_smf_civic_addr_fields(getRDSUBBR());
    civic_addr->prm = populate_mb_smf_civic_addr_fields(getPRM());
    civic_addr->pom = populate_mb_smf_civic_addr_fields(getPOM());
    civic_addr->usage_rules = populate_mb_smf_civic_addr_fields(getUsageRules());
    civic_addr->method = populate_mb_smf_civic_addr_fields(getMethod());
    civic_addr->provided_by = populate_mb_smf_civic_addr_fields(getProvidedBy());

    return civic_addr;
}

void CivicAddr::populateAddr(char* a[6]) const {
     const std::optional<std::string> addr_fields[6] = {
        getA1(),
        getA2(),
        getA3(),
        getA4(),
        getA5(),
        getA6()
    };

    for (int i = 0; i < 6; ++i) {
        if (addr_fields[i].has_value()) {
            a[i] = ogs_strdup(addr_fields[i].value().c_str());  // allocate + copy
        } else {
            a[i] = nullptr;
        }
    }
}

static char *populate_mb_smf_civic_addr_fields(const std::optional<std::string> &civic_addr) {
    char *mb_smf_civic_addr = nullptr;
    if (civic_addr.has_value()) {
        mb_smf_civic_addr = ogs_strdup(civic_addr.value().c_str());
    }
    return mb_smf_civic_addr;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

