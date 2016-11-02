#include "common.h"

void rrd_stats_api_v1_chart(RRDSET *st, BUFFER *wb)
{
    pthread_rwlock_rdlock(&st->rwlock);

    buffer_sprintf(wb,
        "\t\t{\n"
        "\t\t\t\"id\": \"%s\",\n"
        "\t\t\t\"name\": \"%s\",\n"
        "\t\t\t\"type\": \"%s\",\n"
        "\t\t\t\"family\": \"%s\",\n"
        "\t\t\t\"context\": \"%s\",\n"
        "\t\t\t\"title\": \"%s\",\n"
        "\t\t\t\"priority\": %ld,\n"
        "\t\t\t\"enabled\": %s,\n"
        "\t\t\t\"units\": \"%s\",\n"
        "\t\t\t\"data_url\": \"/api/v1/data?chart=%s\",\n"
        "\t\t\t\"chart_type\": \"%s\",\n"
        "\t\t\t\"duration\": %ld,\n"
        "\t\t\t\"first_entry\": %ld,\n"
        "\t\t\t\"last_entry\": %ld,\n"
        "\t\t\t\"update_every\": %d,\n"
        "\t\t\t\"dimensions\": {\n"
        , st->id
        , st->name
        , st->type
        , st->family
        , st->context
        , st->title
        , st->priority
        , st->enabled?"true":"false"
        , st->units
        , st->name
        , rrdset_type_name(st->chart_type)
        , st->entries * st->update_every
        , rrdset_first_entry_t(st)
        , rrdset_last_entry_t(st)
        , st->update_every
        );

    unsigned long memory = st->memsize;

    int c = 0;
    RRDDIM *rd;
    for(rd = st->dimensions; rd ; rd = rd->next) {
        if(rd->flags & RRDDIM_FLAG_HIDDEN) continue;

        memory += rd->memsize;

        buffer_sprintf(wb,
            "%s"
            "\t\t\t\t\"%s\": { \"name\": \"%s\" }"
            , c?",\n":""
            , rd->id
            , rd->name
            );

        c++;
    }

    buffer_strcat(wb, "\n\t\t\t},\n\t\t\t\"green\": ");
    buffer_rrd_value(wb, st->green);
    buffer_strcat(wb, ",\n\t\t\t\"red\": ");
    buffer_rrd_value(wb, st->red);

    buffer_sprintf(wb,
        "\n\t\t}"
        );

    pthread_rwlock_unlock(&st->rwlock);
}

void rrd_stats_api_v1_charts(BUFFER *wb)
{
    long c;
    RRDSET *st;

    buffer_sprintf(wb, "{\n"
           "\t\"hostname\": \"%s\""
        ",\n\t\"update_every\": %d"
        ",\n\t\"history\": %d"
        ",\n\t\"charts\": {"
        , localhost.hostname
        , rrd_update_every
        , rrd_default_history_entries
        );

    pthread_rwlock_rdlock(&localhost.rrdset_root_rwlock);
    for(st = localhost.rrdset_root, c = 0; st ; st = st->next) {
        if(st->enabled && st->dimensions) {
            if(c) buffer_strcat(wb, ",");
            buffer_strcat(wb, "\n\t\t\"");
            buffer_strcat(wb, st->id);
            buffer_strcat(wb, "\": ");
            rrd_stats_api_v1_chart(st, wb);
            c++;
        }
    }
    pthread_rwlock_unlock(&localhost.rrdset_root_rwlock);

    buffer_strcat(wb, "\n\t}\n}\n");
}


unsigned long rrd_stats_one_json(RRDSET *st, char *options, BUFFER *wb)
{
    time_t now = time(NULL);

    pthread_rwlock_rdlock(&st->rwlock);

    buffer_sprintf(wb,
        "\t\t{\n"
        "\t\t\t\"id\": \"%s\",\n"
        "\t\t\t\"name\": \"%s\",\n"
        "\t\t\t\"type\": \"%s\",\n"
        "\t\t\t\"family\": \"%s\",\n"
        "\t\t\t\"context\": \"%s\",\n"
        "\t\t\t\"title\": \"%s\",\n"
        "\t\t\t\"priority\": %ld,\n"
        "\t\t\t\"enabled\": %d,\n"
        "\t\t\t\"units\": \"%s\",\n"
        "\t\t\t\"url\": \"/data/%s/%s\",\n"
        "\t\t\t\"chart_type\": \"%s\",\n"
        "\t\t\t\"counter\": %lu,\n"
        "\t\t\t\"entries\": %ld,\n"
        "\t\t\t\"first_entry_t\": %ld,\n"
        "\t\t\t\"last_entry\": %lu,\n"
        "\t\t\t\"last_entry_t\": %ld,\n"
        "\t\t\t\"last_entry_secs_ago\": %ld,\n"
        "\t\t\t\"update_every\": %d,\n"
        "\t\t\t\"isdetail\": %d,\n"
        "\t\t\t\"usec_since_last_update\": %llu,\n"
        "\t\t\t\"collected_total\": " TOTAL_NUMBER_FORMAT ",\n"
        "\t\t\t\"last_collected_total\": " TOTAL_NUMBER_FORMAT ",\n"
        "\t\t\t\"dimensions\": [\n"
        , st->id
        , st->name
        , st->type
        , st->family
        , st->context
        , st->title
        , st->priority
        , st->enabled
        , st->units
        , st->name, options?options:""
        , rrdset_type_name(st->chart_type)
        , st->counter
        , st->entries
        , rrdset_first_entry_t(st)
        , rrdset_last_slot(st)
        , rrdset_last_entry_t(st)
        , (now < rrdset_last_entry_t(st)) ? (time_t)0 : now - rrdset_last_entry_t(st)
        , st->update_every
        , st->isdetail
        , st->usec_since_last_update
        , st->collected_total
        , st->last_collected_total
        );

    unsigned long memory = st->memsize;

    RRDDIM *rd;
    for(rd = st->dimensions; rd ; rd = rd->next) {

        memory += rd->memsize;

        buffer_sprintf(wb,
            "\t\t\t\t{\n"
            "\t\t\t\t\t\"id\": \"%s\",\n"
            "\t\t\t\t\t\"name\": \"%s\",\n"
            "\t\t\t\t\t\"entries\": %ld,\n"
            "\t\t\t\t\t\"isHidden\": %d,\n"
            "\t\t\t\t\t\"algorithm\": \"%s\",\n"
            "\t\t\t\t\t\"multiplier\": %ld,\n"
            "\t\t\t\t\t\"divisor\": %ld,\n"
            "\t\t\t\t\t\"last_entry_t\": %ld,\n"
            "\t\t\t\t\t\"collected_value\": " COLLECTED_NUMBER_FORMAT ",\n"
            "\t\t\t\t\t\"calculated_value\": " CALCULATED_NUMBER_FORMAT ",\n"
            "\t\t\t\t\t\"last_collected_value\": " COLLECTED_NUMBER_FORMAT ",\n"
            "\t\t\t\t\t\"last_calculated_value\": " CALCULATED_NUMBER_FORMAT ",\n"
            "\t\t\t\t\t\"memory\": %lu\n"
            "\t\t\t\t}%s\n"
            , rd->id
            , rd->name
            , rd->entries
            , (rd->flags & RRDDIM_FLAG_HIDDEN)?1:0
            , rrddim_algorithm_name(rd->algorithm)
            , rd->multiplier
            , rd->divisor
            , rd->last_collected_time.tv_sec
            , rd->collected_value
            , rd->calculated_value
            , rd->last_collected_value
            , rd->last_calculated_value
            , rd->memsize
            , rd->next?",":""
            );
    }

    buffer_sprintf(wb,
        "\t\t\t],\n"
        "\t\t\t\"memory\" : %lu\n"
        "\t\t}"
        , memory
        );

    pthread_rwlock_unlock(&st->rwlock);
    return memory;
}

#define RRD_GRAPH_JSON_HEADER "{\n\t\"charts\": [\n"
#define RRD_GRAPH_JSON_FOOTER "\n\t]\n}\n"

void rrd_stats_graph_json(RRDSET *st, char *options, BUFFER *wb)
{
    buffer_strcat(wb, RRD_GRAPH_JSON_HEADER);
    rrd_stats_one_json(st, options, wb);
    buffer_strcat(wb, RRD_GRAPH_JSON_FOOTER);
}

void rrd_stats_all_json(BUFFER *wb)
{
    unsigned long memory = 0;
    long c;
    RRDSET *st;

    buffer_strcat(wb, RRD_GRAPH_JSON_HEADER);

    pthread_rwlock_rdlock(&localhost.rrdset_root_rwlock);
    for(st = localhost.rrdset_root, c = 0; st ; st = st->next) {
        if(st->enabled && st->dimensions) {
            if(c) buffer_strcat(wb, ",\n");
            memory += rrd_stats_one_json(st, NULL, wb);
            c++;
        }
    }
    pthread_rwlock_unlock(&localhost.rrdset_root_rwlock);

    buffer_sprintf(wb, "\n\t],\n"
        "\t\"hostname\": \"%s\",\n"
        "\t\"update_every\": %d,\n"
        "\t\"history\": %d,\n"
        "\t\"memory\": %lu\n"
        "}\n"
        , localhost.hostname
        , rrd_update_every
        , rrd_default_history_entries
        , memory
        );
}



