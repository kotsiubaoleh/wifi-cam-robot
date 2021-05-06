
import http.server
import socket
import ssl

DIRECTORY = 'build'

class RequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

def get_my_ip(port):
    s1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s1.connect(('8.8.8.8', port))
    my_ip = s1.getsockname()[0]
    s1.close()
    return my_ip

def start_https_server(server_ip, server_port):
    httpd = http.server.HTTPServer((server_ip, server_port),
                                    RequestHandler)

    httpd.socket = ssl.wrap_socket(httpd.socket,
                                   keyfile='ca_key.pem',
                                   certfile='ca_cert.pem', server_side=True)
    httpd.serve_forever()

if __name__ == '__main__':
    server_port = 8000
    host_ip = get_my_ip(server_port);
    start_https_server(host_ip, server_port)