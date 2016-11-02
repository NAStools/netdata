#include "common.h"

static inline char *softnet_column_name(uint32_t column) {
    switch(column) {
        // https://github.com/torvalds/linux/blob/a7fd20d1c476af4563e66865213474a2f9f473a4/net/core/net-procfs.c#L161-L166
        case 0: return "processed";
        case 1: return "dropped";
        case 2: return "squeezed";
        case 9: return "received_rps";
        case 10: return "flow_limit_count";
        default: return NULL;
    }
}

int do_proc_net_softnet_stat(int update_every, unsigned long long dt) {
    (void)dt;

    static procfile *ff = NULL;
    static int do_per_core = -1;
    static uint32_t allocated_lines = 0, allocated_columns = 0, *data = NULL;

    if(do_per_core == -1) do_per_core = config_get_boolean("plugin:proc:/proc/net/softnet_stat", "softnet_stat per core", 1);

    if(!ff) {
        char filename[FILENAME_MAX + 1];
        snprintfz(filename, FILENAME_MAX, "%s%s", global_host_prefix, "/proc/net/softnet_stat");
        ff = procfile_open(config_get("plugin:proc:/proc/net/softnet_stat", "filename to monitor", filename), " \t", PROCFILE_FLAG_DEFAULT);
    }
    if(!ff) return 1;

    ff = procfile_readall(ff);
    if(!ff) return 0; // we return 0, so that we will retry to open it next time

    uint32_t lines = procfile_lines(ff), l;
    uint32_t words = procfile_linewords(ff, 0), w;

    if(!lines || !words) {
        error("Cannot read /proc/net/softnet_stat, %u lines and %u columns reported.", lines, words);
        return 1;
    }

    if(lines > 200) lines = 200;
    if(words > 50) words = 50;

    if(unlikely(!data || lines > allocated_lines || words > allocated_columns)) {
        freez(data);
        allocated_lines = lines;
        allocated_columns = words;
        data = mallocz((allocated_lines + 1) * allocated_columns * sizeof(uint32_t));
    }

    // initialize to zero
    memset(data, 0, (allocated_lines + 1) * allocated_columns * sizeof(uint32_t));

    // parse the values
    for(l = 0; l < lines ;l++) {
        words = procfile_linewords(ff, l);
        if(!words) continue;

        if(words > allocated_columns) words = allocated_columns;

        for(w = 0; w < words ; w++) {
            if(unlikely(softnet_column_name(w))) {
                uint32_t t = strtoul(procfile_lineword(ff, l, w), NULL, 16);
                data[w] += t;
                data[((l + 1) * allocated_columns) + w] = t;
            }
        }
    }

    if(data[(lines * allocated_columns)] == 0)
        lines--;

    RRDSET *st;

    // --------------------------------------------------------------------

    st = rrdset_find_bytype("system", "softnet_stat");
    if(!st) {
        st = rrdset_create("system", "softnet_stat", NULL, "softnet_stat", NULL, "System softnet_stat", "events/s", 955, update_every, RRDSET_TYPE_LINE);
        for(w = 0; w < allocated_columns ;w++)
            if(unlikely(softnet_column_name(w)))
                rrddim_add(st, softnet_column_name(w), NULL, 1, 1, RRDDIM_INCREMENTAL);
    }
    else rrdset_next(st);

    for(w = 0; w < allocated_columns ;w++)
        if(unlikely(softnet_column_name(w)))
            rrddim_set(st, softnet_column_name(w), data[w]);

    rrdset_done(st);

    if(do_per_core) {
        for(l = 0; l < lines ;l++) {
            char id[50+1];
            snprintfz(id, 50, "cpu%u_softnet_stat", l);

            st = rrdset_find_bytype("cpu", id);
            if(!st) {
                char title[100+1];
                snprintfz(title, 100, "CPU%u softnet_stat", l);

                st = rrdset_create("cpu", id, NULL, "softnet_stat", NULL, title, "events/s", 4101 + l, update_every, RRDSET_TYPE_LINE);
                for(w = 0; w < allocated_columns ;w++)
                    if(unlikely(softnet_column_name(w)))
                        rrddim_add(st, softnet_column_name(w), NULL, 1, 1, RRDDIM_INCREMENTAL);
            }
            else rrdset_next(st);

            for(w = 0; w < allocated_columns ;w++)
                if(unlikely(softnet_column_name(w)))
                    rrddim_set(st, softnet_column_name(w), data[((l + 1) * allocated_columns) + w]);

            rrdset_done(st);
        }
    }

    return 0;
}