// ----------------------------------------------------------------------------

// RRDR dimension options
#define RRDR_EMPTY      0x01 // the dimension contains / the value is empty (null)
#define RRDR_RESET      0x02 // the dimension contains / the value is reset
#define RRDR_HIDDEN     0x04 // the dimension contains / the value is hidden
#define RRDR_NONZERO    0x08 // the dimension contains / the value is non-zero

// RRDR result options
#define RRDR_RESULT_OPTION_ABSOLUTE 0x00000001
#define RRDR_RESULT_OPTION_RELATIVE 0x00000002

typedef struct rrdresult {
    RRDSET *st;         // the chart this result refers to

    uint32_t result_options;    // RRDR_RESULT_OPTION_*

    int d;                  // the number of dimensions
    long n;                 // the number of values in the arrays
    long rows;              // the number of rows used

    uint8_t *od;            // the options for the dimensions

    time_t *t;              // array of n timestamps
    calculated_number *v;   // array n x d values
    uint8_t *o;             // array n x d options

    long c;                 // current line ( -1 ~ n ), ( -1 = none, use rrdr_rows() to get number of rows )

    long group;             // how many collected values were grouped for each row
    int update_every;       // what is the suggested update frequency in seconds

    calculated_number min;
    calculated_number max;

    time_t before;
    time_t after;

    int has_st_lock;        // if st is read locked by us
} RRDR;

#define rrdr_rows(r) ((r)->rows)

/*
static void rrdr_dump(RRDR *r)
{
    long c, i;
    RRDDIM *d;

    fprintf(stderr, "\nCHART %s (%s)\n", r->st->id, r->st->name);

    for(c = 0, d = r->st->dimensions; d ;c++, d = d->next) {
        fprintf(stderr, "DIMENSION %s (%s), %s%s%s%s\n"
                , d->id
                , d->name
                , (r->od[c] & RRDR_EMPTY)?"EMPTY ":""
                , (r->od[c] & RRDR_RESET)?"RESET ":""
                , (r->od[c] & RRDR_HIDDEN)?"HIDDEN ":""
                , (r->od[c] & RRDR_NONZERO)?"NONZERO ":""
                );
    }

    if(r->rows <= 0) {
        fprintf(stderr, "RRDR does not have any values in it.\n");
        return;
    }

    fprintf(stderr, "RRDR includes %d values in it:\n", r->rows);

    // for each line in the array
    for(i = 0; i < r->rows ;i++) {
        calculated_number *cn = &r->v[ i * r->d ];
        uint8_t *co = &r->o[ i * r->d ];

        // print the id and the timestamp of the line
        fprintf(stderr, "%ld %ld ", i + 1, r->t[i]);

        // for each dimension
        for(c = 0, d = r->st->dimensions; d ;c++, d = d->next) {
            if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
            if(unlikely(!(r->od[c] & RRDR_NONZERO))) continue;

            if(co[c] & RRDR_EMPTY)
                fprintf(stderr, "null ");
            else
                fprintf(stderr, CALCULATED_NUMBER_FORMAT " %s%s%s%s "
                    , cn[c]
                    , (co[c] & RRDR_EMPTY)?"E":" "
                    , (co[c] & RRDR_RESET)?"R":" "
                    , (co[c] & RRDR_HIDDEN)?"H":" "
                    , (co[c] & RRDR_NONZERO)?"N":" "
                    );
        }

        fprintf(stderr, "\n");
    }
}
*/

void rrdr_disable_not_selected_dimensions(RRDR *r, const char *dims)
{
    char b[strlen(dims) + 1];
    char *o = b, *tok;
    strcpy(o, dims);

    long c;
    RRDDIM *d;

    // disable all of them
    for(c = 0, d = r->st->dimensions; d ;c++, d = d->next)
        r->od[c] |= RRDR_HIDDEN;

    while(o && *o && (tok = mystrsep(&o, ",|"))) {
        if(!*tok) continue;
        
        uint32_t hash = simple_hash(tok);

        // find it and enable it
        for(c = 0, d = r->st->dimensions; d ;c++, d = d->next) {
            if(unlikely((hash == d->hash && !strcmp(d->id, tok)) || !strcmp(d->name, tok))) {
                r->od[c] &= ~RRDR_HIDDEN;

                // since the user needs this dimension
                // make it appear as NONZERO, to return it
                // even if the dimension has only zeros
                r->od[c] |= RRDR_NONZERO;
            }
        }
    }
}

void rrdr_buffer_print_format(BUFFER *wb, uint32_t format)
{
    switch(format) {
    case DATASOURCE_JSON:
        buffer_strcat(wb, DATASOURCE_FORMAT_JSON);
        break;

    case DATASOURCE_DATATABLE_JSON:
        buffer_strcat(wb, DATASOURCE_FORMAT_DATATABLE_JSON);
        break;

    case DATASOURCE_DATATABLE_JSONP:
        buffer_strcat(wb, DATASOURCE_FORMAT_DATATABLE_JSONP);
        break;

    case DATASOURCE_JSONP:
        buffer_strcat(wb, DATASOURCE_FORMAT_JSONP);
        break;

    case DATASOURCE_SSV:
        buffer_strcat(wb, DATASOURCE_FORMAT_SSV);
        break;

    case DATASOURCE_CSV:
        buffer_strcat(wb, DATASOURCE_FORMAT_CSV);
        break;

    case DATASOURCE_TSV:
        buffer_strcat(wb, DATASOURCE_FORMAT_TSV);
        break;

    case DATASOURCE_HTML:
        buffer_strcat(wb, DATASOURCE_FORMAT_HTML);
        break;

    case DATASOURCE_JS_ARRAY:
        buffer_strcat(wb, DATASOURCE_FORMAT_JS_ARRAY);
        break;

    case DATASOURCE_SSV_COMMA:
        buffer_strcat(wb, DATASOURCE_FORMAT_SSV_COMMA);
        break;

    default:
        buffer_strcat(wb, "unknown");
        break;
    }
}

uint32_t rrdr_check_options(RRDR *r, uint32_t options, const char *dims)
{
    if(options & RRDR_OPTION_NONZERO) {
        long i;

        if(dims && *dims) {
            // the caller wants specific dimensions
            // disable NONZERO option
            // to make sure we don't accidentally prevent
            // the specific dimensions from being returned
            i = 0;
        }
        else {
            // find how many dimensions are not zero
            long c;
            RRDDIM *rd;
            for(c = 0, i = 0, rd = r->st->dimensions; rd && c < r->d ; c++, rd = rd->next) {
                if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
                if(unlikely(!(r->od[c] & RRDR_NONZERO))) continue;
                i++;
            }
        }

        // if with nonzero we get i = 0 (no dimensions will be returned)
        // disable nonzero to show all dimensions
        if(!i) options &= ~RRDR_OPTION_NONZERO;
    }

    return options;
}

