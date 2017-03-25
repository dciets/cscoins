import asyncio
import ChallengeSolver
import json
import os
import threading
import time
from Crypto.Hash import SHA256
from coinslib import BaseClient
from coinslib import Challenge

from ProcessThread import ProcessThread

class ProcessManagerThread(BaseClient, threading.Thread):
    def __init__(self, key_dirs="", hostname="localhost", port=8989, ssl=False):
        BaseClient.__init__(self, hostname, port, ssl)
        threading.Thread.__init__(self)

        self.keys_dir = key_dirs
        self.time_limit = 0
        self.solvers = {}
        self.solvers["sorted_list"] = ChallengeSolver.SortedListSolverNative(asc=True)
        self.solvers["reverse_sorted_list"] = ChallengeSolver.SortedListSolverNative(asc=False)
        self.solvers["shortest_path"] = ChallengeSolver.ShortestPathSolverNative()

        self.lock = threading.Lock()
        self.e = threading.Event()
        self.next_challenge = None

    def run(self):
        asyncio.set_event_loop(asyncio.new_event_loop())
        asyncio.get_event_loop().run_until_complete(self.client_loop())

    async def client_loop(self):
        register_wallet = False
        pub_path = "key.pub"
        priv_path = "key.priv"

        if len(self.keys_dir) > 0:
            pub_path = os.path.join(self.keys_dir, pub_path)
            priv_path = os.path.join(self.keys_dir, priv_path)

        if os.path.exists(pub_path) and os.path.exists(priv_path):
            self.load_keys(pub_path, priv_path)

        else:
            self.generate_wallet_keys()
            self.export_keys(pub_path, priv_path)
            register_wallet = True

        # generating wallet id
        self.generate_wallet_id()

        await self.connect()

        if register_wallet:
            # create the wallet
            keyString = ""
            for b in self.public_key.exportKey(format='PEM'):
                keyString += chr(b)

            hasher = SHA256.new()
            hasher.update(self.public_key.exportKey(format='DER'))
            signature = self.sign_message(hasher)

            command = {"command": "register_wallet", "args": {"name": 'miner_wallet', "key": keyString, "signature": signature}}
            message = json.dumps(command)
            await self.socket.send(message)
            message = await self.socket.recv()
            response = json.loads(message)

            if response['success']:
                print("Wallet created : {}".format(response["wallet_id"]))
                return

        await self.mine_loop()

    async def mine_loop(self):
        proc_thread = None
        proc = None

        while True:
            self.e.wait()
            self.e.clear()

            self.lock.acquire()

            if self.next_challenge is not None:
                if proc is not None:
                    proc.kill()
                    proc = None
                    proc_thread = None

                if self.next_challenge.challenge_name in self.solvers:
                    print("Searching for a hash matching : {0}".format(self.next_challenge.hash_prefix))

                    proc_thread = ProcessThread(self.solvers[self.next_challenge.challenge_name], self.next_challenge, self.e)
                    proc_thread.start()

            elif proc is not None:
                if proc_thread.success:
                    print("Result -- Hash: {}\nResult -- Nonce: {}".format(proc_thread.hash, proc_thread.nonce))
                    await self.submit(current_challenge.id, proc_thread.hash, proc_thread.nonce)

                proc = None
                proc_thread = None

            self.lock.release()
