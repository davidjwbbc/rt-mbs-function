#ifndef _MBSF_USER_DATA_ING_SESSION_HH_
#define _MBSF_USER_DATA_ING_SESSION_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS User Data Ingest Session class
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

#include <any>
#include <chrono>
#include <memory>
#include <tuple>
#include <mutex>
#include "openapi/model/MBSUserDataIngSession.h"
#include "common.hh"


namespace fiveg_mag_reftools {
    class CJson;
}

namespace reftools::mbsf {
    class MBSUserDataIngSession;
    class Ssm;
    class DistSession;
}

using fiveg_mag_reftools::CJson;
using reftools::mbsf::DistSession;
using reftools::mbsf::MBSUserDataIngSession;
using reftools::mbsf::MBSDistributionSessionInfo;
using reftools::mbsf::Ssm;
using reftools::mbsf::ObjDistributionOperatingMode;
using reftools::mbsf::ObjAcquisitionMethod;

MBSF_NAMESPACE_START

class Open5GSEvent;
class Open5GSSBIRequest;
class Open5GSSBIObject;
class MBSMFMBSSession;

class UserDataIngSession {
public:
    using SysTimeMS = std::chrono::system_clock::time_point;
    using UserDataIngDistSessId = std::pair<std::string, std::string>;
    using SessionIdContainer = std::pair<std::string, std::shared_ptr< UserDataIngDistSessId >>;
    struct ContextData {
        std::string ingSessionId;
        std::string distSessionInfoKey;
        std::shared_ptr<MBSDistributionSessionInfo> info;
        std::shared_ptr<Ssm> ssm;
        std::shared_ptr<Open5GSEvent> event;
        ogs_sbi_nf_instance_t *mbstfInstance;
               ogs_sbi_xact_t *mbstfXact;
        std::shared_ptr<MBSMFMBSSession> MBSSession;
        bool hasMBSSession;
        bool receivedMBSTFResponse;
        std::string distSessionId;

        ~ContextData() {
            MBSSession.reset();
        }
    };
    UserDataIngSession(CJson &json, bool as_request);
    UserDataIngSession(CJson &json, bool as_request, std::shared_ptr<Open5GSEvent> &event);
    UserDataIngSession(const std::shared_ptr<MBSUserDataIngSession> &mbs_user_service);
    UserDataIngSession() = delete;
    UserDataIngSession(UserDataIngSession &&other) = delete;
    UserDataIngSession(const UserDataIngSession &other) = delete;
    UserDataIngSession &operator=(UserDataIngSession &&other) = delete;
    UserDataIngSession &operator=(const UserDataIngSession &other) = delete;

    virtual ~UserDataIngSession();

    CJson json(bool as_request) const;

    enum OgsExtendedEventId : int {
        MBSF_LOCAL_SEND_MBSTF_REQ_BUILD = OGS_MAX_NUM_OF_PROTO_EVENT + 1600,
        MBSF_LOCAL_SEND_MBSTF_DELETE_SESSION
    };

    static const char *localEventGetName( ogs_event_t *event);

    static const std::shared_ptr<UserDataIngSession> &find(const std::string &id); // throws std::out_of_range if id does not exist
    const std::string &userDataIngSessionId() const { return m_UserDataIngSessionId; };
    const std::shared_ptr<MBSUserDataIngSession> &getMBSUserIngSession() const {return m_MBSUserDataIngSession;};
    const SysTimeMS &generated() const {return m_generated;};
    const std::string &hash() const {return m_hash;};
    const Open5GSSBIObject *getSbiObject() {return m_sbiObject;};

    ogs_sbi_xact_t *nmbstfDiscoverOnly(std::shared_ptr< ContextData > data);
    void processDistributionSessionInfo(std::shared_ptr<MBSUserDataIngSession> mbs_user_data_ing_session, std::shared_ptr<Open5GSEvent> &event);
    static bool processEvent(Open5GSEvent &event);
    const std::shared_ptr<UserDataIngSession> &findSessionBySbiObject(const std::shared_ptr<Open5GSSBIObject>& sbi_obj);
    static bool handleMbstfDiscover(ogs_sbi_nf_instance_t *nf_instance, ogs_sbi_xact_t *xact);

