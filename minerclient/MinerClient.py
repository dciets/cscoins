import json
import os
import ChallengeSolver
import time
import posix_ipc
import subprocess
from coinslib import BaseClient, Challenge
import asyncio

def recv_client(server):
    print("reading...\n")
    server.clientQueue.request_notification((recv_client, server))
    msg = ""
    try:
        while True:
            msg = server.clientQueue.receive()[0]
    except Exception as e:
        print(str(e))
    result = "".join(map(chr, msg))
    print("MESSAGE FROM CLIENT: " + result)
    
    if not result.startswith("DEBUG:") and result != "":
        info = result.split(" ")
        server.my_solution = (info[0], info[1]);
        server.found_solution = True

class MinerClient(BaseClient):
    def __init__(self, key_dirs="", hostname="localhost", wallet_name="SimpleMiner"):
        BaseClient.__init__(self, hostname)
        self.keys_dir = key_dirs
        self.wallet_name = wallet_name
        self.time_limit = 0
        self.solvers = {}
        #self.solvers["sorted_list"] = ChallengeSolver.SortedListSolver
        #self.solvers["reverse_sorted_list"] = ChallengeSolver.ReverseSortedListSolver
        #self.solvers["shortest_path"] = ChallengeSolver.ShortestPathSolver

        # / Create the message queue, unlink them, and reopen them to make
        # / sure they do not contain anything already. (Useful in case the
        # / application crashed and was unable to close the queues.
        self.serverQueue = posix_ipc.MessageQueue("/CS2017_SERVER_QNAME", posix_ipc.O_CREAT)
        self.clientQueue = posix_ipc.MessageQueue("/CS2017_CLIENT_QNAME", posix_ipc.O_CREAT)
        self.serverQueue.close()
        self.clientQueue.close()
        self.serverQueue.unlink()
        self.clientQueue.unlink()
        self.serverQueue = posix_ipc.MessageQueue("/CS2017_SERVER_QNAME", posix_ipc.O_CREAT)
        self.clientQueue = posix_ipc.MessageQueue("/CS2017_CLIENT_QNAME", posix_ipc.O_CREAT)
        self.serverQueue.block = False
        self.clientQueue.block = False

        # set the call back function for when the client sends a message
        self.clientQueue.request_notification((recv_client, self))
        self.found_solution = False;
        self.my_solution = "";
        proc = subprocess.Popen(["./client"])
        time.sleep(1.5)

        self.solving_thread = None

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
            await self.register_wallet()

        await self.mine_loop()

    async def get_challenge(self):
        response = await self.get_current_challenge()
        current_challenge = Challenge()
        current_challenge.fill_from_challenge(response)
        self.time_limit = int(time.time()) + int(response['time_left'])

        # send new challenge to client solver message queue
        msg = str(current_challenge.id) + " " + current_challenge.challenge_name + " " + current_challenge.last_solution_hash + " " + current_challenge.hash_prefix;
        if current_challenge.challenge_name == "shortest_path":
            msg = msg + " " + str(current_challenge.parameters["grid_size"]) + " " + str(current_challenge.parameters["nb_blockers"])
        else:
            msg = msg + " " + str(current_challenge.parameters["nb_elements"])
        print(msg)

        return current_challenge

    async def solve_challenge(self, challenge):
        # send new challenge to client solver message queue
        msg = str(challenge.id) + " " + challenge.challenge_name + " " + challenge.last_solution_hash + " " + challenge.hash_prefix;
        if challenge.challenge_name == "shortest_path":
            msg = msg + " " + str(challenge.parameters["grid_size"]) + " " + str(challenge.parameters["nb_blockers"])
        else:
            msg = msg + " " + str(challenge.parameters["nb_elements"])
        print(msg)

        solver = None
        try:
            solver = self.solvers[challenge.challenge_name]
        except KeyError:
            print("Solver not found for {0}...".format(challenge.challenge_name))

        if solver == None:
            self.found_solution = False
            print("sending: " + msg)
            res = self.serverQueue.send(msg)
            print(str(res))
            while not self.found_solution:
                await asyncio.sleep(0.5)
            print("leaving function")
            return self.my_solution;
        else:
            self.solving_thread = solver(challenge)
            print("Starting solving thread {0}".format(self.solving_thread.challenge_name))
            self.solving_thread.start()

            while not self.solving_thread.solution_found and self.solving_thread.alive:
                await asyncio.sleep(0.5)
            return self.solving_thread.solution

    async def wait_for_new_challenge(self):
        msg = await self.socket.recv()
        response = json.loads(msg)
        if 'error' in response:
            print(response['error'])
        current_challenge = Challenge()
        try:
            current_challenge = Challenge()
            current_challenge.fill_from_challenge(response)
            self.time_limit = int(time.time()) + int(response['time_left'])
            print("New challenge received")

           

        except:
            current_challenge = None

        return current_challenge

    async def mine_loop(self):
        print("Fetching current challenge")
        current_challenge = await self.get_challenge()

        while True:
            new_challenge = False
            while not new_challenge:
                recv_task = asyncio.ensure_future(self.wait_for_new_challenge())
                mine_task = asyncio.ensure_future(self.solve_challenge(current_challenge))

                done, pending = await asyncio.wait([recv_task, mine_task], return_when=asyncio.FIRST_COMPLETED)

                if mine_task in done:
                    solution = mine_task.result()
                    result = await self.submit(solution[1])
                    if result is not None:
                        # we got a new challenge, right after the submission
                        recv_task.cancel()
                        current_challenge = result
                        new_challenge = True
                    else:
                        await asyncio.wait([recv_task], return_when=asyncio.FIRST_COMPLETED)
                        challenge = recv_task.result()
                        if challenge is not None:
                            new_challenge = True
                            current_challenge = challenge
                            continue
                else:
                    self.solving_thread.alive = False
                    mine_task.cancel()

                if recv_task in done:
                    self.solving_thread.alive = False
                    mine_task.cancel()
                    challenge = recv_task.result()
                    if challenge is not None:
                        new_challenge = True
                        current_challenge = challenge
                else:
                    recv_task.cancel()

                asyncio.sleep(1)
