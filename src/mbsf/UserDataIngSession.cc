/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Session class
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
#include <chrono>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstdint>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "hash.hh"
#include "MBSFNetworkFunction.hh"
#include "MBSMFMBSSession.hh"
#include "MBSProblemCause.hh"
#include "NfServer.hh"
#include "Nmb2Build.hh"
#include "Open5GSEvent.hh"
#include "Open5GSSBIMessage.hh"
#include "Open5GSSBIObject.hh"
#include "Open5GSSBIRequest.hh"
#include "Open5GSSBIResponse.hh"
#include "Open5GSSBIServer.hh"
#include "Open5GSSBIClient.hh"
#include "Open5GSSBIStream.hh"
#include "Open5GSTimer.hh"
#include "Open5GSYamlDocument.hh"
#include "Open5GSNetworkFunction.hh"
#include "UserService.hh"
#include "openapi/model/DistSession.h"
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/Tmgi.h"
#include "openapi/model/TunnelAddress.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/Ssm.h"
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/PlmnId.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/MbsServiceType.h"
#include "openapi/model/IpAddr.h"
#include "openapi/model/ProblemCause.hh"

#include "openapi/api/IndividualMBSUserDataIngestSessionDocumentApi-info.h"
#include "mb-smf-service-consumer.h"
#include "TimerFunc.hh"

// Header include for this class
#include "UserDataIngSession.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::ObjDistributionData;
using reftools::mbsf::TunnelAddress;
using reftools::mbsf::Ssm;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MbsServiceType;
using reftools::mbsf::IpAddr;
using reftools::mbsf::PlmnId;
using reftools::mbsf::Tmgi;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjectDistrMethInfo;
using reftools::mbsf::ObjAcquisitionMethod;

MBSF_NAMESPACE_START

static const NfServer::InterfaceMetadata g_nmbsf_userdataingsession_api_metadata(
    NMBSF_MBS_UD_INGEST_API_NAME,
    NMBSF_MBS_UD_INGEST_API_VERSION
);

static std::optional<std::shared_ptr< Ssm > > ssm(CJson &json, bool as_request);
static bool validate_user_data_ing_session(CJson &json, bool as_request);
static bool resolve_src_dest_addr(const std::string &src_ipv4_addr, const std::string &dest_ipv4_addr, struct addrinfo **ai_src, struct addrinfo **ai_dest);
static bool get_src_dest_of_same_addr_family(int family, struct addrinfo *src_addrinfo, struct addrinfo *dest_addrinfo, void **src_addr, void **dest_addr);
static void process_mbs_distribution_session_info(std::shared_ptr< UserDataIngSession::ContextData > context_data, std::shared_ptr< DistSession > dist_session);

std::recursive_mutex UserDataIngSession::m_mutex;
std::map<ogs_sbi_xact_t *, std::shared_ptr< UserDataIngSession::UserDataIngDistSessId >> UserDataIngSession::m_xactRegistry;
std::map<std::string, std::shared_ptr< UserDataIngSession::UserDataIngDistSessId >> UserDataIngSession::s_distSessionIdRegistry;;

UserDataIngSession::UserDataIngSession(CJson &json, bool as_request, std::shared_ptr<Open5GSEvent> &event)
{
    ogs_uuid_t uuid;

    char id[OGS_UUID_FORMATTED_LENGTH + 1];

    ogs_uuid_get(&uuid);
    ogs_uuid_format(id, &uuid);

    m_MBSUserDataIngSession.reset( new MBSUserDataIngSession(json, as_request));

    m_generated = std::chrono::system_clock::now();
    m_lastUsed = m_generated;

    std::string json_str(json.serialise());
    m_hash = calculate_hash(std::vector<std::string::value_type>(json_str.begin(), json_str.end()));

    m_UserDataIngSessionId = std::string(id);
}


UserDataIngSession::~UserDataIngSession()
{
}

CJson UserDataIngSession::json(bool as_request = false) const
{
    return m_MBSUserDataIngSession->toJSON(as_request);
}


const std::shared_ptr<UserDataIngSession> &UserDataIngSession::find(const std::string &id)
{
    const std::map<std::string, std::shared_ptr<UserDataIngSession> > &UserDataIngSessions = App::self().context()->UserDataIngSessions;
    auto it = UserDataIngSessions.find(id);
    if (it == UserDataIngSessions.end()) {
        throw std::out_of_range("MBS User Data Ingest session not found");
    }
    return it->second;
}

