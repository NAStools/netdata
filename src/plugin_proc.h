#ifndef NETDATA_PLUGIN_PROC_H
#define NETDATA_PLUGIN_PROC_H 1

void *proc_main(void *ptr);

extern int do_proc_net_dev(int update_every, unsigned long long dt);
extern int do_proc_diskstats(int update_every, unsigned long long dt);
extern int do_proc_net_snmp(int update_every, unsigned long long dt);
extern int do_proc_net_snmp6(int update_every, unsigned long long dt);
extern int do_proc_net_netstat(int update_every, unsigned long long dt);
extern int do_proc_net_stat_conntrack(int update_every, unsigned long long dt);
extern int do_proc_net_ip_vs_stats(int update_every, unsigned long long dt);
extern int do_proc_stat(int update_every, unsigned long long dt);
extern int do_proc_meminfo(int update_every, unsigned long long dt);
extern int do_proc_vmstat(int update_every, unsigned long long dt);
extern int do_proc_net_rpc_nfsd(int update_every, unsigned long long dt);
extern int do_proc_sys_kernel_random_entropy_avail(int update_every, unsigned long long dt);
extern int do_proc_interrupts(int update_every, unsigned long long dt);
extern int do_proc_softirqs(int update_every, unsigned long long dt);
extern int do_sys_kernel_mm_ksm(int update_every, unsigned long long dt);
extern int do_proc_loadavg(int update_every, unsigned long long dt);
extern int do_proc_net_stat_synproxy(int update_every, unsigned long long dt);

#endif /* NETDATA_PLUGIN_PROC_H */
