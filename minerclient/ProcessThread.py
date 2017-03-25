import threading

class ProcessThread(threading.Thread):
    def __init__(self, solver, challenge, e):
        threading.Thread.__init__(self)

        self.solver = solver
        self.challenge = challenge
        self.e = e
        self.hash = None
        self.nonce = None
        self.success = False

    def run(self):
        out_hash, out_nonce, success = self.solver.solve(self.challenge.parameters, self.challenge.hash_prefix, self.challenge.last_solution_hash)

        if success:
            self.hash = out_hash
            self.nonce = out_nonce
            self.e.set()