bool UserDataIngSession::processEvent(Open5GSEvent &event)
{

    const NfServer::InterfaceMetadata &nmbsf_mbs_userdataingsession_api = g_nmbsf_userdataingsession_api_metadata;
    const NfServer::AppMetadata &app_meta = App::self().mbsfAppMetadata();

    switch (event.id()) {
    case OGS_EVENT_SBI_SERVER:
        {
            Open5GSSBIRequest request(event.sbiRequest());
            Open5GSSBIMessage message;
            ogs_pool_id_t stream_id = OGS_POINTER_TO_UINT(reinterpret_cast<ogs_sbi_stream_t*>(event.sbiData()));
            Open5GSSBIStream stream(stream_id);

            Open5GSSBIServer server(stream.server());
            std::optional<NfServer::InterfaceMetadata> api(std::nullopt);

            try {
                message.parseHeader(request);
            } catch (std::exception &ex) {
                ogs_error("Failed to parse request headers");
                break;
            }

            std::string service_name(message.serviceName());
            std::string resource0(message.resourceComponent(0));
            ogs_debug("OGS_EVENT_SBI_SERVER: service=%s, component[0]=%s", service_name.c_str(), resource0.c_str());
            if (service_name == "nmbsf-mbs-ud-ingest") {
                api.emplace(nmbsf_mbs_userdataingsession_api);
            } else {
                return false;
            }

            if (api.value() == nmbsf_mbs_userdataingsession_api) {
                /******** nmbsf-mbs-ud-ingest ********/
                std::string api_version(message.apiVersion());
                if (api_version != OGS_SBI_API_V1) {
                    ogs_error("Unsupported API version [%s]", api_version.c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message, app_meta,
                                                           api, "Unsupported API version"));
                    return true;
                }

                if (resource0 == "sessions") {
                    std::string method(message.method());
                    const char *ptr_resource1 = message.resourceComponent(1);
                    if (method == OGS_SBI_HTTP_METHOD_POST) {
                        ogs_debug("POST response: status = %i", message.resStatus());
                        std::shared_ptr<UserDataIngSession> user_data_ing_session = nullptr;
                        ogs_debug("Request body: %s", request.content());
                        //ogs_debug("Request " OGS_SBI_CONTENT_TYPE ": %s", request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()).c_str());
                        if (request.headerValue(OGS_SBI_CONTENT_TYPE, std::string()) != "application/json") {
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                                                                   3, message, app_meta, api, "Unsupported Media Type",
                                                                   "Expected content type: application/json"));
                            return true;
                        }

                        CJson user_data_ing_sess(CJson::Null);
                        try {
                            user_data_ing_sess = CJson::parse(request.content());
                        } catch (std::exception &ex) {
                            static const char *err = "Unable to parse MBSF User Service as JSON.";
                            ogs_error("%s", err);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad MBSF User Service", err));
                            return true;
                        }

                        {
                            std::string txt(user_data_ing_sess.serialise());
                            ogs_debug("Request Parsed JSON: %s", txt.c_str());
                        }


                        std::shared_ptr<Open5GSEvent> event_ctx = nullptr;
                        event_ctx.reset(new Open5GSEvent(event.ogsEvent()));
                        try {

                            user_data_ing_session.reset(new UserDataIngSession(user_data_ing_sess, true, event_ctx));
                        } catch (std::exception &err) {
                            ogs_error("Error while populating MBSF Session: %s", err.what());
                            char *error = ogs_msprintf("%s", err.what());
                            ogs_error("%s", error);
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message,
                                                                            app_meta, api, "Bad Request", error));
                            ogs_free(error);
                            return true;
                        }

                        App::self().context()->addUserDataIngSession(user_data_ing_session);
                        user_data_ing_session->processDistributionSessionInfo(user_data_ing_session->getMBSUserIngSession(), event_ctx);

#if 0
                        CJson user_data_ing_sess_json(user_data_ing_session->json(false));
                        std::string body(user_data_ing_sess_json.serialise());
                        ogs_debug("Response Parsed JSON: %s", body.c_str());
                        std::ostringstream location;
                        location << request.uri() << "/" << user_data_ing_session->userDataIngSessionId();
                        std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(location.str(),
                                                                                    body.empty()?nullptr:"application/json",
                                                                                    user_data_ing_session->generated(),
                                                                                    user_data_ing_session->hash().c_str(),
                                                                                    App::self().context()->cacheControl.MBSUserServiceMaxAge,
                                                                                    std::nullopt/*nullptr*/, api, app_meta));
                        ogs_assert(response);
                        NfServer::populateResponse(response, body, OGS_SBI_HTTP_STATUS_CREATED);
                        ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
