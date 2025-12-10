/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: GeographicalCoordinate class
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
#include "MBSPlmnId.hh"
#include "openapi/model/GeographicalCoordinates.h"

#include "mb-smf-service-consumer.h"

// Header include for this class
#include "GeographicalCoordinate.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::GeographicalCoordinates;
using fiveg_mag_reftools::ModelException;

MBSF_NAMESPACE_START

GeographicalCoordinate::GeographicalCoordinate(CJson &json, bool as_request)
    :m_geographicalCoordinates(new GeographicalCoordinates(json, as_request))
{
}

GeographicalCoordinate::GeographicalCoordinate(const std::shared_ptr<GeographicalCoordinates> &geographical_coordinates)
    :m_geographicalCoordinates(geographical_coordinates)
{
}

GeographicalCoordinate::~GeographicalCoordinate()
{
}

CJson GeographicalCoordinate::json(bool as_request = false) const
{
    return m_geographicalCoordinates->toJSON(as_request);
}

 mb_smf_sc_geographic_coordinate_t *GeographicalCoordinate::populateGeographicCoordinate()
{

    mb_smf_sc_geographic_coordinate_t *gc = nullptr;
    //gc = mb_smf_sc_geographic_coordinate_new();
    gc->longitude = getLon();
    gc->latitude = getLat();
    return gc;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