    static bool processDistSession(std::shared_ptr< DistSession > dist_session);
    static std::shared_ptr< ObjDistributionOperatingMode > getOperatingMode(std::shared_ptr<MBSDistributionSessionInfo> info);
    static std::shared_ptr< ObjAcquisitionMethod > getAcquisitionMethod(std::shared_ptr<MBSDistributionSessionInfo> info);
    static std::optional<std::string> getObjectIngestUrl(std::shared_ptr<MBSDistributionSessionInfo> info);
    static std::optional<std::string> getObjectDistributionUrl(std::shared_ptr<MBSDistributionSessionInfo> info);
    static std::list<std::optional<std::string >, fiveg_mag_reftools::OgsAllocator<std::optional<std::string > > > getObjectAcquisitionIds(std::shared_ptr<MBSDistributionSessionInfo> info);
    static std::string maxContBitRate(std::shared_ptr<MBSDistributionSessionInfo> info);
    static bool tmgi(std::string mbs_service_id, std::string mcc, std::string mnc, void *data);

    void addToDistributionSessionInfos(const std::string &key, const std::shared_ptr< ContextData > context);
    std::shared_ptr< UserDataIngSession::ContextData > getDistributionSessionInfoData(std::string &key);
    void removeDistributionSessionInfo(std::string &key);
    static void removeDistributionSessionInfos(void *data);
    void clearDistributionSessionInfos();

    static std::shared_ptr< ContextData > getContextData(std::shared_ptr<UserDataIngDistSessId> &ids);
    void removeContextData(std::shared_ptr<ContextData> context_data);

    void sendMbstfRequests();
    void sendMbstfDeleteRequestsOnError();
    void sendLocalEvent(OgsExtendedEventId event_id, void *data);
    bool sendNmbsfMbsUserDataIngestResponse(std::shared_ptr<UserDataIngSession::UserDataIngDistSessId> &ids);


    void checkIfAllMBSSessionCreated();
    static void setMBSSessionFlag(void *data);
    bool checkIfAllMBSTFResponsesReceived();
    static void populateAndSendError(void *data, const std::optional<fiveg_mag_reftools::ProblemCause> &cause = std::nullopt, const std::optional<CJson> &problem_detail_json = std::nullopt);
    static void deleteMBSTFSession(ogs_sbi_xact_t *xact);

    void addToRegistry(ogs_sbi_xact_t* xact, std::shared_ptr< UserDataIngDistSessId > &ids);
    void removeFromRegistry(ogs_sbi_xact_t* xact);
    std::shared_ptr< UserDataIngDistSessId > getFromRegistry(ogs_sbi_xact_t* xact);

    void addToRegistry(std::string &dist_session_id, std::shared_ptr< UserDataIngDistSessId > &ids);
    void removeFromRegistry(std::string &dist_session_id);
    std::shared_ptr< UserDataIngDistSessId > getFromRegistry(std::string &dist_session_id);

    static std::recursive_mutex m_mutex;
    static std::map<ogs_sbi_xact_t *, std::shared_ptr< UserDataIngDistSessId >> m_xactRegistry;
    static std::map<std::string, std::shared_ptr< UserDataIngDistSessId >> s_distSessionIdRegistry;

private:
    std::shared_ptr<MBSUserDataIngSession> m_MBSUserDataIngSession;
    SysTimeMS m_generated;
    SysTimeMS m_lastUsed;
    std::string m_hash;
    std::string m_UserDataIngSessionId;
    Open5GSSBIObject *m_sbiObject;
    std::recursive_mutex m_rmutex;
    std::map<std::string, std::shared_ptr< ContextData >> m_distributionSessionInfos;

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_USER_DATA_ING_SESSION_HH_ */