#endif
                        return true;
                    } else if (method == OGS_SBI_HTTP_METHOD_GET) {
                        if (!ptr_resource1) {
                            std::ostringstream err;
                            err << "Invalid resource [" << message.uri() << "]";
                            ogs_error("%s", err.str().c_str());
                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                    app_meta, api, "Bad Request", err.str()));
                            return true;
                        }
                        std::string user_data_ing_session_id(ptr_resource1);
                        try {
                            int response_code = 200;

                            std::shared_ptr<UserDataIngSession> user_data_ing_sess = UserDataIngSession::find(user_data_ing_session_id);
                            CJson user_data_ing_session_json(user_data_ing_sess->json(false));
                            std::string body(user_data_ing_session_json.serialise());
                            ogs_debug("Parsed JSON: %s", body.c_str());
                            std::ostringstream location;
                            location << request.uri() << "/" << user_data_ing_sess->userDataIngSessionId();
                            std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::string(request.uri())/*location.str()*/,
                                                    body.empty()?nullptr:"application/json",
                                                    user_data_ing_sess->generated(),
                                                    user_data_ing_sess->hash().c_str(),
                                                    App::self().context()->cacheControl.MBSUserServiceMaxAge,
                                                    std::nullopt/*nullptr*/, api, app_meta));
                            ogs_assert(response);
                            NfServer::populateResponse(response, body, response_code);
                            ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
                        } catch (const std::out_of_range &e) {
                            std::ostringstream err;
                            err << "MBS Session [" << user_data_ing_session_id << "] does not exist.";
                            ogs_error("%s", err.str().c_str());

                            static const std::string param("{sessionId}");
                            std::ostringstream reason;
                            reason << "Invalid MBS Session identifier [" << user_data_ing_session_id << "]";
                            std::map<std::string, std::string> invalid_params(
                                                                        NfServer::makeInvalidParams(param, reason.str()));

                            ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                                    app_meta, api, "MBSF Distribution Session not found",
                                                                    err.str(), std::nullopt, invalid_params));
                        }
                        return true;
                    } else if (method == OGS_SBI_HTTP_METHOD_DELETE) {
                        if (message.resourceComponent(1) && !message.resourceComponent(2)) {
                            std::string user_data_ing_session_id(message.resourceComponent(1));
                            try {
                                App::self().context()->deleteUserDataIngSession(user_data_ing_session_id);
                                std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0, std::nullopt, api, app_meta));
                                NfServer::populateResponse(response, "", OGS_SBI_HTTP_STATUS_NO_CONTENT);
                                ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));

                            } catch (const std::out_of_range &e) {
                                std::ostringstream err;
                                err << "MBS Session [" << user_data_ing_session_id << "] does not exist.";
                                ogs_error("%s", err.str().c_str());

                                static const std::string param("{sessionId}");
                                std::ostringstream reason;
                                reason << "Invalid MBS Session identifier [" << user_data_ing_session_id << "]";
                                std::map<std::string, std::string> invalid_params(
                                                                        NfServer::makeInvalidParams(param, reason.str()));

                                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_NOT_FOUND, 2, message,
                                                        app_meta, api, "MBS Session not found", err.str(),
                                                        std::nullopt, invalid_params));
                            }
                            return true;
                        }
                    } else {
                        std::ostringstream err;

                        err << "Invalid method [" << message.method() << "] for " << message.serviceName() << "/"
                                << message.apiVersion() << "/" << message.resourceComponent(0);
                        ogs_error("%s", err.str().c_str());
                        ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message,
                                                                app_meta, api, "Bad request", err.str()));
                        return true;
                    }
                } else {
                    std::ostringstream err;
                    err << "Unknown object type \"" << message.resourceComponent(0) << "\" in MBSF User Data Ingest Session";
                    ogs_error("%s", err.str().c_str());
                    ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 1, message, app_meta,
                                                            api, "Bad request", err.str()));
                    return true;
                }
            } else {
                static const char *err = "Missing service name from URL path";
                ogs_error("%s", err);
                ogs_assert(true == NfServer::sendError(stream, OGS_SBI_HTTP_STATUS_BAD_REQUEST, 0, message, app_meta, std::nullopt,
                                                "Missing service name", err));
            }
            return true;
        }
        case MBSF_LOCAL_SEND_MBSTF_REQ_BUILD:
        {
            Nmb2Build::sendNmb2DistSession(event.sbiData());

            return true;
        }

        case MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION:
        {
            Nmb2Build::sendNmb2DistSessionDelete(event.sbiData());

            return true;
        }

        default:
            return false;
    }
    return false;
}

ogs_sbi_xact_t *UserDataIngSession::nmbstfDiscoverOnly(std::shared_ptr< ContextData > data)
{
    ogs_sbi_xact_t *xact = nullptr;

    m_sbiObject = new Open5GSSBIObject();
    xact = m_sbiObject->discoverOnly(OGS_SBI_SERVICE_TYPE_NMBSTF_DISTSESSION, nullptr);
    if (!xact) {
        ogs_error("discoverOnly() failed");
    } else {
       std::shared_ptr<UserDataIngDistSessId> ids = nullptr;
       ids.reset(new UserDataIngDistSessId(data->ingSessionId, data->distSessionInfoKey));
       addToRegistry(xact, ids);
    }
    return xact;
}

void UserDataIngSession::addToDistributionSessionInfos(const std::string &key, const std::shared_ptr< ContextData > context)
{
    std::lock_guard<std::recursive_mutex> lock(m_rmutex);
    m_distributionSessionInfos[key] = context;

}

std::shared_ptr< UserDataIngSession::ContextData > UserDataIngSession::getDistributionSessionInfoData(std::string &key)
{

    std::lock_guard<std::recursive_mutex> lock(m_rmutex);
    auto it = m_distributionSessionInfos.find(key);
    if (it != m_distributionSessionInfos.end()) {
        return it->second;
    }
    return nullptr;

}

void UserDataIngSession::removeDistributionSessionInfo(std::string &key)
{
    std::lock_guard<std::recursive_mutex> lock(m_rmutex);
     m_distributionSessionInfos.erase(key);
}

void UserDataIngSession::addToRegistry(ogs_sbi_xact_t* xact, std::shared_ptr< UserDataIngDistSessId > &ids)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_xactRegistry[xact] = ids;

}

void UserDataIngSession::addToRegistry(std::string &dist_session_id, std::shared_ptr< UserDataIngDistSessId > &ids)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    s_distSessionIdRegistry[dist_session_id] = ids;

}

void UserDataIngSession::removeFromRegistry(ogs_sbi_xact_t* xact)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_xactRegistry.erase(xact);
}

void UserDataIngSession::removeFromRegistry(std::string &dist_session_id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    s_distSessionIdRegistry.erase(dist_session_id);
}

std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > UserDataIngSession::getFromRegistry(ogs_sbi_xact_t* xact)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_xactRegistry.find(xact);
    if (it != m_xactRegistry.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr< UserDataIngSession::UserDataIngDistSessId > UserDataIngSession::getFromRegistry(std::string &dist_session_id)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = s_distSessionIdRegistry.find(dist_session_id);
    if (it != s_distSessionIdRegistry.end()) {
        return it->second;
    }
    return nullptr;
}

