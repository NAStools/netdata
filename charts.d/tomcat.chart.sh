#!/bin/bash

# Description: Tomcat netdata charts.d plugin
# Author: Jorge Romero

# the URL to download tomcat status info
# usually http://localhost:8080/manager/status?XML=true
tomcat_url=""

# set tomcat username/password here
tomcatUser=""
tomcatPassword=""

# _update_every is a special variable - it holds the number of seconds
# between the calls of the _update() function
tomcat_update_every=

tomcat_priority=60000

# convert tomcat floating point values
# to integer using this multiplier
# this only affects precision - the values
# will be in the proper units
tomcat_decimal_detail=1000000

# used by volume chart to convert bytes to KB
tomcat_decimal_KB_detail=1000

tomcat_check() {

	require_cmd xmlstarlet || return 1


	# check if url, username, passwords are set
	if [ -z "${tomcat_url}" ]; then
	  	echo >&2 "tomcat url is unset or set to the empty string"
		return 1
	fi
	if [ -z "${tomcatUser}" ]; then
    	  	echo >&2 "tomcat user is unset or set to the empty string"
		return 1
	fi
	if [ -z "${tomcatPassword}" ]; then
    	  	echo >&2 "tomcat password is unset or set to the empty string"
		return 1
	fi

	# check if we can get to tomcat's status page
	tomcat_get
	if [ $? -ne 0 ]
		then
		echo >&2 "tomcat: couldn't get to status page on URL '${tomcat_url}'."\
		"Please make sure tomcat url, username and password are correct."
		return 1
	fi

	# this should return:
	#  - 0 to enable the chart
	#  - 1 to disable the chart

	return 0
}

tomcat_get() {
	# Collect tomcat values
	mapfile -t lines < <(curl -u "$tomcatUser":"$tomcatPassword" -Ss "$tomcat_url" |\
		xmlstarlet sel \
			-t -m "/status/jvm/memory" -v @free \
			-n -m "/status/connector[@name='\"http-bio-8080\"']/threadInfo" -v @currentThreadCount \
			-n -v @currentThreadsBusy \
			-n -m "/status/connector[@name='\"http-bio-8080\"']/requestInfo" -v @requestCount \
			-n -v @bytesSent -n -)

	tomcat_jvm_freememory="${lines[0]}"
	tomcat_threads="${lines[1]}"
	tomcat_threads_busy="${lines[2]}"
	tomcat_accesses="${lines[3]}"
	tomcat_volume="${lines[4]}"

	return 0
}

# _create is called once, to create the charts
tomcat_create() {
	cat <<EOF
CHART tomcat.accesses '' "tomcat requests" "requests/s" statistics tomcat.accesses area $((tomcat_priority + 8)) $tomcat_update_every
DIMENSION accesses '' incremental
CHART tomcat.volume '' "tomcat volume" "KB/s" volume tomcat.volume area $((tomcat_priority + 5)) $tomcat_update_every
DIMENSION volume '' incremental divisor ${tomcat_decimal_KB_detail}
CHART tomcat.threads '' "tomcat threads" "current threads" statistics tomcat.threads line $((tomcat_priority + 6)) $tomcat_update_every
DIMENSION current '' absolute 1
DIMENSION busy '' absolute 1
CHART tomcat.jvm '' "JVM Free Memory" "MB" statistics tomcat.jvm area $((tomcat_priority + 8)) $tomcat_update_every
DIMENSION jvm '' absolute 1 ${tomcat_decimal_detail}
EOF
	return 0
}

# _update is called continiously, to collect the values
tomcat_update() {
	local reqs net
	# the first argument to this function is the microseconds since last update
	# pass this parameter to the BEGIN statement (see bellow).

	# do all the work to collect / calculate the values
	# for each dimension
	# remember: KEEP IT SIMPLE AND SHORT

	tomcat_get || return 1

	# write the result of the work.
	cat <<VALUESEOF
BEGIN tomcat.accesses $1
SET accesses = $((tomcat_accesses))
END
BEGIN tomcat.volume $1
SET volume = $((tomcat_volume))
END
BEGIN tomcat.threads $1
SET current = $((tomcat_threads))
SET busy = $((tomcat_threads_busy))
END
BEGIN tomcat.jvm $1
SET jvm = $((tomcat_jvm_freememory))
END
VALUESEOF

	return 0
}
