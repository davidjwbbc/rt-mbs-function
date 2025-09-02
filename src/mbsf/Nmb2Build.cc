/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: Nmb2 Build class
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
#include <random>
#include <uuid/uuid.h>

// App header includes
#include "common.hh"
#include "App.hh"
#include "Context.hh"
#include "hash.hh"
#include "MBSFNetworkFunction.hh"
#include "MBSMFMBSSession.hh"
#include "NfServer.hh"
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
#include "openapi/model/DistSessionState.h"
#include "openapi/model/IpAddr.h"
#include "openapi/model/Ipv6Addr.h"
#include "openapi/model/TunnelAddress.h"
#include "openapi/model/MBSUserDataIngSession.h"
#include "openapi/model/MBSDistributionSessionInfo.h"
#include "openapi/model/ObjDistributionData.h"
#include "openapi/model/Ssm.h"
#include "openapi/model/UpTrafficFlowInfo.h"
#include "openapi/model/MbsSessionId.h"
#include "openapi/model/MbsServiceType.h"
#include "openapi/model/ObjDistributionData.h"

#include "openapi/api/IndividualMBSUserDataIngestSessionDocumentApi-info.h"
#include "mb-smf-service-consumer.h"
#include "TimerFunc.hh"

#include "UserDataIngSession.hh"

// Header include for this class

#include "Nmb2Build.hh"

using fiveg_mag_reftools::CJson;
using reftools::mbsf::DistSession;
using reftools::mbsf::DistSessionState;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::TunnelAddress;
using reftools::mbsf::Ssm;
using reftools::mbsf::MbsSessionId;
using reftools::mbsf::MbsServiceType;
using reftools::mbsf::IpAddr;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjAcquisitionMethod;
using reftools::mbsf::ObjDistributionData;
using reftools::mbsf::UpTrafficFlowInfo;
using reftools::mbsf::Ipv6Addr;


MBSF_NAMESPACE_START

static ogs_sbi_request_t *nmbstf_build_request(void *data);
static std::optional<std::shared_ptr< Ssm > > ssm(CJson &json, bool as_request);

static std::shared_ptr< ObjDistributionData > populate_mbstf_obj_distribution_data(std::shared_ptr<MBSDistributionSessionInfo> mbs_dist_session_info);
static std::shared_ptr< TunnelAddress > populate_mbstf_mb_upf_tunnel_addr(ogs_sockaddr_t *tunnel_addr);
static std::shared_ptr< UpTrafficFlowInfo > populate_mbstf_up_traffic_flow_info(std::shared_ptr< IpAddr > dest_addr);
static std::shared_ptr< DistSessionState > populate_mbstf_dist_session_state();
static std::string generate_uuid();

#if 0
std::shared_ptr<Open5GSSBIRequest> UserDataIngSession::buildNmbstfRequest()
{
    const std::string data = R"({
  "distSession": {
    "distSessionId": "f2db3860-b06d-43a0-bb9b-c94b40f78c58",
    "distSessionState": "ACTIVE",
    "mbUpfTunAddr": {
      "ipv4Addr": "127.0.0.7",
      "portNumber": 5454
    },
    "upTrafficFlowInfo": {
      "destIpAddr": {
        "ipv4Addr": "232.0.0.1"
      },
      "portNumber": 8081
    },
    "mbr": "10 Mbps",
    "objDistributionData": {
      "objDistributionOperatingMode": "STREAMING",
      "objAcquisitionMethod": "PULL",
      "objIngestBaseUrl": "http://media-origin-server:8080/",
      "objAcquisitionIdsPull": [
        "pc_hd_abr_v2_http.mpd"
      ],
      "objDistributionBaseUrl": "http://localhost:8081/"
    }
  }
})";

    const std::string type = "application/json";
     std::shared_ptr<Open5GSSBIRequest> request;


    //Open5GSSBIRequest(const std::string &method, const std::string &uri, const std::string &apiVersion, const std::optional<std::string> &data, const std::optional<std::string> &type);
    request.reset( new Open5GSSBIRequest("POST", "nmbstf-distsession/v1/dist-sessions", OGS_SBI_API_V1, std::optional<std::string> {data}, std::optional<std::string>{type}));

    return request;
}