void UserDataIngSession::processDistributionSessionInfo(std::shared_ptr<MBSUserDataIngSession> mbs_user_data_ing_session, std::shared_ptr<Open5GSEvent> &event)
{
    const MBSUserDataIngSession::MbsDisSessInfosType &dist_sess_infos = mbs_user_data_ing_session->getMbsDisSessInfos();
    mb_smf_sc_mbs_session_t *session = NULL;

    for(const auto &[key, sess_info]: dist_sess_infos) {
        if(sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = sess_info.value();
            if(info) {
                std::optional<std::shared_ptr< MbsSessionId > > mbs_session_id = info->getMbsSessionId();
                if(mbs_session_id.has_value()) {
                    std::shared_ptr<MbsSessionId > mbs_sess_id = *mbs_session_id;
                    std::optional<std::shared_ptr< Ssm > > ssm = mbs_sess_id->getSsm();
                    if (ssm.has_value()) {
                        std::shared_ptr< Ssm > ssm_val = *ssm;
                        std::shared_ptr< IpAddr > src_ip_addr = ssm_val->getSourceIpAddr();
                        std::shared_ptr< IpAddr > dest_ip_addr = ssm_val->getDestIpAddr();
                        std::optional<std::string > src_ipv4_addr = src_ip_addr->getIpv4Addr();
                        std::optional<std::string > dest_ipv4_addr = dest_ip_addr->getIpv4Addr();
                        std::optional<std::shared_ptr< std::string > > src_ipv6_addr = src_ip_addr->getIpv6Addr();
                        std::optional<std::shared_ptr< std::string > > dest_ipv6_addr = dest_ip_addr->getIpv6Addr();
                        std::shared_ptr< Ssm > ssm_data = nullptr;
                        std::shared_ptr< UserDataIngSession::ContextData > ctx_data = nullptr;
                        ogs_sbi_xact_t *xact = nullptr;

                        ssm_data.reset(new Ssm(*ssm_val));
                        if (dest_ipv4_addr.has_value() || dest_ipv6_addr.has_value()) {

                            ctx_data.reset(new UserDataIngSession::ContextData(/*this,*/ m_UserDataIngSessionId, key, info, ssm_data, event, nullptr));
                            addToDistributionSessionInfos(key, ctx_data);
                            xact = nmbstfDiscoverOnly(ctx_data);

                        } else {
                            ogs_error("Unable to resolve SSM addresses");
                            continue;
                        }

                    }

                }
            }
        }
    }
}

bool UserDataIngSession::processDistSession(std::shared_ptr< DistSession > dist_session)
{
    if(!dist_session) return false;

    std::shared_ptr< UserDataIngSession::ContextData > context_data = nullptr;
    std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> ids = nullptr;

    {
        std::lock_guard<std::recursive_mutex> lock(UserDataIngSession::m_mutex);
        auto it = UserDataIngSession::s_distSessionIdRegistry.find(dist_session->getDistSessionId());
        if (it != UserDataIngSession:: s_distSessionIdRegistry.end()) {
            ids = it->second;
        }
    }

    context_data = UserDataIngSession::getContextData(ids);

    process_mbs_distribution_session_info(context_data, dist_session);

    context_data->receivedMBSTFResponse = true;

    std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    if(ing_sess->checkIfAllMBSTFResponsesReceived()) {
        ing_sess->sendNmbsfMbsUserDataIngestResponse(ids);
    }


    return true;
}

bool UserDataIngSession::checkIfAllMBSTFResponsesReceived()
{
    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->receivedMBSTFResponse) {
            return false;
        }
    }
    return true;

}

bool UserDataIngSession::sendNmbsfMbsUserDataIngestResponse(std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &ids)
{

    std::shared_ptr< UserDataIngSession::ContextData > context_data = UserDataIngSession::getContextData(ids);
    std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    const std::shared_ptr<MBSUserDataIngSession> &user_data_ing_session = ing_sess->getMBSUserIngSession();
    std::shared_ptr<Open5GSEvent> event = context_data->event;

    Open5GSSBIRequest request(event->sbiRequest());
    Open5GSSBIMessage message;
    ogs_pool_id_t stream_id = OGS_POINTER_TO_UINT(reinterpret_cast<ogs_sbi_stream_t*>(event->sbiData()));
    Open5GSSBIStream stream(stream_id);

    const NfServer::InterfaceMetadata &nmbsf_mbs_userdataingsession_api = g_nmbsf_userdataingsession_api_metadata;
    std::optional<NfServer::InterfaceMetadata> api(std::nullopt);

    try {
            message.parseHeader(request);
        } catch (std::exception &ex) {
            ogs_error("Failed to parse request headers");
            return false;
       }

       std::string service_name(message.serviceName());
       ogs_debug("OGS_EVENT_SBI_SERVER: service=%s", service_name.c_str());
       if (service_name == "nmbsf-mbs-ud-ingest") {
           api.emplace(nmbsf_mbs_userdataingsession_api);
       } else {
           return false;
       }


    CJson user_data_ing_sess_json(ing_sess->json(false));
    std::string body(user_data_ing_sess_json.serialise());
    ogs_debug("Response Parsed JSON: %s", body.c_str());
    std::ostringstream location;
    location << request.uri() << "/" << ing_sess->userDataIngSessionId();
    std::shared_ptr<Open5GSSBIResponse> response(NfServer::newResponse(location.str(),
                            body.empty()?nullptr:"application/json",
                            ing_sess->generated(),
                            ing_sess->hash().c_str(),
                            App::self().context()->cacheControl.MBSUserServiceMaxAge,
                            std::nullopt/*nullptr*/, api,  App::self().mbsfAppMetadata()));
    ogs_assert(response);
    NfServer::populateResponse(response, body, OGS_SBI_HTTP_STATUS_CREATED);
    ogs_assert(true == Open5GSSBIServer::sendResponse(stream, *response));
    return true;
}


