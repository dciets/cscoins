import MinerClient
import asyncio
import sys
sys.path.append('..')

keys_dir = ""

if len(sys.argv) > 1:
    keys_dir = sys.argv[1]

mc = MinerClient.MinerClient(keys_dir, "cscoins.2017.csgames.org", 8989, True)
print("WERWER")
asyncio.get_event_loop().run_until_complete(mc.client_loop())
