#!/bin/sh
# The daemon starts its script once while that process lives, and again after it exits.
set -eu
cd "$(dirname "$0")"
mkdir -p build
cc -Wall -Wextra -Werror -O2 -o build/wowd wowd.c
export WOWD="$PWD/build/wowd"

python3 - << 'PY'
import os, socket, subprocess, time, pathlib, sys

root = pathlib.Path("/tmp/wow-oma-test")
subprocess.run(["rm", "-rf", str(root)], check=True)
root.mkdir()
sock_path = root / "launch.sock"
script = root / "launch.sh"
log = root / "log"
script.write_text("#!/bin/sh\necho $$ >> log\nexec sleep 30\n")
script.chmod(0o755)

listener = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
listener.bind(str(sock_path))
listener.listen(16)
listener.set_inheritable(True)
if listener.fileno() != 3:
    os.dup2(listener.fileno(), 3)
os.set_inheritable(3, True)

def arm():
    os.environ["LISTEN_PID"] = str(os.getpid())
    os.environ["LISTEN_FDS"] = "1"

daemon = subprocess.Popen(
    [os.environ["WOWD"], str(script)],
    pass_fds=(3,),
    preexec_fn=arm,
    cwd=root,
    stderr=subprocess.PIPE,
)
listener.close()

def poke():
    last = None
    for _ in range(50):
        client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            client.settimeout(2)
            client.connect(str(sock_path))
            reply = client.recv(1)
            client.close()
            return reply
        except OSError as exc:
            last = exc
            client.close()
            time.sleep(0.05)
    err = daemon.stderr.read().decode() if daemon.poll() is not None else ""
    sys.exit(f"socket did not answer ({last}); daemon said: {err}")

def pids():
    if not log.exists():
        return []
    return [int(line) for line in log.read_text().split() if line]

def wait_for(count):
    for _ in range(50):
        text = pids()
        if len(text) >= count:
            return text
        time.sleep(0.05)
    return pids()

try:
    first = poke()
    second = poke()
    text = wait_for(1)
    if first != b"1" or second != b"1" or len(text) != 1:
        sys.exit(f"expected one launch, got replies {first!r} {second!r} pids {text}")
    os.kill(text[0], 15)
    for _ in range(50):
        if not pathlib.Path(f"/proc/{text[0]}").exists():
            break
        time.sleep(0.05)
    third = poke()
    text = wait_for(2)
    if third != b"1" or len(text) != 2:
        sys.exit(f"expected a second launch, got {third!r} pids {text}")
finally:
    for pid in pids():
        try:
            os.kill(pid, 15)
        except ProcessLookupError:
            pass
    daemon.kill()
    daemon.wait(timeout=5)
print("ok")
PY