bool UserDataIngSession::handleMbstfDiscover(ogs_sbi_nf_instance_t *nf_instance, ogs_sbi_xact_t *xact)
{

    Open5GSSBIClient mbstf_client(reinterpret_cast<ogs_sbi_client_t*>(NF_INSTANCE_CLIENT(nf_instance)));
    Open5GSSBIObject sbi_object(xact->sbi_object);
    std::shared_ptr<MBSMFMBSSession> mb_smf_mbs_session;
    ogs_info("HANDLER XACT: %p, SBI OBJ IN XACT: %p", xact, xact->sbi_object);
    std::string src_ipv4_addr;
    std::string src_ipv6_addr;

    ogs_sockaddr_t *client_ipv4_addr =  mbstf_client.ogsSBIClientIPv4Addr();
    ogs_sockaddr_t *client_ipv6_addr =  mbstf_client.ogsSBIClientIPv6Addr();

    if(client_ipv4_addr) {
        src_ipv4_addr = std::string(mbstf_client.ogsIpStrdup(client_ipv4_addr));
        ogs_info("SRC IPv4 ADDR OF MBSTF: %s", src_ipv4_addr.c_str());
    }

    if(client_ipv6_addr) {
         src_ipv6_addr = std::string(mbstf_client.ogsIpStrdup(client_ipv6_addr));
         ogs_info("SRC IPv6 ADDR OF MBSTF: %s", src_ipv6_addr.c_str());
    }


    std::shared_ptr< UserDataIngSession::ContextData > context_data = nullptr;
    std::shared_ptr< UserDataIngDistSessId > ids = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = m_xactRegistry.find(xact);
        if (it != m_xactRegistry.end()) {
            ids = it->second;
        }
    }

    if(!ids) {
        ogs_info("UNABLE TO GET IDs FROM REGISTRY");
        return false;
    }

    context_data = getContextData(ids);
    if(!context_data) {
        ogs_info("UNABLE TO GET CONTEXT DATA FROM REGISTRY");
        return false;
    }

    context_data->mbstfInstance = nf_instance;
    context_data->mbstfXact = xact;
    std::shared_ptr<Ssm> ssm_ptr = context_data->ssm;
    if(!ssm_ptr) ogs_error("Unable to get SSM from Context Data");

    std::shared_ptr< IpAddr > dest_ip_addr = ssm_ptr->getDestIpAddr();
    std::optional<std::string > dest_ipv4_addr = dest_ip_addr->getIpv4Addr();
    std::optional<std::shared_ptr< std::string > > dest_ipv6_addr = dest_ip_addr->getIpv6Addr();

    if(!client_ipv4_addr && !client_ipv6_addr) {
        ogs_info("Source SSM cannot be resolved");
        mb_smf_mbs_session.reset(new MBSMFMBSSession(mb_smf_sc_mbs_session_new()));
        mb_smf_mbs_session->setTunnelRequest(true);

    } else if (client_ipv4_addr && dest_ipv4_addr.has_value()) {
        struct addrinfo *ai_src = NULL, *ai_dest = NULL;
        void *src_addr = NULL, *dest_addr = NULL;

        if(resolve_src_dest_addr(src_ipv4_addr, dest_ipv4_addr.value(), &ai_src, &ai_dest))
        {
            if(get_src_dest_of_same_addr_family(AF_INET, ai_src, ai_dest, &src_addr, &dest_addr))
            {
                mb_smf_mbs_session.reset(new MBSMFMBSSession(
                mb_smf_sc_mbs_session_new_ipv4((const struct in_addr*)src_addr,
                                                     (const struct in_addr*)dest_addr)));

           } else {
               ogs_error("Unable to resolve SSM addresses for IPv4 address family");
               if(ai_src) {
                   freeaddrinfo(ai_src);
                   ai_src = NULL;
               }

               if(ai_dest) {
                   freeaddrinfo(ai_dest);
                   ai_dest = NULL;
                }
                return false;
            }
        }
        if(ai_src) {
            freeaddrinfo(ai_src);
            ai_src = NULL;
        }

        if(ai_dest) {
            freeaddrinfo(ai_dest);
            ai_dest = NULL;
        }

    } else if (client_ipv6_addr && dest_ipv6_addr.has_value()) {
       struct addrinfo *ai_src = NULL, *ai_dest = NULL;
       void *src_addr = NULL, *dest_addr = NULL;
       std::shared_ptr< std::string >  dest_ipv6 = dest_ipv6_addr.value();

       if(resolve_src_dest_addr(src_ipv6_addr, *dest_ipv6, &ai_src, &ai_dest))
       {
           if(get_src_dest_of_same_addr_family(AF_INET6, ai_src, ai_dest, &src_addr, &dest_addr))
           {
               mb_smf_mbs_session.reset(new MBSMFMBSSession(
               mb_smf_sc_mbs_session_new_ipv6((const struct in6_addr*)src_addr,
                               (const struct in6_addr*)dest_addr)));

           } else {
               ogs_error("Unable to resolve SSM addresses for IPv6 address family");
               if(ai_src) freeaddrinfo(ai_src);
               if(ai_dest) freeaddrinfo(ai_dest);
               return false;
           }
       }
       if(ai_src) freeaddrinfo(ai_src);
       if(ai_dest) freeaddrinfo(ai_dest);

    } else {
       ogs_error("Unable to resolve SSM addresses");
       return false;
    }

    if(mb_smf_mbs_session->ssm()) {

        mb_smf_mbs_session->setTunnelRequest(true);
        mb_smf_mbs_session->setTmgiRequest(true);

        mb_smf_mbs_session->setServiceType(MBS_SERVICE_TYPE_MULTICAST);
        if(!context_data->MBSSession) context_data->MBSSession = mb_smf_mbs_session;
        mb_smf_mbs_session->setCreatedCallback(reinterpret_cast<void*>(context_data.get()));

        context_data->MBSSession = mb_smf_mbs_session;

        mb_smf_mbs_session->pushChanges();
    } else {
        ogs_info("MB-SMF SSM IS NULL");
    }

    return true;

}

