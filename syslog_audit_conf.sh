#########################################################################
# File Name: syslog_audit_conf.sh
# Author: Ral
# mail: 
# Created Time: 
#########################################################################
#!/bin/bash

SYSLOG_CONF="/etc/syslog.conf"
DAEMON_WARNING_CONF="daemon.warning /var/log/audit.log"
SYSLOG_LOCAL_SERVER="daemon.warning @LOCALHOST"
RESTART_FLAG=

ECHO()
{
    echo $* >> ${SYSLOG_CONF}
}

already_exist=`cat ${SYSLOG_CONF} | grep "${DAEMON_WARNING_CONF}"`
if [[ -z ${already_exist} ]]; then
    ECHO
    ECHO "${DAEMON_WARNING_CONF}"
    ECHO
    RESTART_FLAG=1
fi
already_exist=`cat ${SYSLOG_CONF} | grep "${SYSLOG_LOCAL_SERVER}"`
if [[ -z ${already_exist} ]]; then
    ECHO
    ECHO "${SYSLOG_LOCAL_SERVER}"
    ECHO
    RESTART_FLAG=1
fi
if [[ ${RESTART_FLAG} -eq 1 ]]; then
    service syslog restart
fi