void rrdr_json_wrapper_begin(RRDR *r, BUFFER *wb, uint32_t format, uint32_t options, int string_value)
{
    long rows = rrdr_rows(r);
    long c, i;
    RRDDIM *rd;

    //info("JSONWRAPPER(): %s: BEGIN", r->st->id);
    char kq[2] = "",                    // key quote
        sq[2] = "";                     // string quote

    if( options & RRDR_OPTION_GOOGLE_JSON ) {
        kq[0] = '\0';
        sq[0] = '\'';
    }
    else {
        kq[0] = '"';
        sq[0] = '"';
    }

    buffer_sprintf(wb, "{\n"
            "   %sapi%s: 1,\n"
            "   %sid%s: %s%s%s,\n"
            "   %sname%s: %s%s%s,\n"
            "   %sview_update_every%s: %d,\n"
            "   %supdate_every%s: %d,\n"
            "   %sfirst_entry%s: %u,\n"
            "   %slast_entry%s: %u,\n"
            "   %sbefore%s: %u,\n"
            "   %safter%s: %u,\n"
            "   %sdimension_names%s: ["
            , kq, kq
            , kq, kq, sq, r->st->id, sq
            , kq, kq, sq, r->st->name, sq
            , kq, kq, r->update_every
            , kq, kq, r->st->update_every
            , kq, kq, (uint32_t)rrdset_first_entry_t(r->st)
            , kq, kq, (uint32_t)rrdset_last_entry_t(r->st)
            , kq, kq, (uint32_t)r->before
            , kq, kq, (uint32_t)r->after
            , kq, kq);

    for(c = 0, i = 0, rd = r->st->dimensions; rd && c < r->d ;c++, rd = rd->next) {
        if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
        if(unlikely((options & RRDR_OPTION_NONZERO) && !(r->od[c] & RRDR_NONZERO))) continue;

        if(i) buffer_strcat(wb, ", ");
        buffer_strcat(wb, sq);
        buffer_strcat(wb, rd->name);
        buffer_strcat(wb, sq);
        i++;
    }
    if(!i) {
#ifdef NETDATA_INTERNAL_CHECKS
        error("RRDR is empty for %s (RRDR has %d dimensions, options is 0x%08x)", r->st->id, r->d, options);
#endif
        rows = 0;
        buffer_strcat(wb, sq);
        buffer_strcat(wb, "no data");
        buffer_strcat(wb, sq);
    }

    buffer_sprintf(wb, "],\n"
            "   %sdimension_ids%s: ["
            , kq, kq);

    for(c = 0, i = 0, rd = r->st->dimensions; rd && c < r->d ;c++, rd = rd->next) {
        if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
        if(unlikely((options & RRDR_OPTION_NONZERO) && !(r->od[c] & RRDR_NONZERO))) continue;

        if(i) buffer_strcat(wb, ", ");
        buffer_strcat(wb, sq);
        buffer_strcat(wb, rd->id);
        buffer_strcat(wb, sq);
        i++;
    }
    if(!i) {
        rows = 0;
        buffer_strcat(wb, sq);
        buffer_strcat(wb, "no data");
        buffer_strcat(wb, sq);
    }

    buffer_sprintf(wb, "],\n"
            "   %slatest_values%s: ["
            , kq, kq);

    for(c = 0, i = 0, rd = r->st->dimensions; rd && c < r->d ;c++, rd = rd->next) {
        if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
        if(unlikely((options & RRDR_OPTION_NONZERO) && !(r->od[c] & RRDR_NONZERO))) continue;

        if(i) buffer_strcat(wb, ", ");
        i++;

        storage_number n = rd->values[rrdset_last_slot(r->st)];

        if(!does_storage_number_exist(n))
            buffer_strcat(wb, "null");
        else
            buffer_rrd_value(wb, unpack_storage_number(n));
    }
    if(!i) {
        rows = 0;
        buffer_strcat(wb, "null");
    }

    buffer_sprintf(wb, "],\n"
            "   %sview_latest_values%s: ["
            , kq, kq);

    i = 0;
    if(rows) {
        for(c = 0, i = 0, rd = r->st->dimensions; rd && c < r->d ;c++, rd = rd->next) {
            if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
            if(unlikely((options & RRDR_OPTION_NONZERO) && !(r->od[c] & RRDR_NONZERO))) continue;

            if(i) buffer_strcat(wb, ", ");
            i++;

            calculated_number *cn = &r->v[ (0) * r->d ];
            uint8_t *co = &r->o[ (0) * r->d ];

            if(co[c] & RRDR_EMPTY)
                buffer_strcat(wb, "null");
            else
                buffer_rrd_value(wb, cn[c]);
        }
    }
    if(!i) {
        rows = 0;
        buffer_strcat(wb, "null");
    }

    buffer_sprintf(wb, "],\n"
            "   %sdimensions%s: %ld,\n"
            "   %spoints%s: %ld,\n"
            "   %sformat%s: %s"
            , kq, kq, i
            , kq, kq, rows
            , kq, kq, sq
            );

    rrdr_buffer_print_format(wb, format);

    buffer_sprintf(wb, "%s,\n"
            "   %sresult%s: "
            , sq
            , kq, kq
            );

    if(string_value) buffer_strcat(wb, sq);
    //info("JSONWRAPPER(): %s: END", r->st->id);
}

void rrdr_json_wrapper_end(RRDR *r, BUFFER *wb, uint32_t format, uint32_t options, int string_value)
{
    (void)format;

    char kq[2] = "",                    // key quote
        sq[2] = "";                     // string quote

    if( options & RRDR_OPTION_GOOGLE_JSON ) {
        kq[0] = '\0';
        sq[0] = '\'';
    }
    else {
        kq[0] = '"';
        sq[0] = '"';
    }

    if(string_value) buffer_strcat(wb, sq);

    buffer_sprintf(wb, ",\n %smin%s: ", kq, kq);
    buffer_rrd_value(wb, r->min);
    buffer_sprintf(wb, ",\n %smax%s: ", kq, kq);
    buffer_rrd_value(wb, r->max);
    buffer_strcat(wb, "\n}\n");
}

#define JSON_DATES_JS 1
#define JSON_DATES_TIMESTAMP 2