void UserDataIngSession::deleteMBSTFSession(ogs_sbi_xact_t *xact)
{
    std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> ids = nullptr;

    {
        std::lock_guard<std::recursive_mutex> lock(UserDataIngSession::m_mutex);
        auto it = UserDataIngSession::m_xactRegistry.find(xact);
        if (it != UserDataIngSession::m_xactRegistry.end()) {
            ids = it->second;
        }
    }

    std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    ing_sess->sendMbstfDeleteRequestsOnError();
}

std::shared_ptr< UserDataIngSession::ContextData > UserDataIngSession::getContextData(std::shared_ptr<UserDataIngDistSessId> &ids)
{
    std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    return ing_sess->getDistributionSessionInfoData(ids->second);
}

void UserDataIngSession::removeContextData(std::shared_ptr<UserDataIngSession::ContextData> context_data)
{
    if(context_data->mbstfInstance) ogs_sbi_nf_instance_remove(context_data->mbstfInstance);
    if(context_data->mbstfXact) ogs_sbi_xact_remove(context_data->mbstfXact);
    if(context_data->info) context_data->info.reset();
    if(context_data->ssm) context_data->ssm.reset();
    if(context_data->MBSSession) context_data->MBSSession.reset();
}

void UserDataIngSession::removeDistributionSessionInfos(void *data)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);
    std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    ing_sess->clearDistributionSessionInfos();
}

void UserDataIngSession::clearDistributionSessionInfos()
{
    for (auto &dist_sess_info : m_distributionSessionInfos)
    {
        std::shared_ptr<UserDataIngSession::ContextData> context_data = dist_sess_info.second;
        removeContextData(context_data);

    }
    m_distributionSessionInfos.clear();
}

void UserDataIngSession::checkIfAllMBSSessionCreated()
{
    bool all_mbs_sessions_created = true;

    for (const auto &dist_sess_info : m_distributionSessionInfos) {
        if (!dist_sess_info.second->hasMBSSession) {
            all_mbs_sessions_created = false;
            break;
        }
    }
    if(all_mbs_sessions_created) {
        sendMbstfRequests();

    }

}

void UserDataIngSession::sendMbstfRequests()
{
    for (auto &dist_sess_info : m_distributionSessionInfos)
    {
        std::shared_ptr<UserDataIngSession::ContextData> context_data = dist_sess_info.second;

        UserDataIngDistSessId *ids = new UserDataIngDistSessId(context_data->ingSessionId, context_data->distSessionInfoKey);

        sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_REQ_BUILD, static_cast<void *>(ids));

    }
}

void UserDataIngSession::sendMbstfDeleteRequestsOnError()
{
    for (auto &dist_sess : s_distSessionIdRegistry)
    {
        std::shared_ptr <UserDataIngSession::SessionIdContainer> session_id = nullptr;
        session_id.reset( new SessionIdContainer(dist_sess.first, dist_sess.second));

        void *data = static_cast<void*>(session_id.get());
        sendLocalEvent(MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION, data);

    }
}


void UserDataIngSession::sendLocalEvent(OgsExtendedEventId event_id, void *data)
{
    int rv;

    Open5GSEvent local_event(ogs_event_new(event_id));
    local_event.setSbiData(data);

    rv = ogs_queue_push(ogs_app()->queue, local_event.ogsEvent());
    if (rv != OGS_OK) {
        ogs_error("Failed to push MBSF local  Build MBSTF event onto the eveint queue");
        return;
    }
    /* process the event queue */
    ogs_pollset_notify(ogs_app()->pollset);
}


const char *UserDataIngSession::localEventGetName( ogs_event_t *event)
{

    if (ogs_unlikely(!event)) return "*** No Event ***";

    if (event->id == MBSF_LOCAL_SEND_MBSTF_REQ_BUILD)
    {
        return "MBSF_LOCAL_SEND_MBSTF_REQ_BUILD";
    } else {

        return ogs_event_get_name(event);
    }

    return "Unknown MBSF LOCAL Event";
}

