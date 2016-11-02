# -*- coding: utf-8 -*-
# Description: apache netdata python.d module
# Author: Pawel Krupa (paulfantom)

from base import UrlService

# default module values (can be overridden per job in `config`)
# update_every = 2
priority = 60000
retries = 60

# default job configuration (overridden by python.d.plugin)
# config = {'local': {
#             'update_every': update_every,
#             'retries': retries,
#             'priority': priority,
#             'url': 'http://www.apache.org/server-status?auto'
#          }}

# charts order (can be overridden if you want less charts, or different order)
ORDER = ['requests', 'connections', 'conns_async', 'net', 'workers', 'reqpersec', 'bytespersec', 'bytesperreq']

CHARTS = {
    'bytesperreq': {
        'options': [None, 'apache Lifetime Avg. Response Size', 'bytes/request', 'statistics', 'apache.bytesperreq', 'area'],
        'lines': [
            ["size_req"]
        ]},
    'workers': {
        'options': [None, 'apache Workers', 'workers', 'workers', 'apache.workers', 'stacked'],
        'lines': [
            ["idle"],
            ["busy"]
        ]},
    'reqpersec': {
        'options': [None, 'apache Lifetime Avg. Requests/s', 'requests/s', 'statistics', 'apache.reqpersec', 'area'],
        'lines': [
            ["requests_sec"]
        ]},
    'bytespersec': {
        'options': [None, 'apache Lifetime Avg. Bandwidth/s', 'kilobytes/s', 'statistics', 'apache.bytesperreq', 'area'],
        'lines': [
            ["size_sec", None, 'absolute', 1, 1000]
        ]},
    'requests': {
        'options': [None, 'apache Requests', 'requests/s', 'requests', 'apache.requests', 'line'],
        'lines': [
            ["requests", None, 'incremental']
        ]},
    'net': {
        'options': [None, 'apache Bandwidth', 'kilobytes/s', 'bandwidth', 'apache.net', 'area'],
        'lines': [
            ["sent", None, 'incremental']
        ]},
    'connections': {
        'options': [None, 'apache Connections', 'connections', 'connections', 'apache.connections', 'line'],
        'lines': [
            ["connections"]
        ]},
    'conns_async': {
        'options': [None, 'apache Async Connections', 'connections', 'connections', 'apache.conns_async', 'stacked'],
        'lines': [
            ["keepalive"],
            ["closing"],
            ["writing"]
        ]}
}


class Service(UrlService):
    def __init__(self, configuration=None, name=None):
        UrlService.__init__(self, configuration=configuration, name=name)
        if len(self.url) == 0:
            self.url = "http://localhost/server-status?auto"
        self.order = ORDER
        self.definitions = CHARTS
        self.assignment = {"BytesPerReq": 'size_req',
                           "IdleWorkers": 'idle',
                           "BusyWorkers": 'busy',
                           "ReqPerSec": 'requests_sec',
                           "BytesPerSec": 'size_sec',
                           "Total Accesses": 'requests',
                           "Total kBytes": 'sent',
                           "ConnsTotal": 'connections',
                           "ConnsAsyncKeepAlive": 'keepalive',
                           "ConnsAsyncClosing": 'closing',
                           "ConnsAsyncWriting": 'writing'}

    def _get_data(self):
        """
        Format data received from http request
        :return: dict
        """
        try:
            raw = self._get_raw_data().split("\n")
        except AttributeError:
            return None
        data = {}
        for row in raw:
            tmp = row.split(":")
            if str(tmp[0]) in self.assignment:
                try:
                    data[self.assignment[tmp[0]]] = int(float(tmp[1]))
                except (IndexError, ValueError):
                    pass
        if len(data) == 0:
            return None
        return data