#endif
#if 0
ogs_sbi_request_t *NMb2Build::buildNmb2DistSession(void *data)
{

    Open5GSSBIClient mbstf_client(reinterpret_cast<ogs_sbi_client_t*>(NF_INSTANCE_CLIENT(nf_instance)));
    Open5GSSBIObject sbi_object(xact->sbi_object);
    std::shared_ptr<MBSMFMBSSession> mb_smf_mbs_session;
    ogs_info("HANDLER XACT: %p, SBI OBJ IN XACT: %p", xact, xact->sbi_object);
    std::string src_ipv4_addr;
    std::string src_ipv6_addr;
    /*
    if(doesSBIObjectMatch(xact)) {
        ogs_info("NO MATCH: RET SBI OBJECT");
        return false;
    }
    */

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
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            auto it = m_xactRegistry.find(xact);
            if (it != m_xactRegistry.end()) {
                context_data = it->second;
            }
        }
        if(!context_data) {
            ogs_info("UNABLE TO GET CONTEXT DATA FROM REGISTRY");
            return false;
        }
        //auto [self, ssm_ptr, event, nf_inst] = *context_data;
        context_data->mbstfInstance = nf_instance;
        context_data->mbstfXact = xact;

        std::shared_ptr<Ssm> ssm_ptr = context_data->ssm;
        if(!ssm_ptr) ogs_error("Unable to get SSM from Context Data");

        //std::shared_ptr< IpAddr > src_ip_addr = ssm_ptr->getSourceIpAddr();
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

       //Add to context
       if(mb_smf_mbs_session->ssm()) {

           mb_smf_mbs_session->setTunnelRequest(true);
           mb_smf_mbs_session->setTmgiRequest(true);

           mb_smf_mbs_session->setServiceType(MBS_SERVICE_TYPE_MULTICAST);
           if(!context_data->MBSSession) context_data->MBSSession = mb_smf_mbs_session;
           mb_smf_mbs_session->setCreatedCallback(reinterpret_cast<void*>(context_data.get()));

           mb_smf_mbs_session->setMBSTFInstance(nf_instance);
           mb_smf_mbs_session->setXact(xact);
           App::self().context()->addMbSmfMbsSession(mb_smf_mbs_session);

           mb_smf_mbs_session->pushChanges();
       } else {
           ogs_info("MB-SMF SSM IS NULL");
       }


    return true;

}
#endif

ogs_sbi_request_t *Nmb2Build::buildNmb2DistSession(void *data) {

    ogs_sbi_message_t msg;

    std::shared_ptr< ObjDistributionData > mbstf_obj_distribution_data = nullptr;
    std::shared_ptr< UpTrafficFlowInfo > mbstf_up_traffic_flow_info = nullptr;
    std::shared_ptr< TunnelAddress > mbstf_mb_upf_tunnel_addr = nullptr;
    std::shared_ptr< DistSessionState > dist_session_state = nullptr;

    std::shared_ptr< DistSession > dist_session = nullptr;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_POST;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSTF_DISTSESSION;
    msg.h.api.version = (char *)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char *)ogs_strdup("dist-sessions");

    UserDataIngSession::UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngSession::UserDataIngDistSessId*>(data);
    std::shared_ptr< UserDataIngSession::UserDataIngDistSessId> ids_ptr = std::make_shared<UserDataIngSession::UserDataIngDistSessId>(*ids);

    std::shared_ptr<UserDataIngSession> ing_session = UserDataIngSession::find(ids_ptr->first);

    //UserDataIngSession::ContextData *context_data = reinterpret_cast<UserDataIngSession::ContextData*>(data);
    std::shared_ptr< UserDataIngSession::ContextData > context_data_ptr = UserDataIngSession::getContextData(ids_ptr);

    //auto context_data_ptr = std::make_shared<UserDataIngSession::ContextData>(*context_data);

    mbstf_obj_distribution_data = populate_mbstf_obj_distribution_data(context_data_ptr->info);

    std::shared_ptr< IpAddr  > dest_addr = context_data_ptr->ssm->getDestIpAddr();
    mbstf_up_traffic_flow_info = populate_mbstf_up_traffic_flow_info(dest_addr);


    ogs_sockaddr_t *tunnel_addr = context_data_ptr->MBSSession->tunnelAddr();
    mbstf_mb_upf_tunnel_addr = populate_mbstf_mb_upf_tunnel_addr(tunnel_addr);

    dist_session_state = populate_mbstf_dist_session_state();


    std::string mbr = std::string(UserDataIngSession::maxContBitRate(context_data_ptr->info));

    dist_session.reset(new DistSession());
    dist_session->setObjDistributionData(mbstf_obj_distribution_data);
    dist_session->setUpTrafficFlowInfo(mbstf_up_traffic_flow_info);
    dist_session->setMbUpfTunAddr(mbstf_mb_upf_tunnel_addr);
    dist_session->setDistSessionState(dist_session_state);
    dist_session->setMbr(mbr);

    std::string session_id = generate_uuid();
    dist_session->setDistSessionId(session_id);

    ing_session->addToRegistry(session_id, ids_ptr);

    CJson json = dist_session->toJSON();
    std::string body = std::string(json.serialise());

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);

    ogs_sbi_header_set(req->http.headers, OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
    req->http.content = const_cast<char*>(body.c_str());
    req->http.content_length = body.size();

    ogs_info("DIST SESSION: %s", body.c_str());

    return req;

}