void UserDataIngSession::setMBSSessionFlag(void *data)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);

    std::shared_ptr<UserDataIngDistSessId> ids_ptr = nullptr;
    ids_ptr.reset(ids);
    std::shared_ptr< UserDataIngSession::ContextData > context_data = getContextData(ids_ptr);
    context_data->hasMBSSession = true;

    std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    ing_sess->checkIfAllMBSSessionCreated();

}

void UserDataIngSession::populateAndSendError(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause, const std::optional<CJson> &problem_detail_json)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);

    std::shared_ptr<UserDataIngDistSessId> ids_ptr = nullptr;
    ids_ptr.reset(ids);
    std::shared_ptr< UserDataIngSession::ContextData > context_data = getContextData(ids_ptr);

    std::optional<NfServer::InterfaceMetadata> api(std::nullopt);
    const NfServer::AppMetadata &app_meta = App::self().mbsfAppMetadata();

    Open5GSSBIRequest request(context_data->event->sbiRequest());
    Open5GSSBIMessage message;
    ogs_pool_id_t stream_id = OGS_POINTER_TO_UINT(reinterpret_cast<ogs_sbi_stream_t*>(context_data->event->sbiData()));
    Open5GSSBIStream stream(stream_id);

    Open5GSSBIServer server(stream.server());
    try {
            message.parseHeader(request);
    } catch (std::exception &ex) {
       ogs_error("Failed to parse headers");
       return;
    }

    removeDistributionSessionInfos(data);

    std::ostringstream err;

    err << "Failed to create MBS Session in MB-SMF '"
    << ids->second
    << "' of UserDataIngSession '"
    << ids->first
    << "'.";

    ogs_error("%s", err.str().c_str());

    if(cause.has_value()) {
        ogs_assert(true == NfServer::sendError(stream, cause.value(), 1, message,
                                                          app_meta, api, std::nullopt, std::nullopt, problem_detail_json, std::nullopt, err.str()));
    } else {
        ogs_assert(true == NfServer::sendError(stream, ProblemCause::INBOUND_SERVER_ERROR, 1, message,
                                                            app_meta, api));
    }
}

bool UserDataIngSession::tmgi(std::string mbs_service_id, std::string mcc, std::string mnc, void *data)
{
    UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngDistSessId*>(data);
    std::shared_ptr<UserDataIngSession> ing_sess = UserDataIngSession::find(ids->first);
    std::shared_ptr< UserDataIngSession::ContextData > context_data = ing_sess->getDistributionSessionInfoData(ids->second);
    if(context_data && context_data->info) {
        std::shared_ptr<Tmgi> mgi = nullptr;
        std::shared_ptr<PlmnId> plmn_id = nullptr;

        plmn_id.reset(new PlmnId());
        plmn_id->setMcc(mcc);
        plmn_id->setMnc(mnc);

        mgi.reset(new Tmgi());
        mgi->setMbsServiceId(mbs_service_id);
        mgi->setPlmnId(plmn_id);

        std::optional<std::shared_ptr< MbsSessionId > > mbs_sess_id = context_data->info->getMbsSessionId();
        if(mbs_sess_id.has_value()) {
            std::shared_ptr< MbsSessionId > sess_id = mbs_sess_id.value();
            sess_id->setTmgi(mgi);
            return true;
        }
    }
    return false;

}

std::shared_ptr< ObjDistributionOperatingMode > UserDataIngSession::getOperatingMode(std::shared_ptr<MBSDistributionSessionInfo> info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getOperatingMode();
    }
    return nullptr;
}

std::shared_ptr< ObjAcquisitionMethod > UserDataIngSession::getAcquisitionMethod(std::shared_ptr<MBSDistributionSessionInfo> info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjAcqMethod();
    }
    return nullptr;
}

std::optional<std::string> UserDataIngSession::getObjectIngestUrl(std::shared_ptr<MBSDistributionSessionInfo> info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjIngUri();
    }
    return std::nullopt;
}

std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > > UserDataIngSession::getObjectAcquisitionIds(std::shared_ptr<MBSDistributionSessionInfo> info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjAcqIds();
    }
    return {};
}

std::optional<std::string> UserDataIngSession::getObjectDistributionUrl(std::shared_ptr<MBSDistributionSessionInfo> info)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > > obj_dist_method_info = info->getObjDistrInfo();
    if(obj_dist_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > dist_method_info = obj_dist_method_info.value();
        return dist_method_info->getObjDistrUri();
    }
    return std::nullopt;
}

std::string UserDataIngSession::maxContBitRate(std::shared_ptr<MBSDistributionSessionInfo> info)
{
    return info->getMaxContBitRate();
}

static void process_mbs_distribution_session_info(std::shared_ptr< UserDataIngSession::ContextData > context_data, std::shared_ptr< DistSession > dist_session)
{
    std::optional<std::shared_ptr< ObjectDistrMethInfo > >  obj_distribution_method_info = context_data->info->getObjDistrInfo();
    if(obj_distribution_method_info.has_value()) {
        std::shared_ptr< ObjectDistrMethInfo > obj_dist_method_info = obj_distribution_method_info.value();
        if(obj_dist_method_info->getOperatingMode()->getString() == "PUSH") {
            std::optional<std::shared_ptr< ObjDistributionData > > obj_distribution_data = dist_session->getObjDistributionData();
            if(obj_distribution_data.has_value()) {
                std::shared_ptr< ObjDistributionData > obj_dist_data = obj_distribution_data.value();
                std::shared_ptr< ObjAcquisitionMethod> obj_acquisition_method = obj_dist_data->getObjAcquisitionMethod();
                      if(obj_acquisition_method->getString() == "PUSH") {
                    obj_dist_method_info->setObjIngUri(obj_dist_data->getObjIngestBaseUrl());
                }

            }

        }

    }
}

