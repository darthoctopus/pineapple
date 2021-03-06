from ipykernel.kernelbase import Kernel

class EchoKernel(Kernel):
    implementation = 'MyEcho'
    implementation_version = '1.0'
    language = 'yodelling'
    language_version = '0.1'
    language_info = {
        'name': 'text',
        'file_extension': 'txt',
        'mimetype': 'text/plain'
    }
    banner = "Echo kernel - yodelehee yodelehee yodeloo"

    def do_execute(self, code, silent, store_history=True, user_expressions=None,
                   allow_stdin=False):
        with open('echolog.txt', 'a') as f:
            f.write('ECHO SERVER got {}\n'.format(code))
        if not silent:
            stream_content = {'name': 'stdout', 'text': code}
            self.send_response(self.iopub_socket, 'stream', stream_content)

        return {'status': 'ok',
                'execution_count': self.execution_count,
                'payload': [],
                'user_expressions': {},
               }
