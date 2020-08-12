#!/bin/bash

set -euo pipefail

CONFIG_DIR="/openair-mme/etc"

for c in ${CONFIG_DIR}/*.conf; do
    sed -i -e "s#@TAC-LB#@TAC_LB#" -e "s#TAC-HB_#TAC_HB_#" ${c}
    # grep variable names (format: ${VAR}) from template to be rendered
    VARS=$(grep -oP '@[a-zA-Z0-9_]+@' ${c} | sort | uniq | xargs)

    # create sed expressions for substituting each occurrence of ${VAR}
    # with the value of the environment variable "VAR"
    EXPRESSIONS=""
    for v in ${VARS}; do
        NEW_VAR=`echo $v | sed -e "s#@##g"`
        if [[ "${!NEW_VAR}x" == "x" ]]; then
            echo "Error: Environment variable '${NEW_VAR}' is not set." \
                "Config file '$(basename $c)' requires all of $VARS."
            exit 1
        fi
        # Some fields require CIDR format
        if [[ "${NEW_VAR}" == "MME_IPV4_ADDRESS_FOR_S1_MME" ]] || \
           [[ "${NEW_VAR}" == "MME_IPV4_ADDRESS_FOR_S11" ]] || \
           [[ "${NEW_VAR}" == "MME_IPV4_ADDRESS_FOR_S10" ]]; then
            EXPRESSIONS="${EXPRESSIONS};s|${v}|${!NEW_VAR}/24|g"
        else
            EXPRESSIONS="${EXPRESSIONS};s|${v}|${!NEW_VAR}|g"
        fi
    done
    EXPRESSIONS="${EXPRESSIONS#';'}"

    # render template and inline replace config file
    sed -i "${EXPRESSIONS}" ${c}
done

ifconfig ${MME_INTERFACE_NAME_FOR_S10} ${MME_IPV4_ADDRESS_FOR_S10} up

pushd /openair-mme/scripts
./check_mme_s6a_certificate ${PREFIX} mme.${REALM}
popd

exec "$@"