static std::optional<std::shared_ptr< Ssm > > ssm(CJson &json, bool as_request)
{
    MBSUserDataIngSession user_data_ing_session(json, as_request);
    std::string mbs_user_service_id(user_data_ing_session.getMbsUserServId());
    const MBSUserDataIngSession::MbsDisSessInfosType &dist_sess_infos = user_data_ing_session.getMbsDisSessInfos();
    for(const auto &[key, sess_info]: dist_sess_infos) {
        if(sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = *sess_info;
            if(info) {
                std::optional<std::shared_ptr< MbsSessionId > > mbs_session_id = info->getMbsSessionId();
                if(mbs_session_id.has_value()) {
                    std::shared_ptr<MbsSessionId > mbs_sess_id = *mbs_session_id;
                    return mbs_sess_id->getSsm();
                }
            }
        }
    }
    return std::nullopt;

}

static bool validate_user_data_ing_session(CJson &json, bool as_request)
{
    MBSUserDataIngSession user_data_ing_session(json, as_request);
    std::string mbs_user_service_id(user_data_ing_session.getMbsUserServId());
    const std::shared_ptr<UserService> user_service = UserService::find(mbs_user_service_id);
    std::string user_service_type(user_service->getMBSUserServiceType());
    const MBSUserDataIngSession::MbsDisSessInfosType &distSessInfos = user_data_ing_session.getMbsDisSessInfos();
    for(const auto &[key, sess_info]: distSessInfos) {
        if(sess_info.has_value())
        {
            std::shared_ptr<MBSDistributionSessionInfo> info = *sess_info;
            if(info) {
                std::optional<std::shared_ptr< MbsSessionId > > mbs_session_id = info->getMbsSessionId();
                if(mbs_session_id.has_value()) {
                    std::shared_ptr<MbsSessionId > mbs_sess_id = *mbs_session_id;
                    std::optional<std::shared_ptr< Ssm > > ssm = mbs_sess_id->getSsm();
                    if (ssm.has_value()) {
                        std::shared_ptr< Ssm > ssm_val = ssm.value();
                        std::shared_ptr< IpAddr > dest_ip_addr = ssm_val->getDestIpAddr();
                        std::optional<std::string > dest_ipv4_addr = dest_ip_addr->getIpv4Addr();

                        if((user_service_type == "BROADCAST")) {
                            throw std::logic_error("Invalid Source-Specific Multicast (SSM) User Plane address for Broadcast Service Type");
                        }

                    }
                }
            }
        }
    }
    return true;
}

static bool resolve_src_dest_addr(const std::string &src_addr, const std::string &dest_addr, struct addrinfo **src_addrinfo, struct addrinfo **dest_addrinfo)
{
        ogs_debug("resolving SSM");
        int result;

        result = getaddrinfo(src_addr.c_str(), NULL, NULL, src_addrinfo);
        if (result) {
            ogs_error("Unable to resolve SSM source address '%s': %s", src_addr.c_str(), gai_strerror(result));
            if (src_addrinfo && *src_addrinfo) {
                freeaddrinfo(*src_addrinfo);
                *src_addrinfo = NULL;
            }
            return false;
        }

        result = getaddrinfo(dest_addr.c_str(), NULL, NULL, dest_addrinfo);
        if (result) {
            ogs_error("Unable to resolve SSM multicast destination address '%s': %s", dest_addr.c_str(), gai_strerror(result));
            if (src_addrinfo && *src_addrinfo) {
                freeaddrinfo(*src_addrinfo);
                *src_addrinfo = NULL;
            }
            if (dest_addrinfo && *dest_addrinfo) {
                freeaddrinfo(*dest_addrinfo);
                *dest_addrinfo = NULL;
            }
            return false;
        }
        return true;

}

static bool get_src_dest_of_same_addr_family(int family, struct addrinfo *src_addrinfo, struct addrinfo *dest_addrinfo, void **src_addr, void **dest_addr)
{
    while (src_addrinfo && src_addrinfo->ai_family != family) src_addrinfo = src_addrinfo->ai_next;
    if (!src_addrinfo) return false;

    while (dest_addrinfo && dest_addrinfo->ai_family != family) dest_addrinfo = dest_addrinfo->ai_next;
    if (!dest_addrinfo) return false;

    if (family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in*)src_addrinfo->ai_addr;
        *src_addr = &addr->sin_addr;
        addr = (struct sockaddr_in*)dest_addrinfo->ai_addr;
        *dest_addr =  &addr->sin_addr;
    } else if (family == AF_INET6) {
        struct sockaddr_in6 *addr = (struct sockaddr_in6*)src_addrinfo->ai_addr;
        *src_addr = &addr->sin6_addr;
        addr = (struct sockaddr_in6*)dest_addrinfo->ai_addr;
        *dest_addr = &addr->sin6_addr;
    } else {
        *src_addr = NULL;
        *dest_addr = NULL;
    }

    return true;
}

MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
