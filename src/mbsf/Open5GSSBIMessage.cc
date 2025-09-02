/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Open5GS SBI Message interface
 ******************************************************************************
 * Copyright: (C)2024-2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk>
 *            Dev Audsin <dev.audsin@bbc.co.uk>
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

#include <stdexcept>
#include <memory>

#include "common.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"

#include "Open5GSSBIMessage.hh"

MBSF_NAMESPACE_START

Open5GSSBIMessage::~Open5GSSBIMessage()
{
    //ogs_debug("~Open5GSSBIMessage: message=%p, owner=%s", m_message, m_owner?"true":"false");
    if (m_message != nullptr && m_owner) {
        ogs_sbi_message_free(m_message);
        delete m_message;
    }
}

void Open5GSSBIMessage::parseHeader(Open5GSSBIRequest &request)
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
    int rv = ogs_sbi_parse_header(m_message, &request.ogsSBIRequest()->h);
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_header() failed");
}

void Open5GSSBIMessage::parseHeader(Open5GSSBIResponse &response)
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
    int rv = ogs_sbi_parse_header(m_message, &response.ogsSBIResponse()->h);
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_header() failed");
}

void Open5GSSBIMessage::parseResponse(Open5GSSBIResponse &response)
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
    int rv = ogs_sbi_parse_response(m_message, response.ogsSBIResponse());
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_response() failed");
}

void Open5GSSBIMessage::parseRequest(Open5GSSBIRequest &request)
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
    int rv = ogs_sbi_parse_request(m_message, request.ogsSBIRequest());
    if (rv != OGS_OK) throw std::runtime_error("ogs_sbi_parse_request() failed");
}

const char *Open5GSSBIMessage::resourceComponent(size_t idx) const
{
    if (!m_message) return nullptr;
    if (idx >= OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT) return nullptr;
    return m_message->h.resource.component[idx];
}

Open5GSSBIMessage &Open5GSSBIMessage::resourceComponent(size_t idx, char *resource_component)
{
    if (!m_message) throw std::runtime_error("Adding resource component failed");
    if (idx >= OGS_SBI_MAX_NUM_OF_RESOURCE_COMPONENT) throw std::runtime_error("Maximum number of resource component failed");
    m_message->h.resource.component[idx] = (char *) ogs_strdup(resource_component);
    return *this;
}

Open5GSSBIMessage &Open5GSSBIMessage::method(char *method)
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
    m_message->h.method = ogs_strdup(method);
    return *this;
};

Open5GSSBIMessage &Open5GSSBIMessage::serviceName(char *service_name)
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
    m_message->h.service.name = ogs_strdup(service_name);
    return *this;
};

Open5GSSBIMessage &Open5GSSBIMessage::apiVersion(char *api_version)
{
    if (!m_message) {
        m_message = new ogs_sbi_message_t;
        m_owner = true;
    }
    m_message->h.service.name = ogs_strdup(api_version);
    return *this;
};

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
