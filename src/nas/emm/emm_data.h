/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the terms found in the LICENSE file in the root of this source tree.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*****************************************************************************
Source      emm_data.h

Version     0.1

Date        2012/10/18

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines internal private data handled by EPS Mobility
        Management sublayer.

*****************************************************************************/
#ifndef FILE_EMM_DATA_SEEN
#define FILE_EMM_DATA_SEEN

#include <sys/types.h>

#include "3gpp_24.008.h"
#include "3gpp_24.301.h"
#include "hashtable.h"
#include "obj_hashtable.h"
#include "queue.h"
#include "securityDef.h"

#include "AdditionalUpdateType.h"
#include "EpsBearerContextStatus.h"
#include "EpsNetworkFeatureSupport.h"
#include "TrackingAreaIdentityList.h"
#include "UeNetworkCapability.h"
#include "emm_fsm.h"
#include "mme_api.h"
#include "nas_emm_procedures.h"
#include "nas_timer.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

#define TIMER_S6A_AUTH_INFO_RSP_DEFAULT_VALUE \
  2  // two second timeout value to wait for auth_info_rsp message from HSS
#define TIMER_S10_CONTEXT_REQ_DEFAULT_VALUE \
  5  // two second timeout value to wait for context_rsp message from source MME
#define TIMER_SPECIFIC_RETRY_DEFAULT_VALUE \
  0  // two second timeout value to wait for context_rsp message from source MME
#define TIMER_SPECIFIC_RETRY_DEFAULT_VALUE_USEC \
  300000  // two second timeout value to wait for context_rsp message from
          // source MME

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* EPS NAS security context structure
 * EPS NAS security context: This context consists of K ASME with the associated
 key set identifier, the UE security
 * capabilities, and the uplink and downlink NAS COUNT values. In particular,
 separate pairs of NAS COUNT values are
 * used for each EPS NAS security contexts, respectively. The distinction
 between native and mapped EPS security
 * contexts also applies to EPS NAS security contexts. The EPS NAS security
 context is called 'full' if it additionally
 * contains the keys K NASint and K NASenc and the identifiers of the selected
 NAS integrity and encryption algorithms.
 *
  // Authentication Vector Temporary authentication and key agreement data that
 enables an MME to
  // engage in AKA with a particular user. An EPS Authentication Vector consists
 of four elements:
  // a) network challenge RAND,
  // b) an expected response XRES,
  // c) Key K ASME ,
  // d) a network authentication token AUTN.
 */
typedef struct emm_security_context_s {
  emm_sc_type_t sc_type; /* Type of security context        */
  /* state of security context is implicit due to its storage location
   * (current/non-current)*/
#define EKSI_MAX_VALUE 6
  ksi_t eksi; /* NAS key set identifier for E-UTRAN      */
#define EMM_SECURITY_VECTOR_INDEX_INVALID (-1)
  int vector_index;                     /* Pointer on vector */
  uint8_t knas_enc[AUTH_KNAS_ENC_SIZE]; /* NAS cyphering key               */
  uint8_t knas_int[AUTH_KNAS_INT_SIZE]; /* NAS integrity key               */
  uint8_t ncc : 3;               /* next hop chaining counter for handover. */
  uint8_t nh_conj[AUTH_NH_SIZE]; /* nh */

  count_t dl_count, ul_count; /* Downlink and uplink count parameters    */
  struct {
    uint8_t eps_encryption;  /* algorithm used for ciphering            */
    uint8_t eps_integrity;   /* algorithm used for integrity protection */
    uint8_t umts_encryption; /* algorithm used for ciphering            */
    uint8_t umts_integrity;  /* algorithm used for integrity protection */
    uint8_t gprs_encryption; /* algorithm used for ciphering            */
    bool umts_present;
    bool gprs_present;
  } capability; /* UE network capability           */
  struct {
    uint8_t encryption : 4; /* algorithm used for ciphering           */
    uint8_t integrity : 4;  /* algorithm used for integrity protection */
  } selected_algorithms;    /* MME selected algorithms                */

  // Requirement MME24.301R10_4.4.4.3_2 (DETACH REQUEST (if sent before security
  // has been activated);)
  uint8_t activated;
  uint8_t direction_encode;  // SECU_DIRECTION_DOWNLINK, SECU_DIRECTION_UPLINK
  uint8_t direction_decode;  // SECU_DIRECTION_DOWNLINK, SECU_DIRECTION_UPLINK
} emm_security_context_t;

