/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: GeographicalArea class
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
#include "GeographicalCoordinate.hh"
#include "openapi/model/GeographicArea.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "GeographicalArea.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::GeographicalCoordinates;
using reftools::mbsf::GeographicArea;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

GeographicalArea::GeographicalArea(CJson &json, bool as_request)
    :m_geographicArea(new GeographicArea(json, as_request))
{
}

GeographicalArea::GeographicalArea(const std::shared_ptr<GeographicArea> &geographic_area)
    :m_geographicArea(geographic_area)
{
}

GeographicalArea::~GeographicalArea()
{
}

CJson GeographicalArea::json(bool as_request = false) const
{
    return m_geographicArea->toJSON(as_request);
}

 mb_smf_sc_geographic_area_t *GeographicalArea::populateGeographicArea()
{

    std::shared_ptr< GeographicalCoordinate > gc = nullptr;
    
    const std::shared_ptr< GeographicalCoordinates > &point = getPoint();
    gc.reset(new GeographicalCoordinate(point));

    mb_smf_sc_geographic_area_t *ga = mb_smf_sc_ga_point_new(gc->getLon(), gc->getLat());

    return ga;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