bool Nmb2Build::sendNmb2DistSession(void *data) {

    ogs_sbi_message_t msg;

    std::shared_ptr< ObjDistributionData > mbstf_obj_distribution_data = nullptr;
    std::shared_ptr< UpTrafficFlowInfo > mbstf_up_traffic_flow_info = nullptr;
    std::shared_ptr< TunnelAddress > mbstf_mb_upf_tunnel_addr = nullptr;
    std::shared_ptr< DistSessionState > dist_session_state = nullptr;

    std::shared_ptr< DistSession > dist_session = nullptr;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_POST;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSTF_DISTSESSION;
    msg.h.api.version = (char *)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char *)ogs_strdup("dist-sessions");

    //UserDataIngSession::ContextData *context_data = reinterpret_cast<UserDataIngSession::ContextData*>(data);
    //auto context_data_ptr = std::make_shared<UserDataIngSession::ContextData>(*context_data);

    UserDataIngSession::UserDataIngDistSessId *ids = reinterpret_cast<UserDataIngSession::UserDataIngDistSessId*>(data);
    auto ids_ptr = std::make_shared<UserDataIngSession::UserDataIngDistSessId>(*ids);

    std::shared_ptr<UserDataIngSession> ing_session = UserDataIngSession::find(ids_ptr->first);

    std::shared_ptr< UserDataIngSession::ContextData > context_data_ptr = UserDataIngSession::getContextData(ids_ptr);

    mbstf_obj_distribution_data = populate_mbstf_obj_distribution_data(context_data_ptr->info);

    std::shared_ptr< IpAddr  > dest_addr = context_data_ptr->ssm->getDestIpAddr();
    mbstf_up_traffic_flow_info = populate_mbstf_up_traffic_flow_info(dest_addr);


    ogs_sockaddr_t *tunnel_addr = context_data_ptr->MBSSession->tunnelAddr();
    mbstf_mb_upf_tunnel_addr = populate_mbstf_mb_upf_tunnel_addr(tunnel_addr);

    dist_session_state = populate_mbstf_dist_session_state();


    std::string mbr = std::string(UserDataIngSession::maxContBitRate(context_data_ptr->info));

    dist_session.reset(new DistSession());
    dist_session->setObjDistributionData(mbstf_obj_distribution_data);
    dist_session->setUpTrafficFlowInfo(mbstf_up_traffic_flow_info);
    dist_session->setMbUpfTunAddr(mbstf_mb_upf_tunnel_addr);
    dist_session->setDistSessionState(dist_session_state);
    dist_session->setMbr(mbr);

    std::string session_id = generate_uuid();
    dist_session->setDistSessionId(session_id);

    ing_session->addToRegistry(session_id, ids_ptr);

    CJson json = dist_session->toJSON();
    std::string body = std::string(json.serialise());

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);

    ogs_sbi_header_set(req->http.headers, OGS_SBI_CONTENT_TYPE, OGS_SBI_CONTENT_JSON_TYPE);
    req->http.content = const_cast<char*>(body.c_str());
    req->http.content_length = body.size();

    ogs_info("DIST SESSION: %s", body.c_str());

    ogs_assert(context_data_ptr->mbstfInstance);
    ogs_assert(context_data_ptr->mbstfXact);

    context_data_ptr->mbstfXact->request = req;

    return ogs_sbi_send_request_to_nf_instance(context_data_ptr->mbstfInstance, context_data_ptr->mbstfXact);

    //return req;

}

bool Nmb2Build::sendNmb2DistSessionDelete(void *data) {

    ogs_sbi_message_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.h.method = (char*)OGS_SBI_HTTP_METHOD_DELETE;
    msg.h.service.name = (char*)OGS_SBI_SERVICE_NAME_NMBSTF_DISTSESSION;
    msg.h.api.version = (char *)OGS_SBI_API_V1;
    msg.h.resource.component[0] = (char *)ogs_strdup("dist-sessions");

    auto *session_ids = reinterpret_cast<UserDataIngSession::SessionIdContainer*>(data);

    const std::string &dist_session_id = session_ids->first;
    std::shared_ptr< UserDataIngSession::ContextData > context_data_ptr = UserDataIngSession::getContextData(session_ids->second);

    msg.h.resource.component[1] = const_cast<char*>(dist_session_id.c_str());

    ogs_sbi_request_t *req = ogs_sbi_build_request(&msg);
    context_data_ptr->mbstfXact->request = req;

    delete session_ids;
    return ogs_sbi_send_request_to_nf_instance(context_data_ptr->mbstfInstance, context_data_ptr->mbstfXact);

}