/*
 * --------------------------------------------------------------------------
 *  EMM internal data handled by EPS Mobility Management sublayer in the MME
 * --------------------------------------------------------------------------
 */
struct emm_common_data_s;

/*
 * Structure of the EMM context established by the network for a particular UE
 * ---------------------------------------------------------------------------
 */
typedef struct emm_data_context_s {
  mme_ue_s1ap_id_t ue_id; /* UE identifier                                  */
  bool is_dynamic;        /* Dynamically allocated context indicator         */
  bool is_emergency;      /* Emergency bearer services indicator             */
  bool is_has_been_attached;      /* Attachment indicator                   */
  bool is_initial_identity_imsi;  // If the IMSI was used for identification in
                                  // the initial NAS message
  bool is_guti_based_attach;
  emm_procedures_t* emm_procedures;
  /*
   * attach_type has type emm_proc_attach_type_t.
   *
   * Here, it is un-typedef'ed as uint8_t to avoid circular dependency issues.
   */
  uint8_t attach_type; /* EPS/Combined/etc. */
  additional_update_type_t additional_update_type;

  uint32_t member_present_mask; /* bitmask, see significance of bits below */
  uint32_t member_valid_mask;   /* bitmask, see significance of bits below */
#define EMM_CTXT_MEMBER_IMSI ((uint32_t)1 << 0)
#define EMM_CTXT_MEMBER_IMEI ((uint32_t)1 << 1)
#define EMM_CTXT_MEMBER_IMEI_SV ((uint32_t)1 << 2)
#define EMM_CTXT_MEMBER_OLD_GUTI ((uint32_t)1 << 3)
#define EMM_CTXT_MEMBER_GUTI ((uint32_t)1 << 4)
#define EMM_CTXT_MEMBER_TAI_LIST ((uint32_t)1 << 5)
#define EMM_CTXT_MEMBER_LVR_TAI ((uint32_t)1 << 6)
#define EMM_CTXT_MEMBER_AUTH_VECTORS ((uint32_t)1 << 7)
#define EMM_CTXT_MEMBER_SECURITY ((uint32_t)1 << 8)
#define EMM_CTXT_MEMBER_NON_CURRENT_SECURITY ((uint32_t)1 << 9)
#define EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE ((uint32_t)1 << 10)
#define EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE ((uint32_t)1 << 11)
#define EMM_CTXT_MEMBER_CURRENT_DRX_PARAMETER ((uint32_t)1 << 12)
#define EMM_CTXT_MEMBER_PENDING_DRX_PARAMETER ((uint32_t)1 << 13)
#define EMM_CTXT_MEMBER_EPS_BEARER_CONTEXT_STATUS ((uint32_t)1 << 14)

#define EMM_CTXT_MEMBER_AUTH_VECTOR0 ((uint32_t)1 << 26)
  //#define           EMM_CTXT_MEMBER_AUTH_VECTOR1                 ((uint32_t)1
  //<< 27)  // reserved bit for AUTH VECTOR #define EMM_CTXT_MEMBER_AUTH_VECTOR2
  //((uint32_t)1 << 28)  // reserved bit for AUTH VECTOR #define
  // EMM_CTXT_MEMBER_AUTH_VECTOR3                 ((uint32_t)1 << 29)  //
  // reserved bit for AUTH VECTOR #define           EMM_CTXT_MEMBER_AUTH_VECTOR4
  //((uint32_t)1 << 30)  // reserved bit for AUTH VECTOR #define
  // EMM_CTXT_MEMBER_AUTH_VECTOR5                 ((uint32_t)1 << 31)  //
  // reserved bit for AUTH VECTOR

#define EMM_CTXT_MEMBER_SET_BIT(eMmCtXtMemBeRmAsK, bIt) \
  do {                                                  \
    (eMmCtXtMemBeRmAsK) |= bIt;                         \
  } while (0)
#define EMM_CTXT_MEMBER_CLEAR_BIT(eMmCtXtMemBeRmAsK, bIt) \
  do {                                                    \
    (eMmCtXtMemBeRmAsK) &= ~bIt;                          \
  } while (0)

  // imsi present mean we know it but was not checked with identity proc, or was
  // not provided in initial message
  imsi_t _imsi;     /* The IMSI provided by the UE or the MME, set valid when
                       identification returns IMSI */
  imsi64_t _imsi64; /* The IMSI provided by the UE or the MME, set valid when
                       identification returns IMSI */
  imsi64_t saved_imsi64; /* Useful for 5.4.2.7.c */
  imei_t _imei;          /* The IMEI provided by the UE                     */
  imeisv_t _imeisv;      /* The IMEISV provided by the UE                   */
  // bool                   _guti_is_new; /* The GUTI assigned to the UE is new
  // */
  guti_t _guti;         /* The GUTI assigned to the UE                     */
  guti_t _old_guti;     /* The old GUTI (GUTI REALLOCATION)                */
  tai_list_t _tai_list; /* TACs the the UE is registered to                */
  tai_t _lvr_tai;
  tai_t originating_tai;

  ksi_t ksi; /*key set identifier  */
  ue_network_capability_t _ue_network_capability;
  ms_network_capability_t _ms_network_capability;
  drx_parameter_t _drx_parameter;

  int remaining_vectors;                       // remaining unused vectors
  auth_vector_t _vector[MAX_EPS_AUTH_VECTORS]; /* EPS authentication vector */
  emm_security_context_t
      _security; /* Current EPS security context: The security context which has
                    been activated most recently. Note that a current EPS
                    security context originating from either a mapped or native
                    EPS security context may exist simultaneously with a native
                    non-current EPS security context.*/

#define EMM_AUTHENTICATION_SYNC_FAILURE_MAX 2
  int auth_sync_fail_count; /* counter of successive AUTHENTICATION FAILURE
                               messages from the UE with EMM cause #21 "synch
                               failure" */

  // Requirement MME24.301R10_4.4.2.1_2
  emm_security_context_t
      _non_current_security; /* Non-current EPS security context: A native EPS
                                security context that is not the current one. A
                                non-current EPS security context may be stored
                                along with a current EPS security context in the
                                UE and the MME. A non-current EPS security
                                context does not contain an EPS AS security
                                context. A non-current EPS security context is
                                either of type 'full
                                native' or of type 'partial native'.     */

  int emm_cause; /* EMM failure cause code                          */

  emm_fsm_state_t _emm_fsm_state;

  drx_parameter_t _current_drx_parameter; /* stored TAU Request IE Requirement
                                             MME24.301R10_5.5.3.2.4_4*/
#define EMM_CN_SAP_BUFFER_SIZE 4096

#define IS_EMM_CTXT_PRESENT_IMSI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_IMSI))
#define IS_EMM_CTXT_PRESENT_IMEI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_IMEI))
#define IS_EMM_CTXT_PRESENT_IMEISV(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_IMEI_SV))
#define IS_EMM_CTXT_PRESENT_OLD_GUTI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_OLD_GUTI))
#define IS_EMM_CTXT_PRESENT_GUTI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_GUTI))
#define IS_EMM_CTXT_PRESENT_TAI_LIST(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_TAI_LIST))
#define IS_EMM_CTXT_PRESENT_LVR_TAI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_LVR_TAI))
#define IS_EMM_CTXT_PRESENT_AUTH_VECTORS(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_AUTH_VECTORS))
#define IS_EMM_CTXT_PRESENT_SECURITY(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_SECURITY))
#define IS_EMM_CTXT_PRESENT_NON_CURRENT_SECURITY(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask & EMM_CTXT_MEMBER_NON_CURRENT_SECURITY))
#define IS_EMM_CTXT_PRESENT_UE_NETWORK_CAPABILITY(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask &                     \
      EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE))
