from coinslib import MT64, seed_from_hash
import hashlib
import random
import ctypes
import binascii
import subprocess

class ChallengeSolver:
    def __init__(self, challenge_name):
        self.challenge_name = challenge_name
        self.mt = None

    def feed_prng(self, previous_hash, nonce):
        hasher = hashlib.sha256()
        hasher.update("{0}{1}".format(previous_hash, nonce).encode("ascii"))
        seed_hash = hasher.hexdigest()

        seed = seed_from_hash(seed_hash)
        self.mt = MT64(seed)

    def solve(self, parameters, hash_prefix, previous_hash):
        pass


class SortedListSolverNative(ChallengeSolver):
    def __init__(self, asc):
        if asc:
            ChallengeSolver.__init__(self, 'sorted_list')
            self.asc = 1
        else:
            ChallengeSolver.__init__(self, 'reverse_sorted_list')
            self.asc = 0

    def solve(self, parameters, hash_prefix, previous_hash):
        nb_elements = parameters['nb_elements']

        proc = subprocess.Popen(["./sorted_list.solver", previous_hash, hash_prefix, str(nb_elements), str(self.asc)], stdout=subprocess.PIPE)
        output = proc.communicate()[0].splitlines()

        return output[0].decode("utf-8"), int(output[1])

class ShortestPathSolverNative(ChallengeSolver):
    def __init__(self):
        ChallengeSolver.__init__(self, 'shortest_path')

    def solve(self, parameters, hash_prefix, previous_hash):
        grid_size = parameters['grid_size']
        nb_blockers = parameters['nb_blockers']

        proc = subprocess.Popen(["./shortest_path.solver", previous_hash, hash_prefix, str(grid_size), str(nb_blockers)], stdout=subprocess.PIPE)
        output = proc.communicate()[0].splitlines()

        return output[0].decode("utf-8"), int(output[1])

class SortedListSolver(ChallengeSolver):
    def __init__(self):
        ChallengeSolver.__init__(self, 'sorted_list')

    def solve(self, parameters, hash_prefix, previous_hash):
        nb_elements = parameters['nb_elements']

        nonce = random.randint(0, 99999999)

        while True:
            self.feed_prng(previous_hash, nonce)

            elements = []

            for i in range(nb_elements):
                elements.append(self.mt.extract_number())

            elements.sort()

            solution_string = ""
            for i in elements:
                solution_string += "{0}".format(i)

            sha256 = hashlib.sha256()
            sha256.update(solution_string.encode('ascii'))
            solution_hash = sha256.hexdigest()

            if solution_hash.startswith(hash_prefix):
                print("Solution found ! nonce:{0} hash:{1}".format(nonce, solution_hash))
                return solution_hash, nonce

            nonce = random.randint(0, 99999999)


class ReverseSortedListSolver(ChallengeSolver):
    def __init__(self):
        ChallengeSolver.__init__(self, 'reverse_sorted_list')

    def solve(self, parameters, hash_prefix, previous_hash):
        nb_elements = parameters['nb_elements']

        nonce = random.randint(0, 99999999)

        while True:
            self.feed_prng(previous_hash, nonce)

            elements = []

            for i in range(nb_elements):
                elements.append(self.mt.extract_number())

            elements.sort(reverse=True)

            solution_string = ""
            for i in elements:
                solution_string += "{0}".format(i)

            sha256 = hashlib.sha256()
            sha256.update(solution_string.encode('ascii'))
            solution_hash = sha256.hexdigest()

            if solution_hash.startswith(hash_prefix):
                print("Solution found ! nonce:{0} hash:{1}".format(nonce, solution_hash))
                return solution_hash, nonce

            nonce = random.randint(0, 99999999)
