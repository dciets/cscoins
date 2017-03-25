import json
import os
import sys
# import crypto
# sys.modules['Crypto'] = crypto
import ChallengeSolver
import time
from Crypto.Hash import SHA256
from coinslib import BaseClient
from coinslib import Challenge

from ProcessManagerThread import ProcessManagerThread

class MinerClient(BaseClient):
    def __init__(self, key_dirs="", hostname="localhost", port=8989, ssl=False):
        BaseClient.__init__(self, hostname, port, ssl)
        self.keys_dir = key_dirs
        self.time_limit = 0
        self.solvers = []
        self.solvers.append(ChallengeSolver.SortedListSolverNative(asc=True))
        self.solvers.append(ChallengeSolver.SortedListSolverNative(asc=False))
        self.solvers.append(ChallengeSolver.ShortestPathSolverNative())

    async def client_loop(self):
        register_wallet = False
        pub_path = "key.pub"
        priv_path = "key.priv"
        print("WERWERzzz")

        if len(self.keys_dir) > 0:
            pub_path = os.path.join(self.keys_dir, pub_path)
            priv_path = os.path.join(self.keys_dir, priv_path)

        if os.path.exists(pub_path) and os.path.exists(priv_path):
            self.load_keys(pub_path, priv_path)

        else:
            self.generate_wallet_keys()
            self.export_keys(pub_path, priv_path)
            register_wallet = True

        print("WERWERWER")
        # generating wallet id
        self.generate_wallet_id()

        await self.connect()
        print("WERWERWER")
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

        await self.mine_loop()

    async def wait_receive(self):
        message = await self.socket.recv()
        data = json.loads(message)
        return data

    async def mine_loop(self):
        manager_thread = ProcessManagerThread(self.keys_dir, self.hostname, self.port, self.ssl)
        manager_thread.start()
        previous_challenge = None
        current_challenge = None
        response = None

        while True:
            response = await self.get_current_challenge()
            current_challenge = Challenge()

            try:
                current_challenge.fill_from_challenge(response)
                self.time_limit = int(time.time()) + int(response['time_left'])
            except:
                continue

            if previous_challenge is None or current_challenge.id != previous_challenge.id:
                previous_challenge = current_challenge
                manager_thread.lock.acquire()
                manager_thread.next_challenge = current_challenge
                manager_thread.lock.release()
                manager_thread.e.set()

            time.sleep(15)