#define IS_EMM_CTXT_PRESENT_MS_NETWORK_CAPABILITY(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_present_mask &                     \
      EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE))

#define IS_EMM_CTXT_PRESENT_AUTH_VECTOR(eMmCtXtPtR, KsI) \
  (!!((eMmCtXtPtR)->member_present_mask &                \
      ((EMM_CTXT_MEMBER_AUTH_VECTOR0) << KsI)))

#define IS_EMM_CTXT_VALID_IMSI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_IMSI))
#define IS_EMM_CTXT_VALID_IMEI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_IMEI))
#define IS_EMM_CTXT_VALID_IMEISV(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_IMEI_SV))
#define IS_EMM_CTXT_VALID_OLD_GUTI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_OLD_GUTI))
#define IS_EMM_CTXT_VALID_GUTI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_GUTI))
#define IS_EMM_CTXT_VALID_TAI_LIST(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_TAI_LIST))
#define IS_EMM_CTXT_VALID_LVR_TAI(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_LVR_TAI))
#define IS_EMM_CTXT_VALID_AUTH_VECTORS(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_AUTH_VECTORS))
#define IS_EMM_CTXT_VALID_SECURITY(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_SECURITY))
#define IS_EMM_CTXT_VALID_NON_CURRENT_SECURITY(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_NON_CURRENT_SECURITY))
#define IS_EMM_CTXT_VALID_UE_NETWORK_CAPABILITY(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask &                     \
      EMM_CTXT_MEMBER_UE_NETWORK_CAPABILITY_IE))