static void rrdr2json(RRDR *r, BUFFER *wb, uint32_t options, int datatable)
{
    //info("RRD2JSON(): %s: BEGIN", r->st->id);
    int row_annotations = 0, dates, dates_with_new = 0;
    char kq[2] = "",                    // key quote
        sq[2] = "",                     // string quote
        pre_label[101] = "",            // before each label
        post_label[101] = "",           // after each label
        pre_date[101] = "",             // the beginning of line, to the date
        post_date[101] = "",            // closing the date
        pre_value[101] = "",            // before each value
        post_value[101] = "",           // after each value
        post_line[101] = "",            // at the end of each row
        normal_annotation[201] = "",    // default row annotation
        overflow_annotation[201] = "",  // overflow row annotation
        data_begin[101] = "",           // between labels and values
        finish[101] = "";               // at the end of everything

    if(datatable) {
        dates = JSON_DATES_JS;
        if( options & RRDR_OPTION_GOOGLE_JSON ) {
            kq[0] = '\0';
            sq[0] = '\'';
        }
        else {
            kq[0] = '"';
            sq[0] = '"';
        }
        row_annotations = 1;
        snprintfz(pre_date,   100, "        {%sc%s:[{%sv%s:%s", kq, kq, kq, kq, sq);
        snprintfz(post_date,  100, "%s}", sq);
        snprintfz(pre_label,  100, ",\n     {%sid%s:%s%s,%slabel%s:%s", kq, kq, sq, sq, kq, kq, sq);
        snprintfz(post_label, 100, "%s,%spattern%s:%s%s,%stype%s:%snumber%s}", sq, kq, kq, sq, sq, kq, kq, sq, sq);
        snprintfz(pre_value,  100, ",{%sv%s:", kq, kq);
        strcpy(post_value,         "}");
        strcpy(post_line,          "]}");
        snprintfz(data_begin, 100, "\n  ],\n    %srows%s:\n [\n", kq, kq);
        strcpy(finish,             "\n  ]\n}");

        snprintfz(overflow_annotation, 200, ",{%sv%s:%sRESET OR OVERFLOW%s},{%sv%s:%sThe counters have been wrapped.%s}", kq, kq, sq, sq, kq, kq, sq, sq);
        snprintfz(normal_annotation,   200, ",{%sv%s:null},{%sv%s:null}", kq, kq, kq, kq);

        buffer_sprintf(wb, "{\n %scols%s:\n [\n", kq, kq);
        buffer_sprintf(wb, "        {%sid%s:%s%s,%slabel%s:%stime%s,%spattern%s:%s%s,%stype%s:%sdatetime%s},\n", kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq);
        buffer_sprintf(wb, "        {%sid%s:%s%s,%slabel%s:%s%s,%spattern%s:%s%s,%stype%s:%sstring%s,%sp%s:{%srole%s:%sannotation%s}},\n", kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, kq, kq, sq, sq);
        buffer_sprintf(wb, "        {%sid%s:%s%s,%slabel%s:%s%s,%spattern%s:%s%s,%stype%s:%sstring%s,%sp%s:{%srole%s:%sannotationText%s}}", kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, kq, kq, sq, sq);

        // remove the valueobjects flag
        // google wants its own keys
        if(options & RRDR_OPTION_OBJECTSROWS)
            options &= ~RRDR_OPTION_OBJECTSROWS;
    }
    else {
        kq[0] = '"';
        sq[0] = '"';
        if((options & RRDR_OPTION_SECONDS) || (options & RRDR_OPTION_MILLISECONDS)) {
            dates = JSON_DATES_TIMESTAMP;
            dates_with_new = 0;
        }
        else {
            dates = JSON_DATES_JS;
            dates_with_new = 1;
        }
        if( options & RRDR_OPTION_OBJECTSROWS )
            strcpy(pre_date, "      { ");
        else
            strcpy(pre_date, "      [ ");
        strcpy(pre_label,  ", \"");
        strcpy(post_label, "\"");
        strcpy(pre_value,  ", ");
        if( options & RRDR_OPTION_OBJECTSROWS )
            strcpy(post_line, "}");
        else
            strcpy(post_line, "]");
        snprintfz(data_begin, 100, "],\n    %sdata%s:\n [\n", kq, kq);
        strcpy(finish,             "\n  ]\n}");

        buffer_sprintf(wb, "{\n %slabels%s: [", kq, kq);
        buffer_sprintf(wb, "%stime%s", sq, sq);
    }

    // -------------------------------------------------------------------------
    // print the JSON header

    long c, i;
    RRDDIM *rd;

    // print the header lines
    for(c = 0, i = 0, rd = r->st->dimensions; rd && c < r->d ;c++, rd = rd->next) {
        if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
        if(unlikely((options & RRDR_OPTION_NONZERO) && !(r->od[c] & RRDR_NONZERO))) continue;

        buffer_strcat(wb, pre_label);
        buffer_strcat(wb, rd->name);
        buffer_strcat(wb, post_label);
        i++;
    }
    if(!i) {
        buffer_strcat(wb, pre_label);
        buffer_strcat(wb, "no data");
        buffer_strcat(wb, post_label);
    }

    // print the begin of row data
    buffer_strcat(wb, data_begin);

    // if all dimensions are hidden, print a null
    if(!i) {
        buffer_strcat(wb, finish);
        return;
    }

    long start = 0, end = rrdr_rows(r), step = 1;
    if((options & RRDR_OPTION_REVERSED)) {
        start = rrdr_rows(r) - 1;
        end = -1;
        step = -1;
    }

    // for each line in the array
    calculated_number total = 1;
    for(i = start; i != end ;i += step) {
        calculated_number *cn = &r->v[ i * r->d ];
        uint8_t *co = &r->o[ i * r->d ];

        time_t now = r->t[i];

        if(dates == JSON_DATES_JS) {
            // generate the local date time
            struct tm tmbuf, *tm = localtime_r(&now, &tmbuf);
            if(!tm) { error("localtime_r() failed."); continue; }

            if(likely(i != start)) buffer_strcat(wb, ",\n");
            buffer_strcat(wb, pre_date);

            if( options & RRDR_OPTION_OBJECTSROWS )
                buffer_sprintf(wb, "%stime%s: ", kq, kq);

            if(dates_with_new)
                buffer_strcat(wb, "new ");

            buffer_jsdate(wb, tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

            buffer_strcat(wb, post_date);

            if(row_annotations) {
                // google supports one annotation per row
                int annotation_found = 0;
                for(c = 0, rd = r->st->dimensions; rd ;c++, rd = rd->next) {
                    if(co[c] & RRDR_RESET) {
                        buffer_strcat(wb, overflow_annotation);
                        annotation_found = 1;
                        break;
                    }
                }
                if(!annotation_found)
                    buffer_strcat(wb, normal_annotation);
            }
        }
        else {
            // print the timestamp of the line
            if(likely(i != start)) buffer_strcat(wb, ",\n");
            buffer_strcat(wb, pre_date);

            if( options & RRDR_OPTION_OBJECTSROWS )
                buffer_sprintf(wb, "%stime%s: ", kq, kq);

            buffer_rrd_value(wb, (calculated_number)r->t[i]);
            // in ms
            if(options & RRDR_OPTION_MILLISECONDS) buffer_strcat(wb, "000");

            buffer_strcat(wb, post_date);
        }

        if(unlikely(options & RRDR_OPTION_PERCENTAGE)) {
            total = 0;
            for(c = 0, rd = r->st->dimensions; rd && c < r->d ;c++, rd = rd->next) {
                calculated_number n = cn[c];

                if(likely((options & RRDR_OPTION_ABSOLUTE) && n < 0))
                    n = -n;

                total += n;
            }
            // prevent a division by zero
            if(total == 0) total = 1;
        }

        // for each dimension
        for(c = 0, rd = r->st->dimensions; rd && c < r->d ;c++, rd = rd->next) {
            if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
            if(unlikely((options & RRDR_OPTION_NONZERO) && !(r->od[c] & RRDR_NONZERO))) continue;

            calculated_number n = cn[c];

            buffer_strcat(wb, pre_value);

            if( options & RRDR_OPTION_OBJECTSROWS )
                buffer_sprintf(wb, "%s%s%s: ", kq, rd->name, kq);

            if(co[c] & RRDR_EMPTY) {
                if(options & RRDR_OPTION_NULL2ZERO)
                    buffer_strcat(wb, "0");
                else
                    buffer_strcat(wb, "null");
            }
            else {
                if(unlikely((options & RRDR_OPTION_ABSOLUTE) && n < 0))
                    n = -n;

                if(unlikely(options & RRDR_OPTION_PERCENTAGE))
                    n = n * 100 / total;

                buffer_rrd_value(wb, n);
            }

            buffer_strcat(wb, post_value);
        }

        buffer_strcat(wb, post_line);
    }

    buffer_strcat(wb, finish);
    //info("RRD2JSON(): %s: END", r->st->id);
}

static void rrdr2csv(RRDR *r, BUFFER *wb, uint32_t options, const char *startline, const char *separator, const char *endline, const char *betweenlines)
{
    //info("RRD2CSV(): %s: BEGIN", r->st->id);
    long c, i;
    RRDDIM *d;

    // print the csv header
    for(c = 0, i = 0, d = r->st->dimensions; d && c < r->d ;c++, d = d->next) {
        if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
        if(unlikely((options & RRDR_OPTION_NONZERO) && !(r->od[c] & RRDR_NONZERO))) continue;

        if(!i) {
            buffer_strcat(wb, startline);
            if(options & RRDR_OPTION_LABEL_QUOTES) buffer_strcat(wb, "\"");
            buffer_strcat(wb, "time");
            if(options & RRDR_OPTION_LABEL_QUOTES) buffer_strcat(wb, "\"");
        }
        buffer_strcat(wb, separator);
        if(options & RRDR_OPTION_LABEL_QUOTES) buffer_strcat(wb, "\"");
        buffer_strcat(wb, d->name);
        if(options & RRDR_OPTION_LABEL_QUOTES) buffer_strcat(wb, "\"");
        i++;
    }
    buffer_strcat(wb, endline);

    if(!i) {
        // no dimensions present
        return;
    }

    long start = 0, end = rrdr_rows(r), step = 1;
    if((options & RRDR_OPTION_REVERSED)) {
        start = rrdr_rows(r) - 1;
        end = -1;
        step = -1;
    }

    // for each line in the array
    calculated_number total = 1;
    for(i = start; i != end ;i += step) {
        calculated_number *cn = &r->v[ i * r->d ];
        uint8_t *co = &r->o[ i * r->d ];

        buffer_strcat(wb, betweenlines);
        buffer_strcat(wb, startline);

        time_t now = r->t[i];

        if((options & RRDR_OPTION_SECONDS) || (options & RRDR_OPTION_MILLISECONDS)) {
            // print the timestamp of the line
            buffer_rrd_value(wb, (calculated_number)now);
            // in ms
            if(options & RRDR_OPTION_MILLISECONDS) buffer_strcat(wb, "000");
        }
        else {
            // generate the local date time
            struct tm tmbuf, *tm = localtime_r(&now, &tmbuf);
            if(!tm) { error("localtime() failed."); continue; }
            buffer_date(wb, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        }

        if(unlikely(options & RRDR_OPTION_PERCENTAGE)) {
            total = 0;
            for(c = 0, d = r->st->dimensions; d && c < r->d ;c++, d = d->next) {
                calculated_number n = cn[c];

                if(likely((options & RRDR_OPTION_ABSOLUTE) && n < 0))
                    n = -n;

                total += n;
            }
            // prevent a division by zero
            if(total == 0) total = 1;
        }

        // for each dimension
        for(c = 0, d = r->st->dimensions; d && c < r->d ;c++, d = d->next) {
            if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
            if(unlikely((options & RRDR_OPTION_NONZERO) && !(r->od[c] & RRDR_NONZERO))) continue;

            buffer_strcat(wb, separator);

            calculated_number n = cn[c];

            if(co[c] & RRDR_EMPTY) {
                if(options & RRDR_OPTION_NULL2ZERO)
                    buffer_strcat(wb, "0");
                else
                    buffer_strcat(wb, "null");
            }
            else {
                if(unlikely((options & RRDR_OPTION_ABSOLUTE) && n < 0))
                    n = -n;

                if(unlikely(options & RRDR_OPTION_PERCENTAGE))
                    n = n * 100 / total;

                buffer_rrd_value(wb, n);
            }
        }

        buffer_strcat(wb, endline);
    }
    //info("RRD2CSV(): %s: END", r->st->id);
}

inline static calculated_number rrdr2value(RRDR *r, long i, uint32_t options, int *all_values_are_null) {
    long c;
    RRDDIM *d;

    calculated_number *cn = &r->v[ i * r->d ];
    uint8_t *co = &r->o[ i * r->d ];

    calculated_number sum = 0, min = 0, max = 0, v;
    int all_null = 1, init = 1;

    calculated_number total = 1;
    if(unlikely(options & RRDR_OPTION_PERCENTAGE)) {
        total = 0;
        for(c = 0, d = r->st->dimensions; d && c < r->d ;c++, d = d->next) {
            calculated_number n = cn[c];

            if(likely((options & RRDR_OPTION_ABSOLUTE) && n < 0))
                n = -n;

            total += n;
        }
        // prevent a division by zero
        if(total == 0) total = 1;
    }

    // for each dimension
    for(c = 0, d = r->st->dimensions; d && c < r->d ;c++, d = d->next) {
        if(unlikely(r->od[c] & RRDR_HIDDEN)) continue;
        if(unlikely((options & RRDR_OPTION_NONZERO) && !(r->od[c] & RRDR_NONZERO))) continue;

        calculated_number n = cn[c];

        if(likely((options & RRDR_OPTION_ABSOLUTE) && n < 0))
            n = -n;

        if(unlikely(options & RRDR_OPTION_PERCENTAGE))
            n = n * 100 / total;

        if(unlikely(init)) {
            if(n > 0) {
                min = 0;
                max = n;
            }
            else {
                min = n;
                max = 0;
            }
            init = 0;
        }

        if(likely(!(co[c] & RRDR_EMPTY))) {
            all_null = 0;
            sum += n;
        }

        if(n < min) min = n;
        if(n > max) max = n;
    }

    if(unlikely(all_null)) {
        if(likely(all_values_are_null))
            *all_values_are_null = 1;
        return 0;
    }
    else {
        if(likely(all_values_are_null))
            *all_values_are_null = 0;
    }

    if(options & RRDR_OPTION_MIN2MAX)
        v = max - min;
    else
        v = sum;

    return v;
}

static void rrdr2ssv(RRDR *r, BUFFER *wb, uint32_t options, const char *prefix, const char *separator, const char *suffix)
{
    //info("RRD2SSV(): %s: BEGIN", r->st->id);
    long i;

    buffer_strcat(wb, prefix);
    long start = 0, end = rrdr_rows(r), step = 1;
    if((options & RRDR_OPTION_REVERSED)) {
        start = rrdr_rows(r) - 1;
        end = -1;
        step = -1;
    }

    // for each line in the array
    for(i = start; i != end ;i += step) {
        int all_values_are_null = 0;
        calculated_number v = rrdr2value(r, i, options, &all_values_are_null);

        if(likely(i != start)) {
            if(r->min > v) r->min = v;
            if(r->max < v) r->max = v;
        }
        else {
            r->min = v;
            r->max = v;
        }

        if(likely(i != start))
            buffer_strcat(wb, separator);

        if(all_values_are_null) {
            if(options & RRDR_OPTION_NULL2ZERO)
                buffer_strcat(wb, "0");
            else
                buffer_strcat(wb, "null");
        }
        else
            buffer_rrd_value(wb, v);
    }
    buffer_strcat(wb, suffix);
    //info("RRD2SSV(): %s: END", r->st->id);
}

inline static calculated_number *rrdr_line_values(RRDR *r)
{
    return &r->v[ r->c * r->d ];
}

inline static uint8_t *rrdr_line_options(RRDR *r)
{
    return &r->o[ r->c * r->d ];
}

inline static int rrdr_line_init(RRDR *r, time_t t)
{
    r->c++;

    if(unlikely(r->c >= r->n)) {
        error("requested to step above RRDR size for chart %s", r->st->name);
        r->c = r->n - 1;
    }

    // save the time
    r->t[r->c] = t;

    return 1;
}

inline static void rrdr_lock_rrdset(RRDR *r) {
    if(unlikely(!r)) {
        error("NULL value given!");
        return;
    }

    pthread_rwlock_rdlock(&r->st->rwlock);
    r->has_st_lock = 1;
}

inline static void rrdr_unlock_rrdset(RRDR *r) {
    if(unlikely(!r)) {
        error("NULL value given!");
        return;
    }

    if(likely(r->has_st_lock)) {
        pthread_rwlock_unlock(&r->st->rwlock);
        r->has_st_lock = 0;
    }
}

inline static void rrdr_free(RRDR *r)
{
    if(unlikely(!r)) {
        error("NULL value given!");
        return;
    }

    rrdr_unlock_rrdset(r);
    freez(r->t);
    freez(r->v);
    freez(r->o);
    freez(r->od);
    freez(r);
}

inline void rrdr_done(RRDR *r)
{
    r->rows = r->c + 1;
    r->c = 0;
}

static RRDR *rrdr_create(RRDSET *st, long n)
{
    if(unlikely(!st)) {
        error("NULL value given!");
        return NULL;
    }

    RRDR *r = callocz(1, sizeof(RRDR));
    r->st = st;

    rrdr_lock_rrdset(r);

    RRDDIM *rd;
    for(rd = st->dimensions ; rd ; rd = rd->next) r->d++;

    r->n = n;

    r->t = mallocz(n * sizeof(time_t));
    r->v = mallocz(n * r->d * sizeof(calculated_number));
    r->o = mallocz(n * r->d * sizeof(uint8_t));
    r->od = mallocz(r->d * sizeof(uint8_t));

    // set the hidden flag on hidden dimensions
    int c;
    for(c = 0, rd = st->dimensions ; rd ; c++, rd = rd->next) {
        if(unlikely(rd->flags & RRDDIM_FLAG_HIDDEN)) r->od[c] = RRDR_HIDDEN;
        else r->od[c] = 0;
    }

    r->c = -1;
    r->group = 1;
    r->update_every = 1;

    return r;
}

RRDR *rrd2rrdr(RRDSET *st, long points, long long after, long long before, int group_method, int aligned)
{
    int debug = st->debug;
    int absolute_period_requested = -1;

    time_t first_entry_t = rrdset_first_entry_t(st);
    time_t last_entry_t  = rrdset_last_entry_t(st);

    if(before == 0 && after == 0) {
        before = last_entry_t;
        after = first_entry_t;
        absolute_period_requested = 0;
    }

    // allow relative for before and after (smaller than 3 years)
    if(((before < 0)?-before:before) <= (3 * 365 * 86400)) {
        before = last_entry_t + before;
        absolute_period_requested = 0;
    }

    if(((after < 0)?-after:after) <= (3 * 365 * 86400)) {
        if(after == 0) after = -st->update_every;
        after = before + after;
        absolute_period_requested = 0;
    }

    if(absolute_period_requested == -1)
        absolute_period_requested = 1;

    // make sure they are within our timeframe
    if(before > last_entry_t)  before = last_entry_t;
    if(before < first_entry_t) before = first_entry_t;

    if(after > last_entry_t)  after = last_entry_t;
    if(after < first_entry_t) after = first_entry_t;

    // check if they are upside down
    if(after > before) {
        time_t tmp = before;
        before = after;
        after = tmp;
    }

    // the duration of the chart
    time_t duration = before - after;
    long available_points = duration / st->update_every;

    if(duration <= 0 || available_points <= 0)
        return rrdr_create(st, 1);

    // check the wanted points
    if(points < 0) points = -points;
    if(points > available_points) points = available_points;
    if(points == 0) points = available_points;

    // calculate proper grouping of source data
    long group = available_points / points;
    if(group <= 0) group = 1;

    // round group to the closest integer
    if(available_points % points > points / 2) group++;

    time_t after_new  = (aligned) ? (after  - (after  % (group * st->update_every))) : after;
    time_t before_new = (aligned) ? (before - (before % (group * st->update_every))) : before;
    long points_new   = (before_new - after_new) / st->update_every / group;

    // find the starting and ending slots in our round robin db
    long    start_at_slot = rrdset_time2slot(st, before_new),
            stop_at_slot  = rrdset_time2slot(st, after_new);

#ifdef NETDATA_INTERNAL_CHECKS
    if(after_new < first_entry_t) {
        error("after_new %u is too small, minimum %u", (uint32_t)after_new, (uint32_t)first_entry_t);
    }
    if(after_new > last_entry_t) {
        error("after_new %u is too big, maximum %u", (uint32_t)after_new, (uint32_t)last_entry_t);
    }
    if(before_new < first_entry_t) {
        error("before_new %u is too small, minimum %u", (uint32_t)before_new, (uint32_t)first_entry_t);
    }
    if(before_new > last_entry_t) {
        error("before_new %u is too big, maximum %u", (uint32_t)before_new, (uint32_t)last_entry_t);
    }
    if(start_at_slot < 0 || start_at_slot >= st->entries) {
        error("start_at_slot is invalid %ld, expected 0 to %ld", start_at_slot, st->entries - 1);
    }
    if(stop_at_slot < 0 || stop_at_slot >= st->entries) {
        error("stop_at_slot is invalid %ld, expected 0 to %ld", stop_at_slot, st->entries - 1);
    }
    if(points_new > (before_new - after_new) / group / st->update_every + 1) {
        error("points_new %ld is more than points %ld", points_new, (before_new - after_new) / group / st->update_every + 1);
    }
#endif

    //info("RRD2RRDR(): %s: wanted %ld points, got %ld - group=%ld, wanted duration=%u, got %u - wanted %ld - %ld, got %ld - %ld", st->id, points, points_new, group, before - after, before_new - after_new, after, before, after_new, before_new);

    after = after_new;
    before = before_new;
    duration = before - after;
    points = points_new;

    // Now we have:
    // before = the end time of the calculation
    // after = the start time of the calculation
    // duration = the duration of the calculation
    // group = the number of source points to aggregate / group together
    // method = the method of grouping source points
    // points = the number of points to generate


    // -------------------------------------------------------------------------
    // initialize our result set

    RRDR *r = rrdr_create(st, points);
    if(!r) {
#ifdef NETDATA_INTERNAL_CHECKS
        error("Cannot create RRDR for %s, after=%u, before=%u, duration=%u, points=%ld", st->id, (uint32_t)after, (uint32_t)before, (uint32_t)duration, points);
#endif
        return NULL;
    }
    if(!r->d) {
#ifdef NETDATA_INTERNAL_CHECKS
        error("Returning empty RRDR (no dimensions in RRDSET) for %s, after=%u, before=%u, duration=%u, points=%ld", st->id, (uint32_t)after, (uint32_t)before, (uint32_t)duration, points);
#endif
        return r;
    }

    if(absolute_period_requested == 1)
        r->result_options |= RRDR_RESULT_OPTION_ABSOLUTE;
    else
        r->result_options |= RRDR_RESULT_OPTION_RELATIVE;

    // find how many dimensions we have
    long dimensions = r->d;


    // -------------------------------------------------------------------------
    // checks for debugging

    if(debug) debug(D_RRD_STATS, "INFO %s first_t: %u, last_t: %u, all_duration: %u, after: %u, before: %u, duration: %u, points: %ld, group: %ld"
            , st->id
            , (uint32_t)first_entry_t
            , (uint32_t)last_entry_t
            , (uint32_t)(last_entry_t - first_entry_t)
            , (uint32_t)after
            , (uint32_t)before
            , (uint32_t)duration
            , points
            , group
            );


    // -------------------------------------------------------------------------
    // temp arrays for keeping values per dimension

    calculated_number   last_values[dimensions]; // keep the last value of each dimension
    calculated_number   group_values[dimensions]; // keep sums when grouping
    long                group_counts[dimensions]; // keep the number of values added to group_values
    uint8_t             group_options[dimensions];
    uint8_t             found_non_zero[dimensions];


    // initialize them
    RRDDIM *rd;
    long c;
    for( rd = st->dimensions, c = 0 ; rd && c < dimensions ; rd = rd->next, c++) {
        last_values[c] = 0;
        group_values[c] = (group_method == GROUP_MAX || group_method == GROUP_MIN)?NAN:0;
        group_counts[c] = 0;
        group_options[c] = 0;
        found_non_zero[c] = 0;
    }


    // -------------------------------------------------------------------------
    // the main loop

    time_t  now = rrdset_slot2time(st, start_at_slot),
            dt = st->update_every,
            group_start_t = 0;

    if(unlikely(debug)) debug(D_RRD_STATS, "BEGIN %s after_t: %u (stop_at_t: %ld), before_t: %u (start_at_t: %ld), start_t(now): %u, current_entry: %ld, entries: %ld"
            , st->id
            , (uint32_t)after
            , stop_at_slot
            , (uint32_t)before
            , start_at_slot
            , (uint32_t)now
            , st->current_entry
            , st->entries
            );

    r->group = group;
    r->update_every = group * st->update_every;
    r->before = now;
    r->after = now;

    //info("RRD2RRDR(): %s: STARTING", st->id);

    long slot = start_at_slot, counter = 0, stop_now = 0, added = 0, group_count = 0, add_this = 0;
    for(; !stop_now ; now -= dt, slot--, counter++) {
        if(unlikely(slot < 0)) slot = st->entries - 1;
        if(unlikely(slot == stop_at_slot)) stop_now = counter;

        if(unlikely(debug)) debug(D_RRD_STATS, "ROW %s slot: %ld, entries_counter: %ld, group_count: %ld, added: %ld, now: %ld, %s %s"
                , st->id
                , slot
                , counter
                , group_count + 1
                , added
                , now
                , (group_count + 1 == group)?"PRINT":"  -  "
                , (now >= after && now <= before)?"RANGE":"  -  "
                );

        // make sure we return data in the proper time range
        if(unlikely(now > before)) continue;
        if(unlikely(now < after)) break;

        if(unlikely(group_count == 0)) {
            group_start_t = now;
        }
        group_count++;

        if(unlikely(group_count == group)) {
            if(unlikely(added >= points)) break;
            add_this = 1;
        }

        // do the calculations
        for(rd = st->dimensions, c = 0 ; rd && c < dimensions ; rd = rd->next, c++) {
            storage_number n = rd->values[slot];
            if(unlikely(!does_storage_number_exist(n))) continue;

            group_counts[c]++;

            calculated_number value = unpack_storage_number(n);
            if(likely(value != 0.0)) {
                group_options[c] |= RRDR_NONZERO;
                found_non_zero[c] = 1;
            }

            if(unlikely(did_storage_number_reset(n)))
                group_options[c] |= RRDR_RESET;

            switch(group_method) {
                case GROUP_MIN:
                    if(unlikely(isnan(group_values[c])) ||
                            fabsl(value) < fabsl(group_values[c]))
                        group_values[c] = value;
                    break;

                case GROUP_MAX:
                    if(unlikely(isnan(group_values[c])) ||
                            fabsl(value) > fabsl(group_values[c]))
                        group_values[c] = value;
                    break;

                default:
                case GROUP_SUM:
                case GROUP_AVERAGE:
                case GROUP_UNDEFINED:
                    group_values[c] += value;
                    break;

                case GROUP_INCREMENTAL_SUM:
                    if(unlikely(slot == start_at_slot))
                        last_values[c] = value;

                    group_values[c] += last_values[c] - value;
                    last_values[c] = value;
                    break;
            }
        }

        // added it
        if(unlikely(add_this)) {
            if(unlikely(!rrdr_line_init(r, group_start_t))) break;

            r->after = now;

            calculated_number *cn = rrdr_line_values(r);
            uint8_t *co = rrdr_line_options(r);

            for(rd = st->dimensions, c = 0 ; rd && c < dimensions ; rd = rd->next, c++) {

                // update the dimension options
                if(likely(found_non_zero[c])) r->od[c] |= RRDR_NONZERO;

                // store the specific point options
                co[c] = group_options[c];

                // store the value
                if(unlikely(group_counts[c] == 0)) {
                    cn[c] = 0.0;
                    co[c] |= RRDR_EMPTY;
                    group_values[c] = (group_method == GROUP_MAX || group_method == GROUP_MIN)?NAN:0;
                }
                else {
                    switch(group_method) {
                        case GROUP_MIN:
                        case GROUP_MAX:
                            if(unlikely(isnan(group_values[c])))
                                cn[c] = 0;
                            else {
                                cn[c] = group_values[c];
                                group_values[c] = NAN;
                            }
                            break;

                        case GROUP_SUM:
                        case GROUP_INCREMENTAL_SUM:
                            cn[c] = group_values[c];
                            group_values[c] = 0;
                            break;

                        default:
                        case GROUP_AVERAGE:
                        case GROUP_UNDEFINED:
                            cn[c] = group_values[c] / group_counts[c];
                            group_values[c] = 0;
                            break;
                    }

                    if(cn[c] < r->min) r->min = cn[c];
                    if(cn[c] > r->max) r->max = cn[c];
                }

                // reset for the next loop
                group_counts[c] = 0;
                group_options[c] = 0;
            }

            added++;
            group_count = 0;
            add_this = 0;
        }
    }

    rrdr_done(r);
    //info("RRD2RRDR(): %s: END %ld loops made, %ld points generated", st->id, counter, rrdr_rows(r));
    //error("SHIFT: %s: wanted %ld points, got %ld", st->id, points, rrdr_rows(r));
    return r;
}

int rrd2value(RRDSET *st, BUFFER *wb, calculated_number *n, const char *dimensions, long points, long long after, long long before, int group_method, uint32_t options, time_t *db_after, time_t *db_before, int *value_is_null)
{
    RRDR *r = rrd2rrdr(st, points, after, before, group_method, !(options & RRDR_OPTION_NOT_ALIGNED));
    if(!r) {
        if(value_is_null) *value_is_null = 1;
        return 500;
    }

    if(rrdr_rows(r) == 0) {
        rrdr_free(r);

        if(db_after)  *db_after  = 0;
        if(db_before) *db_before = 0;
        if(value_is_null) *value_is_null = 1;

        return 400;
    }

    if(r->result_options & RRDR_RESULT_OPTION_RELATIVE)
        buffer_no_cacheable(wb);
    else if(r->result_options & RRDR_RESULT_OPTION_ABSOLUTE)
        buffer_cacheable(wb);

    options = rrdr_check_options(r, options, dimensions);

    if(dimensions)
        rrdr_disable_not_selected_dimensions(r, dimensions);

    if(db_after)  *db_after  = r->after;
    if(db_before) *db_before = r->before;

    long i = (options & RRDR_OPTION_REVERSED)?rrdr_rows(r) - 1:0;
    *n = rrdr2value(r, i, options, value_is_null);

    rrdr_free(r);
    return 200;
}

int rrd2format(RRDSET *st, BUFFER *wb, BUFFER *dimensions, uint32_t format, long points, long long after, long long before, int group_method, uint32_t options, time_t *latest_timestamp)
{
    RRDR *r = rrd2rrdr(st, points, after, before, group_method, !(options & RRDR_OPTION_NOT_ALIGNED));
    if(!r) {
        buffer_strcat(wb, "Cannot generate output with these parameters on this chart.");
        return 500;
    }

    if(r->result_options & RRDR_RESULT_OPTION_RELATIVE)
        buffer_no_cacheable(wb);
    else if(r->result_options & RRDR_RESULT_OPTION_ABSOLUTE)
        buffer_cacheable(wb);

    options = rrdr_check_options(r, options, (dimensions)?buffer_tostring(dimensions):NULL);

    if(dimensions)
        rrdr_disable_not_selected_dimensions(r, buffer_tostring(dimensions));

    if(latest_timestamp && rrdr_rows(r) > 0)
        *latest_timestamp = r->before;

    switch(format) {
    case DATASOURCE_SSV:
        if(options & RRDR_OPTION_JSON_WRAP) {
            wb->contenttype = CT_APPLICATION_JSON;
            rrdr_json_wrapper_begin(r, wb, format, options, 1);
            rrdr2ssv(r, wb, options, "", " ", "");
            rrdr_json_wrapper_end(r, wb, format, options, 1);
        }
        else {
            wb->contenttype = CT_TEXT_PLAIN;
            rrdr2ssv(r, wb, options, "", " ", "");
        }
        break;

    case DATASOURCE_SSV_COMMA:
        if(options & RRDR_OPTION_JSON_WRAP) {
            wb->contenttype = CT_APPLICATION_JSON;
            rrdr_json_wrapper_begin(r, wb, format, options, 1);
            rrdr2ssv(r, wb, options, "", ",", "");
            rrdr_json_wrapper_end(r, wb, format, options, 1);
        }
        else {
            wb->contenttype = CT_TEXT_PLAIN;
            rrdr2ssv(r, wb, options, "", ",", "");
        }
        break;

    case DATASOURCE_JS_ARRAY:
        if(options & RRDR_OPTION_JSON_WRAP) {
            wb->contenttype = CT_APPLICATION_JSON;
            rrdr_json_wrapper_begin(r, wb, format, options, 0);
            rrdr2ssv(r, wb, options, "[", ",", "]");
            rrdr_json_wrapper_end(r, wb, format, options, 0);
        }
        else {
            wb->contenttype = CT_APPLICATION_JSON;
            rrdr2ssv(r, wb, options, "[", ",", "]");
        }
        break;

    case DATASOURCE_CSV:
        if(options & RRDR_OPTION_JSON_WRAP) {
            wb->contenttype = CT_APPLICATION_JSON;
            rrdr_json_wrapper_begin(r, wb, format, options, 1);
            rrdr2csv(r, wb, options, "", ",", "\\n", "");
            rrdr_json_wrapper_end(r, wb, format, options, 1);
        }
        else {
            wb->contenttype = CT_TEXT_PLAIN;
            rrdr2csv(r, wb, options, "", ",", "\r\n", "");
        }
        break;

    case DATASOURCE_CSV_JSON_ARRAY:
        wb->contenttype = CT_APPLICATION_JSON;
        if(options & RRDR_OPTION_JSON_WRAP) {
            rrdr_json_wrapper_begin(r, wb, format, options, 0);
            buffer_strcat(wb, "[\n");
            rrdr2csv(r, wb, options + RRDR_OPTION_LABEL_QUOTES, "[", ",", "]", ",\n");
            buffer_strcat(wb, "\n]");
            rrdr_json_wrapper_end(r, wb, format, options, 0);
        }
        else {
            wb->contenttype = CT_TEXT_PLAIN;
            buffer_strcat(wb, "[\n");
            rrdr2csv(r, wb, options + RRDR_OPTION_LABEL_QUOTES, "[", ",", "]", ",\n");
            buffer_strcat(wb, "\n]");
        }
        break;

    case DATASOURCE_TSV:
        if(options & RRDR_OPTION_JSON_WRAP) {
            wb->contenttype = CT_APPLICATION_JSON;
            rrdr_json_wrapper_begin(r, wb, format, options, 1);
            rrdr2csv(r, wb, options, "", "\t", "\\n", "");
            rrdr_json_wrapper_end(r, wb, format, options, 1);
        }
        else {
            wb->contenttype = CT_TEXT_PLAIN;
            rrdr2csv(r, wb, options, "", "\t", "\r\n", "");
        }
        break;

    case DATASOURCE_HTML:
        if(options & RRDR_OPTION_JSON_WRAP) {
            wb->contenttype = CT_APPLICATION_JSON;
            rrdr_json_wrapper_begin(r, wb, format, options, 1);
            buffer_strcat(wb, "<html>\\n<center>\\n<table border=\\\"0\\\" cellpadding=\\\"5\\\" cellspacing=\\\"5\\\">\\n");
            rrdr2csv(r, wb, options, "<tr><td>", "</td><td>", "</td></tr>\\n", "");
            buffer_strcat(wb, "</table>\\n</center>\\n</html>\\n");
            rrdr_json_wrapper_end(r, wb, format, options, 1);
        }
        else {
            wb->contenttype = CT_TEXT_HTML;
            buffer_strcat(wb, "<html>\n<center>\n<table border=\"0\" cellpadding=\"5\" cellspacing=\"5\">\n");
            rrdr2csv(r, wb, options, "<tr><td>", "</td><td>", "</td></tr>\n", "");
            buffer_strcat(wb, "</table>\n</center>\n</html>\n");
        }
        break;

    case DATASOURCE_DATATABLE_JSONP:
        wb->contenttype = CT_APPLICATION_X_JAVASCRIPT;

        if(options & RRDR_OPTION_JSON_WRAP)
            rrdr_json_wrapper_begin(r, wb, format, options, 0);

        rrdr2json(r, wb, options, 1);

        if(options & RRDR_OPTION_JSON_WRAP)
            rrdr_json_wrapper_end(r, wb, format, options, 0);
        break;

    case DATASOURCE_DATATABLE_JSON:
        wb->contenttype = CT_APPLICATION_JSON;

        if(options & RRDR_OPTION_JSON_WRAP)
            rrdr_json_wrapper_begin(r, wb, format, options, 0);

        rrdr2json(r, wb, options, 1);

        if(options & RRDR_OPTION_JSON_WRAP)
            rrdr_json_wrapper_end(r, wb, format, options, 0);
        break;

    case DATASOURCE_JSONP:
        wb->contenttype = CT_APPLICATION_X_JAVASCRIPT;
        if(options & RRDR_OPTION_JSON_WRAP)
            rrdr_json_wrapper_begin(r, wb, format, options, 0);

        rrdr2json(r, wb, options, 0);

        if(options & RRDR_OPTION_JSON_WRAP)
            rrdr_json_wrapper_end(r, wb, format, options, 0);
        break;

    case DATASOURCE_JSON:
    default:
        wb->contenttype = CT_APPLICATION_JSON;

        if(options & RRDR_OPTION_JSON_WRAP)
            rrdr_json_wrapper_begin(r, wb, format, options, 0);

        rrdr2json(r, wb, options, 0);

        if(options & RRDR_OPTION_JSON_WRAP)
            rrdr_json_wrapper_end(r, wb, format, options, 0);
        break;
    }

    rrdr_free(r);
    return 200;
}

time_t rrd_stats_json(int type, RRDSET *st, BUFFER *wb, long points, long group, int group_method, time_t after, time_t before, int only_non_zero)
{
    int c;
    pthread_rwlock_rdlock(&st->rwlock);


    // -------------------------------------------------------------------------
    // switch from JSON to google JSON

    char kq[2] = "\"";
    char sq[2] = "\"";
    switch(type) {
        case DATASOURCE_DATATABLE_JSON:
        case DATASOURCE_DATATABLE_JSONP:
            kq[0] = '\0';
            sq[0] = '\'';
            break;

        case DATASOURCE_JSON:
        default:
            break;
    }


    // -------------------------------------------------------------------------
    // validate the parameters

    if(points < 1) points = 1;
    if(group < 1) group = 1;

    if(before == 0 || before > rrdset_last_entry_t(st)) before = rrdset_last_entry_t(st);
    if(after  == 0 || after < rrdset_first_entry_t(st)) after = rrdset_first_entry_t(st);

    // ---

    // our return value (the last timestamp printed)
    // this is required to detect re-transmit in google JSONP
    time_t last_timestamp = 0;


    // -------------------------------------------------------------------------
    // find how many dimensions we have

    int dimensions = 0;
    RRDDIM *rd;
    for( rd = st->dimensions ; rd ; rd = rd->next) dimensions++;
    if(!dimensions) {
        pthread_rwlock_unlock(&st->rwlock);
        buffer_strcat(wb, "No dimensions yet.");
        return 0;
    }


    // -------------------------------------------------------------------------
    // prepare various strings, to speed up the loop

    char overflow_annotation[201]; snprintfz(overflow_annotation, 200, ",{%sv%s:%sRESET OR OVERFLOW%s},{%sv%s:%sThe counters have been wrapped.%s}", kq, kq, sq, sq, kq, kq, sq, sq);
    char normal_annotation[201];   snprintfz(normal_annotation,   200, ",{%sv%s:null},{%sv%s:null}", kq, kq, kq, kq);
    char pre_date[51];             snprintfz(pre_date,             50, "        {%sc%s:[{%sv%s:%s", kq, kq, kq, kq, sq);
    char post_date[21];            snprintfz(post_date,            20, "%s}", sq);
    char pre_value[21];            snprintfz(pre_value,            20, ",{%sv%s:", kq, kq);
    char post_value[21];           strcpy(post_value,                  "}");


    // -------------------------------------------------------------------------
    // checks for debugging

    if(st->debug) {
        debug(D_RRD_STATS, "%s first_entry_t = %ld, last_entry_t = %ld, duration = %ld, after = %ld, before = %ld, duration = %ld, entries_to_show = %ld, group = %ld"
            , st->id
            , rrdset_first_entry_t(st)
            , rrdset_last_entry_t(st)
            , rrdset_last_entry_t(st) - rrdset_first_entry_t(st)
            , after
            , before
            , before - after
            , points
            , group
            );

        if(before < after)
            debug(D_RRD_STATS, "WARNING: %s The newest value in the database (%ld) is earlier than the oldest (%ld)", st->name, before, after);

        if((before - after) > st->entries * st->update_every)
            debug(D_RRD_STATS, "WARNING: %s The time difference between the oldest and the newest entries (%ld) is higher than the capacity of the database (%ld)", st->name, before - after, st->entries * st->update_every);
    }


    // -------------------------------------------------------------------------
    // temp arrays for keeping values per dimension

    calculated_number group_values[dimensions]; // keep sums when grouping
    int               print_hidden[dimensions]; // keep hidden flags
    int               found_non_zero[dimensions];
    int               found_non_existing[dimensions];

    // initialize them
    for( rd = st->dimensions, c = 0 ; rd && c < dimensions ; rd = rd->next, c++) {
        group_values[c] = 0;
        print_hidden[c] = (rd->flags & RRDDIM_FLAG_HIDDEN)?1:0;
        found_non_zero[c] = 0;
        found_non_existing[c] = 0;
    }


    // error("OLD: points=%d after=%d before=%d group=%d, duration=%d", entries_to_show, before - (st->update_every * group * entries_to_show), before, group, before - after + 1);
    // rrd2array(st, entries_to_show, before - (st->update_every * group * entries_to_show), before, group_method, only_non_zero);
    // rrd2rrdr(st, entries_to_show, before - (st->update_every * group * entries_to_show), before, group_method);

    // -------------------------------------------------------------------------
    // remove dimensions that contain only zeros

    int max_loop = 1;
    if(only_non_zero) max_loop = 2;

    for(; max_loop ; max_loop--) {

        // -------------------------------------------------------------------------
        // print the JSON header

        buffer_sprintf(wb, "{\n %scols%s:\n [\n", kq, kq);
        buffer_sprintf(wb, "        {%sid%s:%s%s,%slabel%s:%stime%s,%spattern%s:%s%s,%stype%s:%sdatetime%s},\n", kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq);
        buffer_sprintf(wb, "        {%sid%s:%s%s,%slabel%s:%s%s,%spattern%s:%s%s,%stype%s:%sstring%s,%sp%s:{%srole%s:%sannotation%s}},\n", kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, kq, kq, sq, sq);
        buffer_sprintf(wb, "        {%sid%s:%s%s,%slabel%s:%s%s,%spattern%s:%s%s,%stype%s:%sstring%s,%sp%s:{%srole%s:%sannotationText%s}}", kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, sq, sq, kq, kq, kq, kq, sq, sq);

        // print the header for each dimension
        // and update the print_hidden array for the dimensions that should be hidden
        int pc = 0;
        for( rd = st->dimensions, c = 0 ; rd && c < dimensions ; rd = rd->next, c++) {
            if(!print_hidden[c]) {
                pc++;
                buffer_sprintf(wb, ",\n     {%sid%s:%s%s,%slabel%s:%s%s%s,%spattern%s:%s%s,%stype%s:%snumber%s}", kq, kq, sq, sq, kq, kq, sq, rd->name, sq, kq, kq, sq, sq, kq, kq, sq, sq);
            }
        }
        if(!pc) {
            buffer_sprintf(wb, ",\n     {%sid%s:%s%s,%slabel%s:%s%s%s,%spattern%s:%s%s,%stype%s:%snumber%s}", kq, kq, sq, sq, kq, kq, sq, "no data", sq, kq, kq, sq, sq, kq, kq, sq, sq);
        }

        // print the begin of row data
        buffer_sprintf(wb, "\n  ],\n    %srows%s:\n [\n", kq, kq);


        // -------------------------------------------------------------------------
        // the main loop

        int annotate_reset = 0;
        int annotation_count = 0;

        long    t = rrdset_time2slot(st, before),
                stop_at_t = rrdset_time2slot(st, after),
                stop_now = 0;

        t -= t % group;

        time_t  now = rrdset_slot2time(st, t),
                dt = st->update_every;

        long count = 0, printed = 0, group_count = 0;
        last_timestamp = 0;

        if(st->debug) debug(D_RRD_STATS, "%s: REQUEST after:%u before:%u, points:%ld, group:%ld, CHART cur:%ld first: %u last:%u, CALC start_t:%ld, stop_t:%ld"
                    , st->id
                    , (uint32_t)after
                    , (uint32_t)before
                    , points
                    , group
                    , st->current_entry
                    , (uint32_t)rrdset_first_entry_t(st)
                    , (uint32_t)rrdset_last_entry_t(st)
                    , t
                    , stop_at_t
                    );

        long counter = 0;
        for(; !stop_now ; now -= dt, t--, counter++) {
            if(t < 0) t = st->entries - 1;
            if(t == stop_at_t) stop_now = counter;

            int print_this = 0;

            if(st->debug) debug(D_RRD_STATS, "%s t = %ld, count = %ld, group_count = %ld, printed = %ld, now = %ld, %s %s"
                    , st->id
                    , t
                    , count + 1
                    , group_count + 1
                    , printed
                    , now
                    , (group_count + 1 == group)?"PRINT":"  -  "
                    , (now >= after && now <= before)?"RANGE":"  -  "
                    );


            // make sure we return data in the proper time range
            if(now > before) continue;
            if(now < after) break;

            //if(rrdset_slot2time(st, t) != now)
            //  error("%s: slot=%ld, now=%ld, slot2time=%ld, diff=%ld, last_entry_t=%ld, rrdset_last_slot=%ld", st->id, t, now, rrdset_slot2time(st,t), now - rrdset_slot2time(st,t), rrdset_last_entry_t(st), rrdset_last_slot(st));

            count++;
            group_count++;

            // check if we have to print this now
            if(group_count == group) {
                if(printed >= points) {
                    // debug(D_RRD_STATS, "Already printed all rows. Stopping.");
                    break;
                }

                // generate the local date time
                struct tm tmbuf, *tm = localtime_r(&now, &tmbuf);
                if(!tm) { error("localtime() failed."); continue; }
                if(now > last_timestamp) last_timestamp = now;

                if(printed) buffer_strcat(wb, "]},\n");
                buffer_strcat(wb, pre_date);
                buffer_jsdate(wb, tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
                buffer_strcat(wb, post_date);

                print_this = 1;
            }

            // do the calculations
            for(rd = st->dimensions, c = 0 ; rd && c < dimensions ; rd = rd->next, c++) {
                storage_number n = rd->values[t];
                calculated_number value = unpack_storage_number(n);

                if(!does_storage_number_exist(n)) {
                    value = 0.0;
                    found_non_existing[c]++;
                }
                if(did_storage_number_reset(n)) annotate_reset = 1;

                switch(group_method) {
                    case GROUP_MAX:
                        if(abs(value) > abs(group_values[c])) group_values[c] = value;
                        break;

                    case GROUP_SUM:
                        group_values[c] += value;
                        break;

                    default:
                    case GROUP_AVERAGE:
                        group_values[c] += value;
                        if(print_this) group_values[c] /= ( group_count - found_non_existing[c] );
                        break;
                }
            }

            if(print_this) {
                if(annotate_reset) {
                    annotation_count++;
                    buffer_strcat(wb, overflow_annotation);
                    annotate_reset = 0;
                }
                else
                    buffer_strcat(wb, normal_annotation);

                pc = 0;
                for(c = 0 ; c < dimensions ; c++) {
                    if(found_non_existing[c] == group_count) {
                        // all entries are non-existing
                        pc++;
                        buffer_strcat(wb, pre_value);
                        buffer_strcat(wb, "null");
                        buffer_strcat(wb, post_value);
                    }
                    else if(!print_hidden[c]) {
                        pc++;
                        buffer_strcat(wb, pre_value);
                        buffer_rrd_value(wb, group_values[c]);
                        buffer_strcat(wb, post_value);

                        if(group_values[c]) found_non_zero[c]++;
                    }

                    // reset them for the next loop
                    group_values[c] = 0;
                    found_non_existing[c] = 0;
                }

                // if all dimensions are hidden, print a null
                if(!pc) {
                    buffer_strcat(wb, pre_value);
                    buffer_strcat(wb, "null");
                    buffer_strcat(wb, post_value);
                }

                printed++;
                group_count = 0;
            }
        }

        if(printed) buffer_strcat(wb, "]}");
        buffer_strcat(wb, "\n   ]\n}\n");

        if(only_non_zero && max_loop > 1) {
            int changed = 0;
            for(rd = st->dimensions, c = 0 ; rd && c < dimensions ; rd = rd->next, c++) {
                group_values[c] = 0;
                found_non_existing[c] = 0;

                if(!print_hidden[c] && !found_non_zero[c]) {
                    changed = 1;
                    print_hidden[c] = 1;
                }
            }

            if(changed) buffer_flush(wb);
            else break;
        }
        else break;

    } // max_loop

    debug(D_RRD_STATS, "RRD_STATS_JSON: %s total %lu bytes", st->name, wb->len);

    pthread_rwlock_unlock(&st->rwlock);
    return last_timestamp;
}
