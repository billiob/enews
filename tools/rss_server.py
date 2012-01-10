#!/usr/bin/env python3

import time
import http.server
import socketserver

rss_header_top=b"""<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0"
  xmlns:content="http://purl.org/rss/1.0/modules/content/"
  xmlns:dc="http://purl.org/dc/elements/1.1/"
  xmlns:atom="http://www.w3.org/2005/Atom"
  xmlns:sy="http://purl.org/rss/1.0/modules/syndication/"
  xmlns:slash="http://purl.org/rss/1.0/modules/slash/">
  <channel>
    <title>rss test</title>
    <atom:link href="http://localhost:8080/" rel="self" type="application/rss+xml" />
    <link>http://localhost:8080/</link>"""

rss_header_bottom=b"""</channel>
</rss>"""
rss_item="""<item>
      <title>__TITLE__<title>
      <link>http://localhost:8080/__LINK__</link>
      <description>__DESC__</description>
      <pubDate>__DATE__</pubDate>
      <category>__CATEGORY__</category>
       <content:encoded><![CDATA[ CONTENT __CONTENT__]]></content:encoded>
    </item>"""

items = []
class MyHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type',	'text/xml')
        self.end_headers()
        date = time.strftime("%A, %d %B %Y", time.localtime())
        item = rss_item
        item = item.replace('__DATE__', date)
        item = item.replace('__TITLE__', str(len(items)))
        item = item.replace('__DESC__', str(len(items)))
        item = item.replace('__CONTENT__', str(len(items)))
        items.append(item)

        self.wfile.write(rss_header_top)
        for i in reversed(items):
            self.wfile.write(bytes(i, 'ascii'))
        self.wfile.write(rss_header_bottom)

PORT = 8080

def main():
    try:
        server = http.server.HTTPServer(('', PORT), MyHandler)
        print('started httpserver... on port '+str(PORT))
        server.serve_forever()
    except KeyboardInterrupt:
        print('^C received, shutting down server')
        server.socket.close()

if __name__ == '__main__':
    main()