#define IS_EMM_CTXT_VALID_MS_NETWORK_CAPABILITY(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask &                     \
      EMM_CTXT_MEMBER_MS_NETWORK_CAPABILITY_IE))
#define IS_EMM_CTXT_VALID_CURRENT_DRX_PARAMETER(eMmCtXtPtR) \
  (!!((eMmCtXtPtR)->member_valid_mask & EMM_CTXT_MEMBER_CURRENT_DRX_PARAMETER))

#define IS_EMM_CTXT_VALID_AUTH_VECTOR(eMmCtXtPtR, KsI) \
  (!!((eMmCtXtPtR)->member_valid_mask &                \
      ((EMM_CTXT_MEMBER_AUTH_VECTOR0) << KsI)))
} emm_data_context_t;

/*
 * Structure of the EMM data
 * -------------------------
 */
typedef struct emm_data_s {
  /*
   * MME configuration
   * -----------------
   */
  mme_api_emm_config_t conf;
  /*
   * EMM contexts
   * ------------
   */
  hash_table_ts_t*
      ctx_coll_ue_id;  // key is emm ue id, data is struct emm_data_context_s
  hash_table_uint64_ts_t*
      ctx_coll_imsi;  // key is imsi_t, data is emm ue id (unsigned int)
  obj_hash_table_uint64_t*
      ctx_coll_guti;  // key is guti, data is emm ue id (unsigned int)
} emm_data_t;

/* Timer for S6a. */
typedef struct s6a_auth_info_rsp_timer_arg_s {
  mme_ue_s1ap_id_t ue_id;  // UE identifier
  bool resync;  // Indicates whether the authentication information is requested
                // due to sync failure
} s6a_auth_info_rsp_timer_arg_t;