std::shared_ptr< UpTrafficFlowInfo > populate_mbstf_up_traffic_flow_info(std::shared_ptr< IpAddr > dest_addr)
{
    std::shared_ptr< UpTrafficFlowInfo > flow_info = nullptr;

    static std::random_device rd;
    int32_t port =static_cast<int32_t>(rd());

    flow_info.reset(new UpTrafficFlowInfo() );
    flow_info->setDestIpAddr(dest_addr);
    flow_info->setPortNumber(port);

    return flow_info;
}

std::shared_ptr < TunnelAddress > populate_mbstf_mb_upf_tunnel_addr(ogs_sockaddr_t *tunnel_addr)
{
    ogs_sockaddr_t *sa;
    char buf[OGS_ADDRSTRLEN + 1];

    std::shared_ptr< TunnelAddress > tun_addr = nullptr;
    tun_addr.reset(new TunnelAddress());

    for (sa = tunnel_addr; sa; sa = sa->next) {
        if(sa->ogs_sa_family == AF_INET) {
          OGS_ADDR(sa, buf);
          std::string ipv4_addr = std::string(buf, strnlen(buf, sizeof(buf)));
          tun_addr->setIpv4Addr(ipv4_addr);
          ogs_info("  UDP Tunnel = %s:%u", buf, OGS_PORT(sa));
          ogs_info("Recieved IPv4 tunnel address");
        } else if(sa->ogs_sa_family == AF_INET6) {
            std::shared_ptr < Ipv6Addr > ipv6_addr;

            OGS_ADDR(sa, buf);

            ipv6_addr.reset(new Ipv6Addr(buf, std::string::size_type(strnlen(buf, sizeof(buf)))));

            //ipv6_addr.reset(new std::string(buf, strnlen(buf, sizeof(buf))));

            tun_addr->setIpv6Addr(std::make_optional(ipv6_addr));
        }
        tun_addr->setPortNumber(OGS_PORT(sa));
    }
    return tun_addr;
}

static std::shared_ptr< ObjDistributionData > populate_mbstf_obj_distribution_data(std::shared_ptr<MBSDistributionSessionInfo> mbs_dist_session_info)
{

    std::shared_ptr< ObjDistributionOperatingMode > operating_mode = UserDataIngSession::getOperatingMode(mbs_dist_session_info);

    std::shared_ptr< ObjAcquisitionMethod > acquisition_method = UserDataIngSession::getAcquisitionMethod(mbs_dist_session_info);

    std::optional<std::string> obj_ingest_url = UserDataIngSession::getObjectIngestUrl(mbs_dist_session_info);

    std::optional<std::string> obj_disribution_url = UserDataIngSession::getObjectDistributionUrl(mbs_dist_session_info);

    std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > >
            obj_acquisition_ids = UserDataIngSession::getObjectAcquisitionIds(mbs_dist_session_info);


    std::shared_ptr< ObjDistributionData > mbstf_obj_dist_data = nullptr;
    mbstf_obj_dist_data.reset(new ObjDistributionData());

    mbstf_obj_dist_data->setObjDistributionOperatingMode(operating_mode);
    mbstf_obj_dist_data->setObjAcquisitionMethod(acquisition_method);
    mbstf_obj_dist_data->setObjIngestBaseUrl(obj_ingest_url);
    mbstf_obj_dist_data->setObjDistributionBaseUrl(obj_disribution_url);
    if(acquisition_method->getString() == "PULL") {
        mbstf_obj_dist_data->setObjAcquisitionIdsPull(obj_acquisition_ids);
    } else if(acquisition_method->getString() == "PUSH") {
        if (!obj_acquisition_ids.empty()) {
            mbstf_obj_dist_data->setObjAcquisitionIdPush(obj_acquisition_ids.front());
        }
    }

    return mbstf_obj_dist_data;

}

static std::shared_ptr< DistSessionState > populate_mbstf_dist_session_state()
{
    std::shared_ptr< DistSessionState > session_state = nullptr;
    session_state.reset(new DistSessionState());
    *session_state =  DistSessionState::VAL_ACTIVE;
    return session_state;
}

static std::string generate_uuid() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}




static ogs_sbi_request_t *nmbstf_build_request(void *data)
{
    char *method = (char *)OGS_SBI_HTTP_METHOD_GET;
    char *service_name = (char *)OGS_SBI_SERVICE_NAME_NMBSTF_DISTSESSION;
    char *api_version = (char *)OGS_SBI_API_V1;

    Open5GSSBIMessage message;

    message.method(method);
    message.serviceName(service_name);
    message.apiVersion(api_version);
    message.resourceComponent(0, (char *)OGS_SBI_RESOURCE_NAME_PCF_BINDINGS);
    Open5GSSBIRequest request(message);
    return request.ogsSBIRequest();
}


MBSF_NAMESPACE_STOP

/* vim:ts=8:sts=4:sw=4:expandtab:
 */
