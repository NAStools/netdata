#!/bin/bash

# http://dev.mysql.com/doc/refman/5.0/en/server-status-variables.html
#
# https://dev.mysql.com/doc/refman/5.1/en/show-status.html
# SHOW STATUS provides server status information (see Section 5.1.6, “Server Status Variables”).
# This statement does not require any privilege.
# It requires only the ability to connect to the server.

mysql_update_every=5
mysql_priority=60000

declare -A mysql_cmds=() mysql_opts=() mysql_ids=()

mysql_exec() {
	local ret

	"${@}" -s -e "show global status;"
	ret=$?

	[ $ret -ne 0 ] && echo "plugin_command_failure $ret"
	return $ret
}

mysql_get() {
	unset \
		mysql_Bytes_received \
		mysql_Bytes_sent \
		mysql_Queries \
		mysql_Questions \
		mysql_Slow_queries \
		mysql_Handler_commit \
		mysql_Handler_delete \
		mysql_Handler_prepare \
		mysql_Handler_read_first \
		mysql_Handler_read_key \
		mysql_Handler_read_next \
		mysql_Handler_read_prev \
		mysql_Handler_read_rnd \
		mysql_Handler_read_rnd_next \
		mysql_Handler_rollback \
		mysql_Handler_savepoint \
		mysql_Handler_savepoint_rollback \
		mysql_Handler_update \
		mysql_Handler_write \
		mysql_Table_locks_immediate \
		mysql_Table_locks_waited \
		mysql_Select_full_join \
		mysql_Select_full_range_join \
		mysql_Select_range \
		mysql_Select_range_check \
		mysql_Select_scan \
		mysql_Sort_merge_passes \
		mysql_Sort_range \
		mysql_Sort_scan \
		mysql_Created_tmp_disk_tables \
		mysql_Created_tmp_files \
		mysql_Created_tmp_tables \
		mysql_Connection_errors_accept \
		mysql_Connection_errors_internal \
		mysql_Connection_errors_max_connections \
		mysql_Connection_errors_peer_addr \
		mysql_Connection_errors_select \
		mysql_Connection_errors_tcpwrap \
		mysql_Connections \
		mysql_Aborted_connects \
		mysql_Binlog_cache_disk_use \
		mysql_Binlog_cache_use \
		mysql_Binlog_stmt_cache_disk_use \
		mysql_Binlog_stmt_cache_use \
		mysql_Threads_connected \
		mysql_Threads_created \
		mysql_Threads_cached \
		mysql_Threads_running \
		mysql_Innodb_data_read \
		mysql_Innodb_data_written \
		mysql_Innodb_data_reads \
		mysql_Innodb_data_writes \
		mysql_Innodb_data_fsyncs \
		mysql_Innodb_data_pending_reads \
		mysql_Innodb_data_pending_writes \
		mysql_Innodb_data_pending_fsyncs \
		mysql_Innodb_log_waits \
		mysql_Innodb_log_write_requests \
		mysql_Innodb_log_writes \
		mysql_Innodb_os_log_fsyncs \
		mysql_Innodb_os_log_pending_fsyncs \
		mysql_Innodb_os_log_pending_writes \
		mysql_Innodb_os_log_written \
		mysql_Innodb_row_lock_current_waits \
		mysql_Innodb_rows_inserted \
		mysql_Innodb_rows_read \
		mysql_Innodb_rows_updated \
		mysql_Innodb_rows_deleted

	mysql_plugin_command_failure=0

	eval "$(mysql_exec "${@}" |\
		sed \
			-e "s/[[:space:]]\+/ /g" \
			-e "s/\./_/g" \
			-e "s/^\([a-zA-Z0-9_]\+\)[[:space:]]\+\([0-9]\+\)$/mysql_\1=\2/g" |\
		egrep "^mysql_[a-zA-Z0-9_]+=[[:digit:]]+$")"

	[ $mysql_plugin_command_failure -eq 1 ] && return 1
	[ -z "$mysql_Connections" ] && return 1

	mysql_Thread_cache_misses=0
	[ $(( mysql_Connections + 1 - 1 )) -gt 0 ] && mysql_Thread_cache_misses=$(( mysql_Threads_created * 10000 / mysql_Connections ))

	return 0
}