/*
 * Timer for S10.
 * todo: this and ULR timer, here or MME_APP.
 */
typedef struct s10_ctx_rsp_timer_arg_s {
  mme_ue_s1ap_id_t ue_id;  // UE identifier
} s10_ctx_rsp_timer_arg_t;

mme_ue_s1ap_id_t emm_ctx_get_new_ue_id(const emm_data_context_t* const ctxt)
    __attribute__((nonnull));

void emm_ctx_set_attribute_present(emm_data_context_t* const ctxt,
                                   const int attribute_bit_pos)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_clear_attribute_present(emm_data_context_t* const ctxt,
                                     const int attribute_bit_pos)
    __attribute__((nonnull)) __attribute__((flatten));

void emm_ctx_set_attribute_valid(emm_data_context_t* const ctxt,
                                 const int attribute_bit_pos)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_clear_attribute_valid(emm_data_context_t* const ctxt,
                                   const int attribute_bit_pos)
    __attribute__((nonnull)) __attribute__((flatten));

void emm_ctx_clear_guti(emm_data_context_t* const ctxt) __attribute__((nonnull))
__attribute__((flatten));
void emm_ctx_set_guti(emm_data_context_t* const ctxt, guti_t* guti)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_valid_guti(emm_data_context_t* const ctxt, guti_t* guti)
    __attribute__((nonnull)) __attribute__((flatten));

void emm_ctx_clear_old_guti(emm_data_context_t* const ctxt)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_old_guti(emm_data_context_t* const ctxt, guti_t* guti)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_valid_old_guti(emm_data_context_t* const ctxt, guti_t* guti)
    __attribute__((nonnull)) __attribute__((flatten));

void emm_ctx_clear_imsi(emm_data_context_t* const ctxt) __attribute__((nonnull))
__attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_imsi(emm_data_context_t* const ctxt, imsi_t* imsi,
                      const imsi64_t imsi64) __attribute__((nonnull))
__attribute__((flatten));
void emm_ctx_set_valid_imsi(emm_data_context_t* const ctxt, imsi_t* imsi,
                            const imsi64_t imsi64) __attribute__((nonnull))
__attribute__((flatten));

void emm_ctx_clear_imei(emm_data_context_t* const ctxt) __attribute__((nonnull))
__attribute__((flatten));
void emm_ctx_set_imei(emm_data_context_t* const ctxt, imei_t* imei)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_valid_imei(emm_data_context_t* const ctxt, imei_t* imei)
    __attribute__((nonnull)) __attribute__((flatten));

void emm_ctx_clear_imeisv(emm_data_context_t* const ctxt)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_imeisv(emm_data_context_t* const ctxt, imeisv_t* imeisv)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_valid_imeisv(emm_data_context_t* const ctxt, imeisv_t* imeisv)
    __attribute__((nonnull)) __attribute__((flatten));

void emm_ctx_clear_lvr_tai(emm_data_context_t* const ctxt)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_lvr_tai(emm_data_context_t* const ctxt, tai_t* lvr_tai)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_valid_lvr_tai(emm_data_context_t* const ctxt, tai_t* lvr_tai)
    __attribute__((nonnull)) __attribute__((flatten));

void emm_ctx_clear_auth_vectors(emm_data_context_t* const ctxt)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_clear_auth_vector(emm_data_context_t* const ctxt, ksi_t eksi)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_clear_security(emm_data_context_t* const ctxt)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_security_type(emm_data_context_t* const ctxt,
                               emm_sc_type_t sc_type) __attribute__((nonnull))
__attribute__((flatten));
void emm_ctx_set_security_eksi(emm_data_context_t* const ctxt, ksi_t eksi)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_clear_security_vector_index(emm_data_context_t* const ctxt)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_set_security_vector_index(emm_data_context_t* const ctxt,
                                       int vector_index)
    __attribute__((nonnull)) __attribute__((flatten));

