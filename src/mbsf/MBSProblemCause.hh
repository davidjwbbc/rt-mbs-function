#ifndef _MBSF_PROBLEM_CAUSE_HH_
#define _MBSF_PROBLEM_CAUSE_HH_
/******************************************************************************
 * 5G-MAG Reference Tools: MBS Function: MBS Nmb2 DistSession handler
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


#include <unordered_map>

#include "ogs-app.h"
#include "ogs-proto.h"
#include "ogs-sbi.h"

#include "openapi/model/ProblemCause.hh"
#include "common.hh"


namespace fiveg_mag_reftools {
    class CJson;
}

using fiveg_mag_reftools::CJson;
using fiveg_mag_reftools::ProblemCause;

MBSF_NAMESPACE_START

class MBSProblemCause {
public:
    MBSProblemCause();
    MBSProblemCause(MBSProblemCause &&other) = delete;
    MBSProblemCause(const MBSProblemCause &other) = delete;
    MBSProblemCause &operator=(MBSProblemCause &&other) = delete;
    MBSProblemCause &operator=(const MBSProblemCause &other) = delete;

    static const fiveg_mag_reftools::ProblemCause INVALID_MBS_SERVICE_INFO;
    static const fiveg_mag_reftools::ProblemCause MBS_SERVICE_AREA_NOT_SUPPORTED;
    static const fiveg_mag_reftools::ProblemCause MBS_SERVICE_INFO_NOT_AUTHORIZED;
    static const fiveg_mag_reftools::ProblemCause MBS_DIST_SESSION_ALREADY_CREATED;
    static const fiveg_mag_reftools::ProblemCause OVERLAPPING_MBS_SERVICE_AREA;
    static const fiveg_mag_reftools::ProblemCause UNKNOWN_MBS_SERVICE_AREA;

    static inline const std::unordered_map<std::string, std::optional<ProblemCause>> propagationTable = {
        // 400 Bad Request equivalents
        { "INVALID_MBS_SERVICE_INFO",             MBSProblemCause::INVALID_MBS_SERVICE_INFO },
        { "FILTER_RESTRICTIONS_NOT_RESPECTED",    ProblemCause::UNSPECIFIED_MSG_FAILURE },
        { "ERROR_INPUT_PARAMETERS",               ProblemCause::UNSPECIFIED_MSG_FAILURE },

        // 403 Forbidden equivalents
        { "MBS_SERVICE_INFO_NOT_AUTHORIZED",      MBSProblemCause::MBS_SERVICE_INFO_NOT_AUTHORIZED },
        { "MBS_POLICY_CONTEXT_DENIED",            std::nullopt },
        { "MBS_SESSION_ALREADY_CREATED",          MBSProblemCause::MBS_DIST_SESSION_ALREADY_CREATED },
        { "OVERLAPPING_MBS_SERVICE_AREA",         MBSProblemCause::OVERLAPPING_MBS_SERVICE_AREA },

        // 404 Not Found equivalents
        { "UNKNOWN_TMGI",                         std::nullopt },
        { "UNKNOWN_MBS_SESSION",                  std::nullopt },
        { "UNKNOWN_MBS_SERVICE_AREA",             MBSProblemCause::UNKNOWN_MBS_SERVICE_AREA }
    };

    static std::optional<ProblemCause> lookup(const std::string& mbsmf_problem_cause);


private:

};

MBSF_NAMESPACE_STOP


/* vim:ts=8:sts=4:sw=4:expandtab:
 */
#endif /* _MBSF_PROBLEM_CAUSE_HH_ */