mysql_check() {
	# this should return:
	#  - 0 to enable the chart
	#  - 1 to disable the chart

	local x m mysql_cmd

	[ -z "${mysql_cmd}" ] && mysql_cmd="$(which mysql)"

	if [ ${#mysql_opts[@]} -eq 0 ]
		then
		mysql_cmds[local]="$mysql_cmd"
		mysql_opts[local]=
	fi

	# check once if the url works
	for m in "${!mysql_opts[@]}"
	do
		[ -z "${mysql_cmds[$m]}" ] && mysql_cmds[$m]="$mysql_cmd"
		if [ -z "${mysql_cmds[$m]}" ]
			then
			echo >&2 "$PROGRAM_NAME: mysql: cannot get mysql command for '$m'. Please set mysql_cmds[$m]='/path/to/mysql', in $confd/mysql.conf"
		fi

		mysql_get "${mysql_cmds[$m]}" ${mysql_opts[$m]}
		if [ ! $? -eq 0 ]
		then
			echo >&2 "$PROGRAM_NAME: mysql: cannot get global status for '$m'. Please set mysql_opts[$m]='options' to whatever needed to get connected to the mysql server, in $confd/mysql.conf"
			unset mysql_cmds[$m]
			unset mysql_opts[$m]
			unset mysql_ids[$m]
			continue
		fi

		mysql_ids[$m]="$( fixid "$m" )"
	done

	if [ ${#mysql_opts[@]} -eq 0 ]
		then
		echo >&2 "$PROGRAM_NAME: mysql: no mysql servers found. Please set mysql_opts[name]='options' to whatever needed to get connected to the mysql server, in $confd/mysql.conf"
		return 1
	fi

	return 0
}

mysql_create() {
	local m

	# create the charts
	for m in "${mysql_ids[@]}"
	do
		cat <<EOF
CHART mysql_$m.net '' "mysql Bandwidth" "kilobits/s" bandwidth mysql.net area $[mysql_priority + 1] $mysql_update_every
DIMENSION Bytes_received in incremental 8 1024
DIMENSION Bytes_sent out incremental -8 1024

CHART mysql_$m.queries '' "mysql Queries" "queries/s" queries mysql.queries line $[mysql_priority + 2] $mysql_update_every
DIMENSION Queries queries incremental 1 1
DIMENSION Questions questions incremental 1 1
DIMENSION Slow_queries slow_queries incremental -1 1

CHART mysql_$m.handlers '' "mysql Handlers" "handlers/s" handlers mysql.handlers line $[mysql_priority + 3] $mysql_update_every
DIMENSION Handler_commit commit incremental 1 1
DIMENSION Handler_delete delete incremental 1 1
DIMENSION Handler_prepare prepare incremental 1 1
DIMENSION Handler_read_first read_first incremental 1 1
DIMENSION Handler_read_key read_key incremental 1 1
DIMENSION Handler_read_next read_next incremental 1 1
DIMENSION Handler_read_prev read_prev incremental 1 1
DIMENSION Handler_read_rnd read_rnd incremental 1 1
DIMENSION Handler_read_rnd_next read_rnd_next incremental 1 1
DIMENSION Handler_rollback rollback incremental 1 1
DIMENSION Handler_savepoint savepoint incremental 1 1
DIMENSION Handler_savepoint_rollback savepoint_rollback incremental 1 1
DIMENSION Handler_update update incremental 1 1
DIMENSION Handler_write write incremental 1 1

CHART mysql_$m.table_locks '' "mysql Tables Locks" "locks/s" locks mysql.table_locks line $[mysql_priority + 4] $mysql_update_every
DIMENSION Table_locks_immediate immediate incremental 1 1
DIMENSION Table_locks_waited waited incremental -1 1

CHART mysql_$m.join_issues '' "mysql Select Join Issues" "joins/s" issues mysql.join_issues line $[mysql_priority + 5] $mysql_update_every
DIMENSION Select_full_join full_join incremental 1 1
DIMENSION Select_full_range_join full_range_join incremental 1 1
DIMENSION Select_range range incremental 1 1
DIMENSION Select_range_check range_check incremental 1 1
DIMENSION Select_scan scan incremental 1 1

CHART mysql_$m.sort_issues '' "mysql Sort Issues" "issues/s" issues mysql.sort.issues line $[mysql_priority + 6] $mysql_update_every
DIMENSION Sort_merge_passes merge_passes incremental 1 1
DIMENSION Sort_range range incremental 1 1
DIMENSION Sort_scan scan incremental 1 1

CHART mysql_$m.tmp '' "mysql Tmp Operations" "counter" temporaries mysql.tmp line $[mysql_priority + 7] $mysql_update_every
DIMENSION Created_tmp_disk_tables disk_tables incremental 1 1
DIMENSION Created_tmp_files files incremental 1 1
DIMENSION Created_tmp_tables tables incremental 1 1

CHART mysql_$m.connections '' "mysql Connections" "connections/s" connections mysql.connections line $[mysql_priority + 8] $mysql_update_every
DIMENSION Connections all incremental 1 1
DIMENSION Aborted_connects aborded incremental 1 1

CHART mysql_$m.binlog_cache '' "mysql Binlog Cache" "transactions/s" binlog mysql.binlog_cache line $[mysql_priority + 9] $mysql_update_every
DIMENSION Binlog_cache_disk_use disk incremental 1 1
DIMENSION Binlog_cache_use all incremental 1 1

CHART mysql_$m.threads '' "mysql Threads" "threads" threads mysql.threads line $[mysql_priority + 10] $mysql_update_every
DIMENSION Threads_connected connected absolute 1 1
DIMENSION Threads_created created incremental 1 1
DIMENSION Threads_cached cached absolute -1 1
DIMENSION Threads_running running absolute 1 1

CHART mysql_$m.thread_cache_misses '' "mysql Threads Cache Misses" "misses" threads mysql.thread_cache_misses area $[mysql_priority + 11] $mysql_update_every
DIMENSION misses misses absolute 1 100

CHART mysql_$m.innodb_io '' "mysql InnoDB I/O Bandwidth" "kilobytes/s" innodb mysql.innodb_io area $[mysql_priority + 12] $mysql_update_every
DIMENSION Innodb_data_read read incremental 1 1024
DIMENSION Innodb_data_written write incremental -1 1024

CHART mysql_$m.innodb_io_ops '' "mysql InnoDB I/O Operations" "operations/s" innodb mysql.innodb_io_ops line $[mysql_priority + 13] $mysql_update_every
DIMENSION Innodb_data_reads reads incremental 1 1
DIMENSION Innodb_data_writes writes incremental -1 1
DIMENSION Innodb_data_fsyncs fsyncs incremental 1 1

CHART mysql_$m.innodb_io_pending_ops '' "mysql InnoDB Pending I/O Operations" "operations" innodb mysql.innodb_io_pending_ops line $[mysql_priority + 14] $mysql_update_every
DIMENSION Innodb_data_pending_reads reads absolute 1 1
DIMENSION Innodb_data_pending_writes writes absolute -1 1
DIMENSION Innodb_data_pending_fsyncs fsyncs absolute 1 1

CHART mysql_$m.innodb_log '' "mysql InnoDB Log Operations" "operations/s" innodb mysql.innodb_log line $[mysql_priority + 15] $mysql_update_every
DIMENSION Innodb_log_waits waits incremental 1 1
DIMENSION Innodb_log_write_requests write_requests incremental -1 1
DIMENSION Innodb_log_writes writes incremental -1 1

CHART mysql_$m.innodb_os_log '' "mysql InnoDB OS Log Operations" "operations" innodb mysql.innodb_os_log line $[mysql_priority + 16] $mysql_update_every
DIMENSION Innodb_os_log_fsyncs fsyncs incremental 1 1
DIMENSION Innodb_os_log_pending_fsyncs pending_fsyncs absolute 1 1
DIMENSION Innodb_os_log_pending_writes pending_writes absolute -1 1

CHART mysql_$m.innodb_os_log_io '' "mysql InnoDB OS Log Bandwidth" "kilobytes/s" innodb mysql.innodb_os_log_io area $[mysql_priority + 17] $mysql_update_every
DIMENSION Innodb_os_log_written write incremental -1 1024

CHART mysql_$m.innodb_cur_row_lock '' "mysql InnoDB Current Row Locks" "operations" innodb mysql.innodb_cur_row_lock area $[mysql_priority + 18] $mysql_update_every
DIMENSION Innodb_row_lock_current_waits current_waits absolute 1 1

CHART mysql_$m.innodb_rows '' "mysql InnoDB Row Operations" "operations/s" innodb mysql.innodb_rows area $[mysql_priority + 19] $mysql_update_every
DIMENSION Innodb_rows_read read incremental 1 1
DIMENSION Innodb_rows_deleted deleted incremental -1 1
DIMENSION Innodb_rows_inserted inserted incremental 1 1
DIMENSION Innodb_rows_updated updated incremental -1 1

EOF

	if [ ! -z "$mysql_Binlog_stmt_cache_disk_use" ]
		then
		cat <<EOF
CHART mysql_$m.binlog_stmt_cache '' "mysql Binlog Statement Cache" "statements/s" binlog mysql.binlog_stmt_cache line $[mysql_priority + 20] $mysql_update_every
DIMENSION Binlog_stmt_cache_disk_use disk incremental 1 1
DIMENSION Binlog_stmt_cache_use all incremental 1 1
EOF
	fi

	if [ ! -z "$mysql_Connection_errors_accept" ]
		then
		cat <<EOF
CHART mysql_$m.connection_errors '' "mysql Connection Errors" "connections/s" connections mysql.connection_errors line $[mysql_priority + 21] $mysql_update_every
DIMENSION Connection_errors_accept accept incremental 1 1
DIMENSION Connection_errors_internal internal incremental 1 1
DIMENSION Connection_errors_max_connections max incremental 1 1
DIMENSION Connection_errors_peer_addr peer_addr incremental 1 1
DIMENSION Connection_errors_select select incremental 1 1
DIMENSION Connection_errors_tcpwrap tcpwrap incremental 1 1
EOF
	fi

	done
	return 0
}


mysql_update() {
	# the first argument to this function is the microseconds since last update
	# pass this parameter to the BEGIN statement (see bellow).

	# do all the work to collect / calculate the values
	# for each dimension
	# remember: KEEP IT SIMPLE AND SHORT

	# 1. get the counters page from mysql
	# 2. sed to remove spaces; replace . with _; remove spaces around =; prepend each line with: local mysql_
	# 3. egrep lines starting with:
	#    local mysql_client_http_ then one or more of these a-z 0-9 _ then = and one of more of 0-9
	#    local mysql_server_all_ then one or more of these a-z 0-9 _ then = and one of more of 0-9
	# 4. then execute this as a script with the eval
	#
	# be very carefull with eval:
	# prepare the script and always grep at the end the lines that are usefull, so that
	# even if something goes wrong, no other code can be executed

	local m x
	for m in "${!mysql_ids[@]}"
	do
		x="${mysql_ids[$m]}"

		mysql_get "${mysql_cmds[$m]}" ${mysql_opts[$m]}
		if [ $? -ne 0 ]
			then
			unset mysql_ids[$m]
			unset mysql_opts[$m]
			unset mysql_cmds[$m]
			echo >&2 "$PROGRAM_NAME: mysql: failed to get values for '$m', disabling it."
			continue
		fi

		# write the result of the work.
		cat <<VALUESEOF
BEGIN mysql_$x.net $1
SET Bytes_received = $mysql_Bytes_received
SET Bytes_sent = $mysql_Bytes_sent
END
BEGIN mysql_$x.queries $1
SET Queries = $mysql_Queries
SET Questions = $mysql_Questions
SET Slow_queries = $mysql_Slow_queries
END
BEGIN mysql_$x.handlers $1
SET Handler_commit = $mysql_Handler_commit
SET Handler_delete = $mysql_Handler_delete
SET Handler_prepare = $mysql_Handler_prepare
SET Handler_read_first = $mysql_Handler_read_first
SET Handler_read_key = $mysql_Handler_read_key
SET Handler_read_next = $mysql_Handler_read_next
SET Handler_read_prev = $mysql_Handler_read_prev
SET Handler_read_rnd = $mysql_Handler_read_rnd
SET Handler_read_rnd_next = $mysql_Handler_read_rnd_next
SET Handler_rollback = $mysql_Handler_rollback
SET Handler_savepoint = $mysql_Handler_savepoint
SET Handler_savepoint_rollback = $mysql_Handler_savepoint_rollback
SET Handler_update = $mysql_Handler_update
SET Handler_write = $mysql_Handler_write
END
BEGIN mysql_$x.table_locks $1
SET Table_locks_immediate = $mysql_Table_locks_immediate
SET Table_locks_waited = $mysql_Table_locks_waited
END
BEGIN mysql_$x.join_issues $1
SET Select_full_join = $mysql_Select_full_join
SET Select_full_range_join = $mysql_Select_full_range_join
SET Select_range = $mysql_Select_range
SET Select_range_check = $mysql_Select_range_check
SET Select_scan = $mysql_Select_scan
END
BEGIN mysql_$x.sort_issues $1
SET Sort_merge_passes = $mysql_Sort_merge_passes
SET Sort_range = $mysql_Sort_range
SET Sort_scan = $mysql_Sort_scan
END
BEGIN mysql_$m.tmp $1
SET Created_tmp_disk_tables = $mysql_Created_tmp_disk_tables
SET Created_tmp_files = $mysql_Created_tmp_files
SET Created_tmp_tables = $mysql_Created_tmp_tables
END
BEGIN mysql_$m.connections $1
SET Connections = $mysql_Connections
SET Aborted_connects = $mysql_Aborted_connects
END
BEGIN mysql_$m.binlog_cache $1
SET Binlog_cache_disk_use = $mysql_Binlog_cache_disk_use
SET Binlog_cache_use = $mysql_Binlog_cache_use
END
BEGIN mysql_$m.threads $1
SET Threads_connected = $mysql_Threads_connected
SET Threads_created = $mysql_Threads_created
SET Threads_cached = $mysql_Threads_cached
SET Threads_running = $mysql_Threads_running
END
BEGIN mysql_$m.thread_cache_misses $1
SET misses = $mysql_Thread_cache_misses
END
BEGIN mysql_$m.innodb_io $1
SET Innodb_data_read = $mysql_Innodb_data_read
SET Innodb_data_written = $mysql_Innodb_data_written
END
BEGIN mysql_$m.innodb_io_ops $1
SET Innodb_data_reads = $mysql_Innodb_data_reads
SET Innodb_data_writes = $mysql_Innodb_data_writes
SET Innodb_data_fsyncs = $mysql_Innodb_data_fsyncs
END
BEGIN mysql_$m.innodb_io_pending_ops $1
SET Innodb_data_pending_reads = $mysql_Innodb_data_pending_reads
SET Innodb_data_pending_writes = $mysql_Innodb_data_pending_writes
SET Innodb_data_pending_fsyncs = $mysql_Innodb_data_pending_fsyncs
END
BEGIN mysql_$m.innodb_log $1
SET Innodb_log_waits = $mysql_Innodb_log_waits
SET Innodb_log_write_requests = $mysql_Innodb_log_write_requests
SET Innodb_log_writes = $mysql_Innodb_log_writes
END
BEGIN mysql_$m.innodb_os_log $1
SET Innodb_os_log_fsyncs = $mysql_Innodb_os_log_fsyncs
SET Innodb_os_log_pending_fsyncs = $mysql_Innodb_os_log_pending_fsyncs
SET Innodb_os_log_pending_writes = $mysql_Innodb_os_log_pending_writes
END
BEGIN mysql_$m.innodb_os_log_io $1
SET Innodb_os_log_written = $mysql_Innodb_os_log_written
END
BEGIN mysql_$m.innodb_cur_row_lock $1
SET Innodb_row_lock_current_waits = $mysql_Innodb_row_lock_current_waits
END
BEGIN mysql_$m.innodb_rows $1
SET Innodb_rows_inserted = $mysql_Innodb_rows_inserted
SET Innodb_rows_read = $mysql_Innodb_rows_read
SET Innodb_rows_updated = $mysql_Innodb_rows_updated
SET Innodb_rows_deleted = $mysql_Innodb_rows_deleted
END
VALUESEOF

		if [ ! -z "$mysql_Binlog_stmt_cache_disk_use" ]
			then
			cat <<VALUESEOF
BEGIN mysql_$m.binlog_stmt_cache $1
SET Binlog_stmt_cache_disk_use = $mysql_Binlog_stmt_cache_disk_use
SET Binlog_stmt_cache_use = $mysql_Binlog_stmt_cache_use
END
VALUESEOF
		fi

		if [ ! -z "$mysql_Connection_errors_accept" ]
			then
			cat <<VALUESEOF
BEGIN mysql_$m.connection_errors $1
SET Connection_errors_accept = $mysql_Connection_errors_accept
SET Connection_errors_internal = $mysql_Connection_errors_internal
SET Connection_errors_max_connections = $mysql_Connection_errors_max_connections
SET Connection_errors_peer_addr = $mysql_Connection_errors_peer_addr
SET Connection_errors_select = $mysql_Connection_errors_select
SET Connection_errors_tcpwrap = $mysql_Connection_errors_tcpwrap
END
VALUESEOF
		fi
	done

	[ ${#mysql_ids[@]} -eq 0 ] && echo >&2 "$PROGRAM_NAME: mysql: no mysql servers left active." && return 1
	return 0
}