void emm_ctx_clear_non_current_security(emm_data_context_t* const ctxt)
    __attribute__((nonnull)) __attribute__((flatten));
void emm_ctx_clear_non_current_security_vector_index(
    emm_data_context_t* const ctxt) __attribute__((nonnull));
void emm_ctx_set_non_current_security_vector_index(
    emm_data_context_t* const ctxt, int vector_index) __attribute__((nonnull));

void emm_ctx_clear_ue_nw_cap(emm_data_context_t* const ctxt)
    __attribute__((nonnull));
void emm_ctx_set_ue_nw_cap(emm_data_context_t* const ctxt,
                           const ue_network_capability_t* const ue_nw_cap_ie)
    __attribute__((nonnull));
void emm_ctx_set_valid_ue_nw_cap(
    emm_data_context_t* const ctxt,
    const ue_network_capability_t* const ue_nw_cap_ie) __attribute__((nonnull));

void emm_ctx_clear_ms_nw_cap(emm_data_context_t* const ctxt)
    __attribute__((nonnull));
void emm_ctx_set_ms_nw_cap(emm_data_context_t* const ctxt,
                           const ms_network_capability_t* const ms_nw_cap_ie);
void emm_ctx_set_valid_ms_nw_cap(
    emm_data_context_t* const ctxt,
    const ms_network_capability_t* const ms_nw_cap_ie);

void emm_ctx_clear_drx_parameter(emm_data_context_t* const ctxt)
    __attribute__((nonnull));
void emm_ctx_set_drx_parameter(emm_data_context_t* const ctxt,
                               drx_parameter_t* drx) __attribute__((nonnull));
void emm_ctx_set_valid_drx_parameter(emm_data_context_t* const ctxt,
                                     drx_parameter_t* drx)
    __attribute__((nonnull));

/** Update the EMM context from the received MM Context during Handover/TAU
 * procedure. */
void emm_ctx_update_from_mm_eps_context(emm_data_context_t* const ctxt,
                                        void* const _mm_eps_ctxt);
void temp_sec_ctx_from_mm_eps_context(
    emm_security_context_t* const emm_sec_ctx_p, void* const _mm_eps_ctxt);

struct emm_data_context_s* emm_data_context_create(
    const mme_ue_s1ap_id_t mme_ue_s1ap_id);

struct emm_data_context_s* emm_data_context_get(emm_data_t* emm_data,
                                                const mme_ue_s1ap_id_t ue_id);
struct emm_data_context_s* emm_data_context_get_by_imsi(emm_data_t* emm_data,
                                                        imsi64_t imsi64);
struct emm_data_context_s* emm_data_context_get_by_guti(emm_data_t* emm_data,
                                                        guti_t* guti);
int emm_context_unlock(struct emm_data_context_s* emm_context_p);

struct emm_data_context_s* emm_data_context_remove(
    emm_data_t* emm_data, struct emm_data_context_s* elm, bool clear_fields);

// int emm_context_upsert_imsi (emm_data_t * emm_data, struct emm_data_context_s
// *elm) __attribute__((nonnull));

/**
 * This method updates the AS security parameters.
 * Used for handover.
 */
int emm_data_context_update_security_parameters(
    const mme_ue_s1ap_id_t ue_id, uint16_t* encryption_algorithm_capabilities,
    uint16_t* integrity_algorithm_capabilities);

void mm_ue_eps_context_update_security_parameters(
    const mme_ue_s1ap_id_t ue_id, mm_context_eps_t* mm_eps_ue_context,
    uint16_t* encryption_algorithm_capabilities,
    uint16_t* integrity_algorithm_capabilities);

void nas_start_T3450(const mme_ue_s1ap_id_t ue_id,
                     struct nas_timer_s* const T3450, time_out_t time_out_cb,
                     void* timer_callback_args);
