#!/bin/bash -x

TOTAL=5
STEP=1

# move the old out of the way
#
echo ""
echo "$STEP/$TOTAL Stopping ..."
STEP=$((STEP+1))
if [ `which systemctl` ] ; then
	systemctl stop brubeck > /dev/null 2>&1
else
	echo "No systemctl found.  Service not stopped"
fi
sleep 1
[ -f /usr/local/sbin/brubeck ] && mv /usr/local/sbin/brubeck /usr/local/sbin/brubeck.old
#
# install the binary
echo "$STEP/$TOTAL Installing binary ..."
STEP=$((STEP+1))
[ ! -d /usr/local/sbin ] && mkdir -p /usr/local/sbin
cp brubeck /usr/local/sbin/brubeck
chown root.root /usr/local/sbin/brubeck
chmod +x /usr/local/sbin/brubeck
#
# install the init file
echo "$STEP/$TOTAL Installing init script ..."
STEP=$((STEP+1))
VALUE=n
read -p "Install init file [(y/n)n]: " RVALUE
[ ! -z "${RVALUE}" ] && VALUE="${RVALUE}"
if [ $VALUE == y ] ; then
    [ -f /etc/init.d/brubeck ] && mv /etc/init.d/brubeck /etc/init.d/brubeck.old
    if [ -d /etc/init.d ] ; then
	cp brubeck_init.d /etc/init.d/brubeck
	chown root.root /etc/init.d/brubeck
	chmod +x /etc/init.d/brubeck
	if [ `which systemctl` ] ; then
	    systemctl daemon-reload > /dev/null 2>&1
	    if [ `which chkconfig` ] ; then
		chkconfig --add brubeck
	    else
		if [ `which update-rc.d` ] ; then
		    update-rc.d brubeck defaults
		else
		    systemctl enable brubeck
		fi
	    fi
	else
	    echo "No systemctl found.  Service not configured.  Init script installed."
	fi
    fi
fi


function set()
{
   MUNGE=$1
   KEY=$2
   VALUE=$3
   FILE=$4
   INT=$5

   if [ $INT -gt 0 ] ; then
	sed -i -e "s/${MUNGE}/            \"${KEY}\" : ${VALUE}\,/g" $FILE
   else
	sed -i -e "s/${MUNGE}/            \"${KEY}\" : \"${VALUE}\"\,/g" $FILE
   fi
}

function getset()
{
   MUNGE=$1
   KEY=$2
   VALUE=$3
   PROMPT=$4
   FILE=$5
   INT=$6

   read -p "${PROMPT} [${VALUE}]: " RVALUE
   [ ! -z "${RVALUE}" ] && VALUE="${RVALUE}"

   set "${MUNGE}" "${KEY}" "${VALUE}" "${FILE}" $INT
}

echo "$STEP/$TOTAL Configuring ..."
STEP=$((STEP+1))
echo ""
VALUE=n
read -p "Reconfigure config file [(y/n)n]: " RVALUE
[ ! -z $RVALUE ] && VALUE=$RVALUE
if [ $VALUE == y ] ; then
    [ -f /etc/brubeck/config.json ] && mv /etc/brubeck/config.json /etc/brubeck/config.json.old
    CONFIG_WORK="/tmp/config.json"
    rm -f "${CONFIG_WORK}"
    cat <<EOF >> "${CONFIG_WORK}"
{
    "sharding" : false,
SERVER_NAME
    "dumpfile" : "/var/log/brubeck.dump",
    "capacity" : 2000,
    "backends" :
    [
	{
	    "type" : "carbon",
CARBON_SERVER
	    "port" : 2003,
	    "frequency" : 10,
	    "pickle" : false
	}
    ],
    "samplers" :
    [
	{
	    "type" : "statsd",
	    "address" : "0.0.0.0",
BRUBECK_PORT
	    "workers" : 8,
	    "multisock" : false,
	    "multimsg" : 0
	}
    ]
}
EOF

    # get the vars
    # server_name
    HOSTNAME=`uname -n`
    getset "SERVER_NAME" "server_name" "brubeck-${HOSTNAME}" "Enter the server name, used for server metrics" "${CONFIG_WORK}" 0
    getset "CARBON_SERVER" "address" "bubble.bottorrent.net" "Enter a graphite/carbon server ip name or number" "${CONFIG_WORK}" 0
    getset "BRUBECK_PORT" "port" "8125" "Enter the listening port for brubeck, typically 8125." "${CONFIG_WORK}" 1

    read -p "Add datadog configuration [(y/n)n]: " RVALUE
    case $RVALUE in
	Y|y)
	    # the datadog blob
	    DATADOG_CONFIG_WORK="/tmp/datadog.config.json"
	    rm -f "${DATADOG_CONFIG_WORK}"
	    cat <<EOF >> "${DATADOG_CONFIG_WORK}"
	},
	{
	    "type" : "datadog",
DATADOG_SERVER
DATADOG_PORT
	    "frequency" : 10,
DATADOG_FILTER
	    "expire" : 1
EOF
	    getset "DATADOG_SERVER" "address" "localhost" "Enter a datadog server, typically localhost" "${DATADOG_CONFIG_WORK}"
	    getset "DATADOG_PORT" "port" "9125" "Enter the port of the datadog server, typically 9125 as we run brubeck on 8125" "${DATADOG_CONFIG_WORK}"
	    getset "DATADOG_FILTER" "filter" ".*" "Enter a regex.  Matches will be sent to datadog.  Defaults to all." "${DATADOG_CONFIG_WORK}"
   	    sed -i -e "/\"pickle\" : false/r${DATADOG_CONFIG_WORK}" "${CONFIG_WORK}"
	    ;;	

	*)
   	    sed -i -e "/\"pickle\" : false/r\", \"expire\" : 1"
	    ;;
    esac

    [ ! -d /etc/brubeck ] && mkdir -p /etc/brubeck
    cp "${CONFIG_WORK}" /etc/brubeck/config.json
    chown -R root.root /etc/brubeck/config.json
fi

echo ""
echo "$STEP/$TOTAL Restarting ..."
STEP=$((STEP+1))
if [ `which systemctl` ] ; then
	systemctl restart brubeck
else
	echo "No systemctl found.  Service not restarted."
fi