void nas_start_T3460(const mme_ue_s1ap_id_t ue_id,
                     struct nas_timer_s* const T3460, time_out_t time_out_cb,
                     void* timer_callback_args);
void nas_start_T3470(const mme_ue_s1ap_id_t ue_id,
                     struct nas_timer_s* const T3470, time_out_t time_out_cb,
                     void* timer_callback_args);
void nas_stop_T3450(const mme_ue_s1ap_id_t ue_id,
                    struct nas_timer_s* const T3450, void* timer_callback_args);
void nas_stop_T3460(const mme_ue_s1ap_id_t ue_id,
                    struct nas_timer_s* const T3460, void* timer_callback_args);
void nas_stop_T3470(const mme_ue_s1ap_id_t ue_id,
                    struct nas_timer_s* const T3470, void* timer_callback_args);
void nas_start_Ts6a_auth_info(const mme_ue_s1ap_id_t ue_id,
                              struct nas_timer_s* const Ts6a_auth_info,
                              time_out_t time_out_cb,
                              void* timer_callback_args);
void nas_stop_Ts6a_auth_info(const mme_ue_s1ap_id_t ue_id,
                             struct nas_timer_s* const Ts6a_auth_info,
                             void* timer_callback_args);
/** S10 Timer: todo: here or in MME_APP (compare with ULR). */
void nas_start_Ts10_ctx_req(const mme_ue_s1ap_id_t ue_id,
                            struct nas_timer_s* const Ts10_auth_info,
                            time_out_t time_out_cb, void* timer_callback_args);
void nas_stop_Ts10_ctx_res(const mme_ue_s1ap_id_t ue_id,
                           struct nas_timer_s* const Ts10_ctx_res,
                           void* timer_callback_args);

void nas_start_T_retry_specific_procedure(const mme_ue_s1ap_id_t ue_id,
                                          struct nas_timer_s* const T_retry,
                                          time_out_t time_out_cb,
                                          void* timer_callback_args);
void nas_stop_T_retry_specific_procedure(const mme_ue_s1ap_id_t ue_id,
                                         struct nas_timer_s* const T_retry,
                                         void* timer_callback_args);

int emm_data_context_add(emm_data_t* emm_data, struct emm_data_context_s* elm)
    __attribute__((nonnull));
int emm_data_context_add_guti(emm_data_t* emm_data,
                              struct emm_data_context_s* elm)
    __attribute__((nonnull));
int emm_data_context_add_old_guti(emm_data_t* emm_data,
                                  struct emm_data_context_s* elm)
    __attribute__((nonnull));
int emm_data_context_add_imsi(emm_data_t* emm_data,
                              struct emm_data_context_s* elm)
    __attribute__((nonnull));
int emm_data_context_upsert_imsi(emm_data_t* emm_data,
                                 struct emm_data_context_s* elm)
    __attribute__((nonnull));

void emm_init_context(struct emm_data_context_s* const emm_ctx)
    __attribute__((nonnull));
void emm_context_free(struct emm_data_context_s* const emm_ctx)
    __attribute__((nonnull));
void emm_context_free_content(struct emm_data_context_s* const emm_ctx)
    __attribute__((nonnull));
void emm_context_free_content_except_key_fields(
    struct emm_data_context_s* const emm_ctx) __attribute__((nonnull));
void emm_context_dump(const struct emm_data_context_s* const elm_pP,
                      const uint8_t indent_spaces, bstring bstr_dump)
    __attribute__((nonnull));

int _start_context_request_procedure(struct emm_data_context_s* emm_context,
                                     nas_emm_specific_proc_t* const spec_proc,
                                     const tai_t* const last_tai,
                                     success_cb_t* _context_res_proc_success,
                                     failure_cb_t* _context_res_proc_fail);

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *      EPS mobility management data (used within EMM only)
 * --------------------------------------------------------------------------
 */
emm_data_t _emm_data;

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* FILE_EMM_DATA_SEEN*/
